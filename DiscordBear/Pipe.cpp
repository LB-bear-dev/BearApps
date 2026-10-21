#include "PCH.h"
#include "Pipe.h"
#include "Message.h"
#include "Log.h"

namespace DiscordBear
{
	namespace
	{
		// Largest frame we accept; anything bigger means the stream is out of sync.
		constexpr unsigned long MaxFrameLength = 16 * 1024 * 1024;

		// Once a frame has started arriving, how long we wait for the rest of it before giving up on the stream.
		constexpr auto FrameTimeout = std::chrono::seconds(5);
		constexpr auto PollInterval = std::chrono::milliseconds(10);

		enum class ReadResult
		{
			Ok,
			Timeout,
			Cancelled,
			Broken
		};

		// Waits until at least one byte is buffered, without consuming it. Polls instead of blocking in ReadFile,
		// a blocked ReadFile on this synchronous handle would also hold up WriteFile from other threads.
		ReadResult WaitForData(HANDLE pipe, std::chrono::milliseconds timeout, DWORD& available, const std::atomic<bool>* cancel)
		{
			const auto deadline = std::chrono::steady_clock::now() + timeout;
			for (;;)
			{
				if (cancel != nullptr && *cancel)
				{
					return ReadResult::Cancelled;
				}

				available = 0;
				if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr))
				{
					LOG(LogSeverity::Severe) << "Pipe broke while waiting for data, error code: " << GetLastError();
					return ReadResult::Broken;
				}

				if (available != 0)
				{
					return ReadResult::Ok;
				}

				if (std::chrono::steady_clock::now() >= deadline)
				{
					return ReadResult::Timeout;
				}

				std::this_thread::sleep_for(PollInterval);
			}
		}

		// A pipe read can return fewer bytes than asked for, so keep reading until we have them all.
		// timeout is how long we wait for more bytes to show up, not for the whole read.
		ReadResult ReadExact(HANDLE pipe, char* buffer, DWORD size, std::chrono::milliseconds timeout)
		{
			DWORD total = 0;
			while (total < size)
			{
				DWORD available = 0;
				// Mid-frame, so never cancel here: stopping now would leave a half-read frame in the stream.
				const auto waited = WaitForData(pipe, timeout, available, nullptr);
				if (waited != ReadResult::Ok)
				{
					return waited;
				}

				const DWORD remaining = size - total;
				const DWORD toRead = available < remaining ? available : remaining;
				DWORD read = 0;
				if (!ReadFile(pipe, buffer + total, toRead, &read, nullptr))
				{
					LOG(LogSeverity::Severe) << "Did not successfully recieve message, error code: " << GetLastError();
					return ReadResult::Broken;
				}
				total += read;
			}
			return ReadResult::Ok;
		}
	}

	bool Pipe::Connect(const std::atomic<bool>* cancel)
	{
		m_cancel = cancel;

		// When reconnecting we still own the dead pipe's handle, release it first.
		CloseIfOpen();

		// Discord may start, or auto-restart (updates), after us. Wait for a pipe to appear.
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
		bool firstScan = true;
		for (;;)
		{
			if (cancel != nullptr && *cancel)
			{
				LOG(LogSeverity::Info) << "Pipe connect cancelled";
				return false;
			}

			auto result = ScanForPipe();
			if (result == ScanResult::Found)
			{
				return true;
			}

			if (result == ScanResult::Error)
			{
				return false;
			}

			if (std::chrono::steady_clock::now() >= deadline)
			{
				LOG(LogSeverity::Severe) << "Could not find any discord-ipc pipe after 60 seconds, is discord running?";
				m_connected = false;
				return false;
			}

			if (firstScan)
			{
				LOG(LogSeverity::Info) << "No discord-ipc pipe found yet, waiting for discord to start...";
				firstScan = false;
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	Pipe::ScanResult Pipe::ScanForPipe()
	{
		wchar_t pipeName[]{L"\\\\?\\pipe\\discord-ipc-0"};
		const unsigned long pipeDigit = sizeof(pipeName) / sizeof(wchar_t) - 2;
		pipeName[pipeDigit] = L'0';
		for (;;)
		{
			LOG(LogSeverity::Info) << "Trying connection to pipe \\\\.\\pipe\\discord-ipc-" << (char)pipeName[pipeDigit];
			m_pipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

			if (m_pipe != INVALID_HANDLE_VALUE)
			{
				LOG(LogSeverity::Info) << "Connected to pipe \\\\.\\pipe\\discord-ipc-" << (char)pipeName[pipeDigit];
				m_connected = true;
				return ScanResult::Found;
			}

			auto lastError = GetLastError();
			if (lastError == ERROR_FILE_NOT_FOUND)
			{
				LOG(LogSeverity::NonSevere) << "Could not find pipe at \\\\.\\pipe\\discord-ipc-" << (char)pipeName[pipeDigit];
				if (pipeName[pipeDigit] < L'9')
				{
					pipeName[pipeDigit]++;
					continue;
				}

				return ScanResult::NotFound;
			}
			else if (lastError == ERROR_PIPE_BUSY)
			{
				LOG(LogSeverity::NonSevere) << "Pipe at \\\\.\\pipe\\discord-ipc-" << (char)pipeName[pipeDigit] << " is busy";
				if (!WaitNamedPipeW(pipeName, 10000))
				{
					LOG(LogSeverity::Severe) << "Pipe at \\\\.\\pipe\\discord-ipc-" << (char)pipeName[pipeDigit] << " is busy and could not be reached after wait timeout. Connection rejected";
					m_connected = false;
					return ScanResult::Error;
				}
				continue;
			}

			LOG(LogSeverity::Severe) << "Could not connect to pipe \\\\.\\pipe\\discord-ipc-" << (char)pipeName[pipeDigit] << ", unknown issue";
			m_connected = false;
			return ScanResult::Error;
		}
	}

	bool Pipe::Connected()
	{
		return m_connected;
	}

	bool Pipe::Disconnect()
	{
		const bool wasConnected = m_connected.exchange(false);
		if (wasConnected)
		{
			LOG(LogSeverity::Info) << "Disconnecting pipe";
		}
		else
		{
			LOG(LogSeverity::Verbose) << "pipe already disconnected";
		}

		// A broken pipe is already marked disconnected but still owns its handle, so always close it.
		CloseIfOpen();
		return wasConnected;
	}

	void Pipe::MarkBroken()
	{
		LOG(LogSeverity::Severe) << "Pipe is no longer usable, it needs to be reconnected";
		m_connected = false;
	}

	void Pipe::CloseIfOpen()
	{
		if (m_pipe != nullptr && m_pipe != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_pipe);
		}

		m_pipe = nullptr;
	}

	bool Pipe::WriteMessage(const Message&& message)
	{
		if (!Connected())
		{
			LOG(LogSeverity::Severe) << "Pipe is not connected, message will not be sent";
			return false;
		}

		// Frames can carry the client secret, auth code and access token, so only log opcode and size.
		LOG(LogSeverity::Verbose) << "Attempting to write message with opcode " << static_cast<unsigned long>(message.GetHeader().opcode) << ", " << message.GetMessageSize() << " bytes";

		DWORD cbWrite = 0;
		auto success = WriteFile(m_pipe, message.GetData(), message.GetSize(), &cbWrite, nullptr);

		if (!success)
		{
			LOG(LogSeverity::Severe) << "Message not sent successfully, error code: " << GetLastError();
		}
		else
		{
			LOG(LogSeverity::Verbose) << "Message successfully sent";
		}

		return success;
	}

	bool Pipe::HasMessage()
	{
		if (!Connected())
		{
			LOG(LogSeverity::Severe) << "Pipe is not connected, cannot check if the pipe has messages";
			return false;
		}

		DWORD bytesToRead = 0;
		if (!PeekNamedPipe(m_pipe, nullptr, 0, nullptr, &bytesToRead, nullptr))
		{
			// Discord closed the pipe (quit, or restarted for an update), nothing more will ever arrive on this handle.
			LOG(LogSeverity::Severe) << "Pipe broke while checking for messages, error code: " << GetLastError();
			MarkBroken();
			return false;
		}

		if (bytesToRead != 0)
		{
			LOG(LogSeverity::Verbose) << "Pipe has message";
			return true;
		}

		return false;
	}

	Message Pipe::ReadMessage(std::chrono::milliseconds timeout)
	{
		if (!Connected())
		{
			LOG(LogSeverity::Severe) << "Pipe is not connected, cannot read a message";
			return Message(0ul);
		}

		LOG(LogSeverity::Verbose) << "Attempting to read message from pipe";

		// Wait for the reply to start arriving. Nothing has been consumed yet, so timing out here leaves the stream intact.
		DWORD available = 0;
		const auto waited = WaitForData(m_pipe, timeout, available, m_cancel);
		if (waited == ReadResult::Timeout)
		{
			LOG(LogSeverity::Severe) << "Timed out waiting for a message from the pipe";
			return Message(0ul);
		}

		if (waited == ReadResult::Cancelled)
		{
			LOG(LogSeverity::Info) << "Stopped waiting for a message from the pipe";
			return Message(0ul);
		}

		if (waited == ReadResult::Broken)
		{
			MarkBroken();
			return Message(0ul);
		}

		// The pipe is a byte stream: it can hold several frames, or only part of one.
		// Read the header first, then exactly the payload length it announces, so each call returns one frame.
		// From here a failure leaves a half-read frame behind, so the stream can't be trusted any more.
		Message::Header header{};
		if (ReadExact(m_pipe, reinterpret_cast<char*>(&header), sizeof(header), FrameTimeout) != ReadResult::Ok)
		{
			MarkBroken();
			return Message(0ul);
		}

		if (header.length > MaxFrameLength)
		{
			LOG(LogSeverity::Severe) << "Message header has invalid length " << header.length << ", the stream is out of sync";
			MarkBroken();
			return Message(0ul);
		}

		if (header.length == 0)
		{
			LOG(LogSeverity::NonSevere) << "Dropping empty message";
			return Message(0ul);
		}

		Message message(static_cast<unsigned long>(sizeof(header)) + header.length);
		memcpy(message.m_packedData.get(), &header, sizeof(header));

		if (ReadExact(m_pipe, message.m_packedData.get() + sizeof(header), header.length, FrameTimeout) != ReadResult::Ok)
		{
			MarkBroken();
			return Message(0ul);
		}

		LOG(LogSeverity::Verbose) << "Read message from pipe, " << message.GetSize() << " bytes";
		return message;
	}
}