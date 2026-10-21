// Unit tests for DiscordBear.
//
// The DiscordBear sources are compiled directly into this exe (see TestCore.vcxproj) because the
// DLL only exports GetDiscordClient/SetLogFn, so Message/Pipe/Client internals can't be linked
// from the DLL.
//
// Pipe and Client are tested against a fake Discord: FakeDiscord owns \\.\pipe\discord-ipc-0 and
// answers the protocol frames. No real Discord is needed. One exception: Client::Connect() always
// reaches GetAuthToken(), which does a real HTTPS call to discord.com; that call fails (or throws)
// for the fake app ID, which is expected and caught here. Everything before that step - pipe
// connect, handshake, authorize - succeeds against the fake and leaves the pipe connected.
//
// If real Discord is running, discord-ipc-0 is taken and the pipe tests skip.

#include <windows.h>
#include <tlhelp32.h>
#undef GetMessage // windows.h defines a GetMessage macro; DiscordBear::Message uses that name

#include <nlohmann/json.hpp>
#include <ppl.h>
#include <ppltasks.h>
#include <concurrent_unordered_map.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

// DiscordBear headers get this alias from DiscordBear's PCH, so it must be declared before them.
using json = nlohmann::json;

#include "../DiscordBear/Interface.h"
#include "../DiscordBear/Log.h"
#include "../DiscordBear/LogDefinitions.h"
#include "../DiscordBear/Message.h"
#include "../DiscordBear/Pipe.h"
#include "../DiscordBear/Client.h"

// ---------------------------------------------------------------------------
// Tiny test harness (no framework)
// ---------------------------------------------------------------------------

namespace
{
	int g_checks = 0;
	int g_failures = 0;
	int g_skips = 0;
	std::string g_currentTest;

	struct TestCase
	{
		const char* name;
		void (*fn)();
	};

	std::vector<TestCase>& TestRegistry()
	{
		static std::vector<TestCase> registry;
		return registry;
	}

	void ReportSkip(const std::string& reason)
	{
		++g_skips;
		std::cout << "[ SKIP ] " << g_currentTest << " - " << reason << '\n';
	}
}

#define TEST(name) \
	static void name(); \
	static bool name##Registrar = (TestRegistry().push_back({ #name, &name }), true); \
	static void name()

#define CHECK(expr) \
	do { \
		++g_checks; \
		if (!(expr)) { \
			++g_failures; \
			std::cout << "[ FAIL ] " << __FILE__ << ":" << __LINE__ << " in " << g_currentTest << ": " << #expr << '\n'; \
		} \
	} while (false)

// ---------------------------------------------------------------------------
// FakeDiscord: a stand-in for the Discord RPC pipe server
// ---------------------------------------------------------------------------

// Wire format (must match DiscordBear::Message):
// [opcode:4 LE][length:4 LE][payload:length]
namespace
{
	std::string EncodeFrame(unsigned long opcode, const std::string& payload)
	{
		unsigned long length = static_cast<unsigned long>(payload.size());
		std::string buffer(8 + payload.size(), '\0');
		memcpy(&buffer[0], &opcode, 4);
		memcpy(&buffer[4], &length, 4);
		memcpy(&buffer[8], payload.data(), payload.size());
		return buffer;
	}

	void WriteRaw(HANDLE pipe, const std::string& bytes)
	{
		DWORD written = 0;
		WriteFile(pipe, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
	}

	void EncodeAndWrite(HANDLE pipe, unsigned long opcode, const std::string& payload)
	{
		// Single write so the whole frame lands in the client's pipe buffer at once.
		WriteRaw(pipe, EncodeFrame(opcode, payload));
	}
}

class FakeDiscord final
{
public:
	~FakeDiscord()
	{
		Stop();
	}

	// Fails if real Discord is already holding the pipe; tests should skip in that case.
	bool Start()
	{
		HANDLE probe = CreateFileW(L"\\\\.\\pipe\\discord-ipc-0", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

		if (probe != INVALID_HANDLE_VALUE)
		{
			CloseHandle(probe);
			return false;
		}

		m_serverPipe = CreateNamedPipeW(L"\\\\.\\pipe\\discord-ipc-0", PIPE_ACCESS_DUPLEX, 0, 1, 65536, 65536, 0, nullptr);

		if (m_serverPipe == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		m_running = true;
		m_thread = std::thread(&FakeDiscord::Run, this);
		return true;
	}

	void Stop()
	{
		if (!m_thread.joinable())
		{
			return;
		}

		m_running = false;

		// Closing the handle also aborts a pending ConnectNamedPipe in the server thread.
		if (m_serverPipe != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_serverPipe);
			m_serverPipe = INVALID_HANDLE_VALUE;
		}

		m_thread.join();
	}

	// Send one Frame to the connected client.
	void SendFrame(const std::string& payload)
	{
		EncodeAndWrite(m_serverPipe, 1 /*Frame*/, payload);
	}

	// Write arbitrary bytes, e.g. several frames at once or part of a frame, to exercise framing.
	void SendRaw(const std::string& bytes)
	{
		WriteRaw(m_serverPipe, bytes);
	}

	// Block until the client sends a message; returns its (opcode, payload).
	// Replies to a request are written before it is recorded, so by the time this
	// returns, the reply (if any) is already in the client's pipe buffer.
	std::optional<std::pair<unsigned long, std::string>> WaitForMessage(std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (!m_cv.wait_for(lock, timeout, [this] { return !m_queue.empty(); }))
		{
			return std::nullopt;
		}

		auto message = m_queue.front();
		m_queue.pop_front();
		return message;
	}

	int CountCommand(const std::string& cmd, const std::string& evt)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		int count = 0;
		for (const auto& frame : m_history)
		{
			if (frame.value("cmd", "") == cmd && frame.value("evt", "") == evt)
			{
				++count;
			}
		}
		return count;
	}

	// Blocks until the client has sent a frame with the given cmd/evt (or timeout).
	bool WaitForCommand(const std::string& cmd, const std::string& evt, std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_cv.wait_for(
			lock,
			timeout,
			[this, &cmd, &evt]
				{
					for (const auto& frame : m_history)
					{
						if (frame.value("cmd", "") == cmd && frame.value("evt", "") == evt)
						{
							return true;
						}
					}
					return false;
				});
	}

	std::string HandshakeClientId()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_handshakeClientId;
	}

	void SetVoiceChannelData(json data)
	{
		m_voiceChannelData = std::move(data);
	}

private:
	void Run()
	{
		// Blocks until the client connects.
		ConnectNamedPipe(m_serverPipe, nullptr);

		while (m_running)
		{
			DWORD available = 0;
			if (!PeekNamedPipe(m_serverPipe, nullptr, 0, nullptr, &available, nullptr) || available == 0)
			{
				if (!m_running)
				{
					break;
				}
				Sleep(2);
				continue;
			}

			auto message = ReadMessageFromPipe();
			if (!message)
			{
				break; // client disconnected
			}

			Handle(*message);
		}
	}

	bool ReadAll(void* buffer, DWORD size)
	{
		DWORD total = 0;
		while (total < size)
		{
			DWORD read = 0;
			if (!ReadFile(m_serverPipe, static_cast<char*>(buffer) + total, size - total, &read, nullptr) || read == 0)
			{
				return false;
			}
			total += read;
		}
		return true;
	}

	std::optional<std::pair<unsigned long, std::string>> ReadMessageFromPipe()
	{
		unsigned long opcode = 0;
		unsigned long length = 0;

		if (!ReadAll(&opcode, 4) || !ReadAll(&length, 4) || length == 0 || length > 16 * 1024 * 1024)
		{
			return std::nullopt;
		}

		std::string payload(length, '\0');
		if (!ReadAll(&payload[0], length))
		{
			return std::nullopt;
		}

		return std::make_pair(opcode, std::move(payload));
	}

	void Handle(const std::pair<unsigned long, std::string>& message)
	{
		json parsed;
		try
		{
			parsed = json::parse(message.second);
		}
		catch (...)
		{
			parsed = json::object();
		}

		// Build the reply (if any) and send it before recording the request, so
		// WaitForMessage only returns after the reply reached the client's buffer.
		const std::string cmd = parsed.value("cmd", "");
		std::string reply;

		if (message.first == 0 /*Handshake*/)
		{
			reply = json{{"cmd", "DISPATCH"}, {"evt", "READY"}}.dump();
		}
		else if (cmd == "AUTHORIZE")
		{
			reply = json{{"nonce", parsed.value("nonce", "")}, {"evt", "AUTHORIZE"}, {"data", {{"code", "test-auth-code"}}}}.dump();
		}
		else if (cmd == "AUTHENTICATE")
		{
			reply = json{{"nonce", parsed.value("nonce", "")}, {"evt", "AUTHENTICATE"}, {"data", json::object()}}.dump();
		}
		else if (cmd == "GET_SELECTED_VOICE_CHANNEL")
		{
			reply = json{{"cmd", "GET_CHANNEL"}, {"nonce", parsed.value("nonce", "")}, {"data", m_voiceChannelData}}.dump();
		}
		// SUBSCRIBE / UNSUBSCRIBE get no reply.

		if (!reply.empty())
		{
			EncodeAndWrite(m_serverPipe, 1 /*Frame*/, reply);
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (message.first == 0 /*Handshake*/)
			{
				m_handshakeClientId = parsed.value("client_id", "");
			}
			m_queue.push_back(message);
			m_history.push_back(parsed);
		}
		m_cv.notify_all();
	}

	HANDLE m_serverPipe = INVALID_HANDLE_VALUE;
	std::thread m_thread;
	std::atomic<bool> m_running{false};

	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::deque<std::pair<unsigned long, std::string>> m_queue;
	std::vector<json> m_history;
	std::string m_handshakeClientId;
	json m_voiceChannelData = json::object();
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{
	const char* SkipReason = "Discord appears to be running (discord-ipc-0 is taken) - close Discord and rerun";

	// Real Discord's discord-ipc-0 pipe is transient (recreated periodically), so probing the
	// pipe races with it. Checking for the process instead is deterministic.
	bool DiscordIsRunning()
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot == INVALID_HANDLE_VALUE)
		{
			return true; // assume the worst and let the test skip
		}

		bool found = false;
		PROCESSENTRY32W entry;
		entry.dwSize = sizeof(entry);

		static const wchar_t* knownNames[] = {L"Discord.exe", L"DiscordPTB.exe", L"DiscordCanary.exe"};
		if (Process32FirstW(snapshot, &entry))
		{
			do
			{
				for (const auto* name : knownNames)
				{
					if (_wcsicmp(entry.szExeFile, name) == 0)
					{
						found = true;
						break;
					}
				}
				if (found)
				{
					break;
				}
			} while (Process32NextW(snapshot, &entry));
		}

		CloseHandle(snapshot);
		return found;
	}

	// Start the fake Discord or skip the test. Returns false (skip) if it couldn't start.
	bool StartFakeOrSkip(FakeDiscord& fake)
	{
		if (DiscordIsRunning() || !fake.Start())
		{
			ReportSkip(SkipReason);
			return false;
		}
		return true;
	}

	// Pump client.Update() until done() is true or maxIters is exhausted.
	// Returns the 1-based iteration count, or 0 on timeout.
	template <typename Done>
	int PumpUntil(DiscordBear::Client& client, const Done& done, int maxIters = 100)
	{
		for (int i = 0; i < maxIters; ++i)
		{
			client.Update();
			if (done())
			{
				return i + 1;
			}
			Sleep(20);
		}
		return 0;
	}

	json MakeVoiceChannelData()
	{
		return json{
			{"id", "1234567890"},
			{"guild_id", "0987654321"},
			{
				"voice_states",
				{
					{{"user", {{"id", "1"}, {"username", "alice"}, {"discriminator", "0001"}, {"avatar", "a1"}, {"bot", false}}}, {"nick", "alice"}, {"volume", 100}, {"mute", false}},
					{{"user", {{"id", "2"}, {"username", "bob"}, {"discriminator", "0002"}, {"avatar", "a2"}, {"bot", true}}}, {"nick", "bob"}, {"volume", 25}, {"mute", true}}
				}
			}
		};
	}

	json MakeVoiceUserData()
	{
		return json{{"user", {{"id", "42"}, {"username", "chris"}, {"discriminator", "0007"}, {"avatar", "c1"}, {"bot", false}}}, {"nick", "chris"}, {"volume", 66}, {"mute", false}};
	}

	// Client::Connect() always reaches GetAuthToken(), which does a real HTTPS call to
	// discord.com; that call fails (or throws) for the fake app ID. Everything before it
	// (registry registration, pipe connect, handshake, authorize) succeeds against the
	// fake Discord, and the pipe stays connected - which is all the dispatch tests need.
	bool ConnectForTest(DiscordBear::Client& client)
	{
		try
		{
			return client.Connect();
		}
		catch (...)
		{
			return false;
		}
	}
}

// ---------------------------------------------------------------------------
// Message
// ---------------------------------------------------------------------------

TEST(Message_RoundTrip)
{
	std::string payload = R"({"cmd":"test","n":42})";
	const auto originalLength = payload.length();

	DiscordBear::Message message(DiscordBear::Message::Opcode::Frame, std::move(payload));

	CHECK(message.IsValid());
	CHECK(message.GetHeader().opcode == DiscordBear::Message::Opcode::Frame);
	CHECK(message.GetMessageSize() == originalLength);
	CHECK(message.GetSize() == 8 + originalLength);
	CHECK(message.GetMessage() == std::string(R"({"cmd":"test","n":42})"));
}

TEST(Message_WireLayout)
{
	DiscordBear::Message message(DiscordBear::Message::Opcode::Ping, std::string("abc"));

	const auto* raw = static_cast<const unsigned char*>(message.GetData());
	unsigned long wireOpcode = 0;
	unsigned long wireLength = 0;
	memcpy(&wireOpcode, raw, 4);
	memcpy(&wireLength, raw + 4, 4);

	CHECK(wireOpcode == static_cast<unsigned long>(DiscordBear::Message::Opcode::Ping));
	CHECK(wireLength == 3);
	CHECK(memcmp(raw + 8, "abc", 3) == 0);
}

TEST(Message_BinaryPayloadPreserved)
{
	std::string payload;
	payload += 'a';
	payload += '\0';
	payload += 'b';

	DiscordBear::Message message(DiscordBear::Message::Opcode::Frame, std::move(payload));

	CHECK(message.IsValid());
	CHECK(message.GetMessage() == std::string("a\0b", 3));
}

TEST(Message_EmptyPayloadInvalid)
{
	DiscordBear::Message message(DiscordBear::Message::Opcode::Frame, std::string());

	CHECK(!message.IsValid());
}

// ---------------------------------------------------------------------------
// Pipe
// ---------------------------------------------------------------------------

TEST(Pipe_ConnectFindsFakePipe)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	CHECK(pipe.Connect());
	CHECK(pipe.Connected());
	CHECK(pipe.Disconnect());
	CHECK(!pipe.Connected());
}

TEST(Pipe_OperationsFailWhenNotConnected)
{
	DiscordBear::Pipe pipe;

	CHECK(!pipe.WriteMessage(DiscordBear::Message(DiscordBear::Message::Opcode::Frame, std::string("x"))));
	CHECK(!pipe.HasMessage());

	auto message = pipe.ReadMessage();
	CHECK(!message.IsValid());
}

TEST(Pipe_WriteMessage_SendsExactWireBytes)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	CHECK(pipe.WriteMessage(DiscordBear::Message(DiscordBear::Message::Opcode::Frame, std::string(R"({"op":"write"})"))));

	auto received = fake.WaitForMessage(std::chrono::milliseconds(5000));
	CHECK(received.has_value());
	if (received)
	{
		CHECK(received->first == static_cast<unsigned long>(DiscordBear::Message::Opcode::Frame));
		CHECK(received->second == R"({"op":"write"})");
	}
}

TEST(Pipe_ReadMessage_RoundTrip)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	CHECK(!pipe.HasMessage());

	fake.SendFrame(R"({"pong":true})");
	CHECK(pipe.HasMessage());

	auto message = pipe.ReadMessage();
	CHECK(message.IsValid());
	CHECK(message.GetHeader().opcode == DiscordBear::Message::Opcode::Frame);
	CHECK(message.GetMessage() == std::string(R"({"pong":true})"));

	pipe.Disconnect();
}

TEST(Pipe_ReadMessage_TwoFramesInOneRead)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	// Both frames land in the client's buffer together; each read must return exactly one.
	fake.SendRaw(EncodeFrame(1 /*Frame*/, R"({"n":1})") + EncodeFrame(1 /*Frame*/, R"({"n":2,"more":true})"));

	auto first = pipe.ReadMessage();
	CHECK(first.IsValid());
	CHECK(first.GetMessage() == std::string(R"({"n":1})"));

	CHECK(pipe.HasMessage());
	auto second = pipe.ReadMessage();
	CHECK(second.IsValid());
	CHECK(second.GetMessage() == std::string(R"({"n":2,"more":true})"));

	CHECK(!pipe.HasMessage());

	pipe.Disconnect();
}

TEST(Pipe_ReadMessage_SplitFrame)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	// Split mid-payload: the reader sees a full header but only part of the body, and must wait for the rest.
	const std::string payload = R"({"split":"across two writes"})";
	const std::string frame = EncodeFrame(1 /*Frame*/, payload);
	const size_t splitAt = 8 + payload.size() / 2;

	fake.SendRaw(frame.substr(0, splitAt));
	std::thread rest(
		[&]
			{
				Sleep(100);
				fake.SendRaw(frame.substr(splitAt));
			});

	auto message = pipe.ReadMessage();
	rest.join();

	CHECK(message.IsValid());
	CHECK(message.GetMessageSize() == payload.size());
	CHECK(message.GetMessage() == payload);
	CHECK(!pipe.HasMessage());

	pipe.Disconnect();
}

TEST(Pipe_Disconnect_RejectsSecondDisconnect)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	CHECK(pipe.Disconnect());
	CHECK(!pipe.Disconnect());
}

TEST(Pipe_HasMessage_DetectsClosedPeer)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	// Discord quitting, or restarting for an update, closes its end of the pipe.
	fake.Stop();

	CHECK(!pipe.HasMessage());
	CHECK(!pipe.Connected());

	// The dead handle is still owned: Disconnect() releases it, but there was no live connection to report.
	CHECK(!pipe.Disconnect());
}

TEST(Pipe_ReadMessage_TimesOutWithoutData)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	const auto start = std::chrono::steady_clock::now();
	auto message = pipe.ReadMessage(std::chrono::milliseconds(200));
	const auto elapsed = std::chrono::steady_clock::now() - start;

	CHECK(!message.IsValid());
	CHECK(elapsed >= std::chrono::milliseconds(150));
	CHECK(elapsed < std::chrono::seconds(5));

	// Nothing was consumed while waiting, so the stream is still good and a late reply reads fine.
	CHECK(pipe.Connected());
	const std::string payload = R"({"late":"reply"})";
	fake.SendFrame(payload);
	auto late = pipe.ReadMessage(std::chrono::seconds(2));
	CHECK(late.IsValid());
	CHECK(late.GetMessage() == payload);

	pipe.Disconnect();
}

TEST(Pipe_ReadMessage_BreaksWhenPeerCloses)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	std::thread closer([&]
	{
		Sleep(100);
		fake.Stop();
	});

	// Used to spin forever once the pipe was gone.
	auto message = pipe.ReadMessage(std::chrono::seconds(10));
	closer.join();

	CHECK(!message.IsValid());
	CHECK(!pipe.Connected());
}

TEST(Pipe_ReadMessage_StopsWhenCancelled)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	std::atomic<bool> cancel{ false };
	DiscordBear::Pipe pipe;
	if (!pipe.Connect(&cancel))
	{
		CHECK(false);
		return;
	}

	std::thread canceller([&]
	{
		Sleep(100);
		cancel = true;
	});

	const auto start = std::chrono::steady_clock::now();
	auto message = pipe.ReadMessage(std::chrono::seconds(30));
	const auto elapsed = std::chrono::steady_clock::now() - start;
	canceller.join();

	CHECK(!message.IsValid());
	CHECK(elapsed < std::chrono::seconds(5));

	// Cancelling is not a failure, the pipe is still usable.
	CHECK(pipe.Connected());
	pipe.Disconnect();
}

TEST(Pipe_ReadMessage_OversizedLengthBreaksPipe)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Pipe pipe;
	if (!pipe.Connect())
	{
		CHECK(false);
		return;
	}

	// A header announcing a huge payload means the stream is out of sync, we must not try to allocate or wait for it.
	std::string header(8, '\0');
	const unsigned long opcode = 1;
	const unsigned long length = 0x7FFFFFFF;
	memcpy(&header[0], &opcode, 4);
	memcpy(&header[4], &length, 4);
	fake.SendRaw(header);

	auto message = pipe.ReadMessage(std::chrono::seconds(2));
	CHECK(!message.IsValid());
	CHECK(!pipe.Connected());
}

// ---------------------------------------------------------------------------
// Client
// ---------------------------------------------------------------------------

TEST(Client_Connect_HandshakeReachesServer)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client); // expected to fail at the OAuth network step

	auto handshake = fake.WaitForMessage(std::chrono::milliseconds(5000));
	CHECK(handshake.has_value());
	if (handshake)
	{
		CHECK(handshake->first == static_cast<unsigned long>(DiscordBear::Message::Opcode::Handshake));
		CHECK(fake.HandshakeClientId() == "test-app-id-000");
	}
}

TEST(Client_GetSelectedVoiceChannel_DispatchesParsedInfo)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	fake.SetVoiceChannelData(MakeVoiceChannelData());

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	DiscordBear::VoiceChannelInfo received;
	bool got = false;
	client.GetSelectedVoiceChannel(
		[&](DiscordBear::VoiceChannelInfo&& info)
			{
				received = std::move(info);
				got = true;
			});

	// Blocks until the server received the request (its GET_CHANNEL reply is already in the client's buffer).
	CHECK(fake.WaitForCommand("GET_SELECTED_VOICE_CHANNEL", "", std::chrono::milliseconds(5000)));

	PumpUntil(client, [&] { return got; });

	CHECK(got);
	if (got)
	{
		CHECK(received.valid);
		CHECK(received.channelID == "1234567890");
		CHECK(received.serverID == "0987654321");
		CHECK(received.voiceUserList.size() == 2);
		if (received.voiceUserList.size() == 2)
		{
			CHECK(received.voiceUserList[0].userID == "1");
			CHECK(received.voiceUserList[0].globalUsername == "alice");
			CHECK(received.voiceUserList[0].serverNickname == "alice");
			CHECK(received.voiceUserList[0].volume == 100);
			CHECK(received.voiceUserList[0].isMuted == false);
			CHECK(received.voiceUserList[0].isBot == false);
			CHECK(received.voiceUserList[1].userID == "2");
			CHECK(received.voiceUserList[1].isBot == true);
			CHECK(received.voiceUserList[1].volume == 25);
			CHECK(received.voiceUserList[1].isMuted == true);
		}
	}
}

TEST(Client_VoiceChannelJoin_Dispatch)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	DiscordBear::GenericChannelInfo received;
	bool got = false;
	client.SubscribeOnVoiceChannelJoin(
		[&](DiscordBear::GenericChannelInfo&& info)
			{
				received = std::move(info);
				got = true;
			});

	CHECK(fake.WaitForCommand("SUBSCRIBE", "VOICE_CHANNEL_SELECT", std::chrono::milliseconds(5000)));

	fake.SendFrame(json{{"cmd", "DISPATCH"}, {"nonce", "join-1"}, {"evt", "VOICE_CHANNEL_SELECT"}, {"data", {{"channel_id", "111"}, {"guild_id", "222"}}}}.dump());

	PumpUntil(client, [&] { return got; });

	CHECK(got);
	if (got)
	{
		CHECK(received.valid);
		CHECK(received.channelID == "111");
		CHECK(received.serverID == "222");
	}
}

TEST(Client_SpeakingEvents_Dispatch)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	DiscordBear::SpeakingEventInfo started;
	DiscordBear::SpeakingEventInfo stopped;
	bool gotStart = false;
	bool gotStop = false;
	client.SubscribeOnVoiceChannelSpeak(
		{"ch1"},
		[&](DiscordBear::SpeakingEventInfo&& info)
			{
				started = std::move(info);
				gotStart = true;
			},
		[&](DiscordBear::SpeakingEventInfo&& info)
			{
				stopped = std::move(info);
				gotStop = true;
			});

	CHECK(fake.WaitForCommand("SUBSCRIBE", "SPEAKING_START", std::chrono::milliseconds(5000)));
	CHECK(fake.WaitForCommand("SUBSCRIBE", "SPEAKING_STOP", std::chrono::milliseconds(5000)));

	fake.SendFrame(json{{"cmd", "DISPATCH"}, {"nonce", "spk-1"}, {"evt", "SPEAKING_START"}, {"data", {{"channel_id", "ch1"}, {"user_id", "u1"}}}}.dump());
	PumpUntil(client, [&] { return gotStart; });

	fake.SendFrame(json{{"cmd", "DISPATCH"}, {"nonce", "spk-2"}, {"evt", "SPEAKING_STOP"}, {"data", {{"channel_id", "ch1"}, {"user_id", "u1"}}}}.dump());
	PumpUntil(client, [&] { return gotStop; });

	CHECK(gotStart);
	CHECK(gotStop);
	if (gotStart && gotStop)
	{
		CHECK(started.valid);
		CHECK(started.userID == "u1");
		CHECK(started.channelID == "ch1");
		CHECK(stopped.valid);
		CHECK(stopped.userID == "u1");
	}
}

TEST(Client_Update_DispatchesBackToBackFrames)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	bool gotStart = false;
	bool gotStop = false;
	client.SubscribeOnVoiceChannelSpeak({"ch1"}, [&](DiscordBear::SpeakingEventInfo&&) { gotStart = true; }, [&](DiscordBear::SpeakingEventInfo&&) { gotStop = true; });

	CHECK(fake.WaitForCommand("SUBSCRIBE", "SPEAKING_START", std::chrono::milliseconds(5000)));
	CHECK(fake.WaitForCommand("SUBSCRIBE", "SPEAKING_STOP", std::chrono::milliseconds(5000)));

	// Discord can deliver several events before the client next reads; neither may be dropped.
	fake.SendRaw(
		EncodeFrame(1 /*Frame*/, json{{"cmd", "DISPATCH"}, {"nonce", "b2b-1"}, {"evt", "SPEAKING_START"}, {"data", {{"channel_id", "ch1"}, {"user_id", "u1"}}}}.dump()) + EncodeFrame(
			1 /*Frame*/,
			json{{"cmd", "DISPATCH"}, {"nonce", "b2b-2"}, {"evt", "SPEAKING_STOP"}, {"data", {{"channel_id", "ch1"}, {"user_id", "u1"}}}}.dump()));

	PumpUntil(client, [&] { return gotStart && gotStop; });

	CHECK(gotStart);
	CHECK(gotStop);
}

TEST(Client_VoiceStateEnterExit_Dispatch)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	DiscordBear::VoiceUserInfo entered;
	DiscordBear::VoiceUserInfo exited;
	bool gotEnter = false;
	bool gotExit = false;
	client.SubscribeOnVoiceChannelEnter(
		{"ch1"},
		[&](DiscordBear::VoiceUserInfo&& info)
			{
				entered = std::move(info);
				gotEnter = true;
			});
	client.SubscribeOnVoiceChannelExit(
		{"ch1"},
		[&](DiscordBear::VoiceUserInfo&& info)
			{
				exited = std::move(info);
				gotExit = true;
			});

	CHECK(fake.WaitForCommand("SUBSCRIBE", "VOICE_STATE_CREATE", std::chrono::milliseconds(5000)));
	CHECK(fake.WaitForCommand("SUBSCRIBE", "VOICE_STATE_DELETE", std::chrono::milliseconds(5000)));

	fake.SendFrame(json{{"cmd", "DISPATCH"}, {"nonce", "vs-1"}, {"evt", "VOICE_STATE_CREATE"}, {"data", MakeVoiceUserData()}}.dump());
	PumpUntil(client, [&] { return gotEnter; });

	fake.SendFrame(json{{"cmd", "DISPATCH"}, {"nonce", "vs-2"}, {"evt", "VOICE_STATE_DELETE"}, {"data", MakeVoiceUserData()}}.dump());
	PumpUntil(client, [&] { return gotExit; });

	CHECK(gotEnter);
	CHECK(gotExit);
	if (gotEnter && gotExit)
	{
		CHECK(entered.valid);
		CHECK(entered.userID == "42");
		CHECK(entered.globalUsername == "chris");
		CHECK(entered.serverNickname == "chris");
		CHECK(entered.volume == 66);
		CHECK(entered.isMuted == false);
		CHECK(entered.isBot == false);
		CHECK(exited.valid);
		CHECK(exited.userID == "42");
	}
}

TEST(Client_Unsubscribe_GuardsAgainstDoubleUnsubscribeAndResubscribe)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	auto noEvent = [](auto&&) {};

	client.SubscribeOnVoiceChannelJoin(noEvent);
	client.UnsubscribeOnVoiceChannelJoin();
	client.UnsubscribeOnVoiceChannelJoin(); // must be a no-op (handler already cleared)
	CHECK(fake.WaitForCommand("SUBSCRIBE", "VOICE_CHANNEL_SELECT", std::chrono::milliseconds(5000)));
	CHECK(fake.WaitForCommand("UNSUBSCRIBE", "VOICE_CHANNEL_SELECT", std::chrono::milliseconds(5000)));
	Sleep(100); // let the poller pick up any late frames before counting
	CHECK(fake.CountCommand("SUBSCRIBE", "VOICE_CHANNEL_SELECT") == 1);
	CHECK(fake.CountCommand("UNSUBSCRIBE", "VOICE_CHANNEL_SELECT") == 1);

	client.SubscribeOnVoiceChannelSpeak({"ch1"}, noEvent, noEvent);
	client.SubscribeOnVoiceChannelSpeak({"ch2"}, noEvent, noEvent); // must be ignored (already subscribed)
	CHECK(fake.WaitForCommand("SUBSCRIBE", "SPEAKING_START", std::chrono::milliseconds(5000)));
	client.UnsubscribeOnVoiceChannelSpeak();
	client.UnsubscribeOnVoiceChannelSpeak(); // must be a no-op (channel list already cleared)
	CHECK(fake.WaitForCommand("UNSUBSCRIBE", "SPEAKING_START", std::chrono::milliseconds(5000)));
	CHECK(fake.WaitForCommand("UNSUBSCRIBE", "SPEAKING_STOP", std::chrono::milliseconds(5000)));
	Sleep(100); // let the poller pick up any late frames before counting
	CHECK(fake.CountCommand("SUBSCRIBE", "SPEAKING_START") == 1);
	CHECK(fake.CountCommand("SUBSCRIBE", "SPEAKING_STOP") == 1);
	CHECK(fake.CountCommand("UNSUBSCRIBE", "SPEAKING_START") == 1);
	CHECK(fake.CountCommand("UNSUBSCRIBE", "SPEAKING_STOP") == 1);
}

TEST(Client_Update_IgnoresUnknownNonce)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	bool got = false;
	client.GetSelectedVoiceChannel(
		[&](DiscordBear::VoiceChannelInfo&&)
			{
				got = true;
			});

	// The fake auto-replies to the request above (matching nonce) - that must fire the callback.
	CHECK(fake.WaitForCommand("GET_SELECTED_VOICE_CHANNEL", "", std::chrono::milliseconds(5000)));
	PumpUntil(client, [&] { return got; });
	CHECK(got);

	// A reply for a nonce we never sent: Update must not crash and must not fire the callback again.
	got = false;
	fake.SendFrame(json{{"cmd", "GET_CHANNEL"}, {"nonce", "unknown-nonce"}, {"data", json::object()}}.dump());

	PumpUntil(client, [&] { return got; }, 25); // ~0.5s: enough time to surface a spurious dispatch

	CHECK(!got);
}

TEST(Client_Disconnect_StopsAsyncWorker)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	std::atomic<int> callbacks{ 0 }; // declared before the client so it outlives the worker
	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	client.GetSelectedVoiceChannel([&](DiscordBear::VoiceChannelInfo&&)
	{
		++callbacks;
	});
	client.UpdateAsync();

	for (int i = 0; i < 250 && callbacks == 0; ++i)
	{
		Sleep(20);
	}
	CHECK(callbacks == 1);

	// Disconnect() has to wait for the worker before closing the pipe, and must not hang doing so.
	const auto start = std::chrono::steady_clock::now();
	client.Disconnect();
	CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(5));
}

TEST(Client_Disconnect_FromCallbackDoesNotDeadlock)
{
	FakeDiscord fake;
	if (!StartFakeOrSkip(fake))
	{
		return;
	}

	std::atomic<bool> done{ false };
	DiscordBear::Client client("test-app-id-000", "test-secret");
	ConnectForTest(client);

	// Callbacks run on the async worker and Disconnect() waits for that worker, so it must not wait on itself.
	client.GetSelectedVoiceChannel([&](DiscordBear::VoiceChannelInfo&&)
	{
		client.Disconnect();
		done = true;
	});
	client.UpdateAsync();

	for (int i = 0; i < 250 && !done; ++i)
	{
		Sleep(20);
	}
	CHECK(done);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
	// Print only severe logs; they surface real problems in a failing test.
	DiscordBear::LogFn = [](LogSeverity severity, const char* message)
		{
			if (severity >= LogSeverity::Severe)
			{
				std::cout << "[log] " << message << '\n';
			}
		};

	for (const auto& test : TestRegistry())
	{
		g_currentTest = test.name;
		std::cout << "[ RUN  ] " << test.name << '\n';
		test.fn();
	}

	std::cout << "=== " << TestRegistry().size() << " tests, " << g_checks << " checks, " << g_failures << " failures, " << g_skips << " skipped ===" << '\n';

	return g_failures == 0 ? 0 : 1;
}