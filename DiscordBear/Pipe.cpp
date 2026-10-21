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

	// A pipe read can return fewer bytes than asked for, so keep reading until we have them all.
	// Blocks until the bytes arrive, and fails if the pipe breaks.
	bool ReadExact(HANDLE pipe, char* buffer, DWORD size)
	{
		DWORD total = 0;
		while (total < size)
		{
			DWORD read = 0;
			if (!ReadFile(pipe, buffer + total, size - total, &read, nullptr))
			{
				LOG(LogSeverity::Severe) << "Did not successfully recieve message, error code: " << GetLastError();
				return false;
			}
			total += read;
		}
		return true;
	}
}

	bool Pipe::Connect()
	{
		// Discord may start, or auto-restart (updates), after us. Wait for a pipe to appear.
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
		bool firstScan = true;
		for (;;)
		{
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
		wchar_t pipeName[]{ L"\\\\?\\pipe\\discord-ipc-0" };
		const unsigned long pipeDigit = sizeof(pipeName) / sizeof(wchar_t) - 2;
		pipeName[pipeDigit] = L'0';
		for (;;)
		{
			LOG(LogSeverity::Info) << "Trying connection to pipe \\\\.\\pipe\\discord-ipc-" << (char)pipeName[pipeDigit];
			m_pipe = CreateFileW(
				pipeName,
				GENERIC_READ | GENERIC_WRITE,
				0,
				nullptr,
				OPEN_EXISTING,
				0,
				nullptr);

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
		if (!Connected())
		{
			LOG(LogSeverity::NonSevere) << "pipe already disconnected, aborting";
			return false;
		}

		LOG(LogSeverity::Info) << "Disconnecting pipe";
		CloseHandle(m_pipe);
		m_connected = false;
		return true;
	}

	bool Pipe::WriteMessage(const Message&& message)
	{
		if (!Connected())
		{
			LOG(LogSeverity::Severe) << "Pipe is not connected, message will not be sent";
			return false;
		}

		// Frames can carry the client secret, auth code and access token, so only log opcode and size.
		LOG(LogSeverity::Verbose) << "Attempting to write message with opcode "
			<< static_cast<unsigned long>(message.GetHeader()->opcode) << ", " << message.GetMessageSize() << " bytes";

		DWORD cbWrite = 0;
		auto success = WriteFile(
			m_pipe,
			message.GetData(),
			message.GetSize(),
			&cbWrite,
			nullptr);

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
		PeekNamedPipe(
			m_pipe,
			nullptr,
			0,
			nullptr,
			&bytesToRead,
			nullptr);

		if (bytesToRead != 0)
		{
			LOG(LogSeverity::Verbose) << "Pipe has message";
			return true;
		}

		return false;
	}

	Message Pipe::ReadMessage()
	{
		if (!Connected())
		{
			LOG(LogSeverity::Severe) << "Pipe is not connected, cannot check if the pipe has messages";
			return false;
		}

		// The pipe is a byte stream: it can hold several frames, or only part of one.
		// Read the header first, then exactly the payload length it announces, so each call returns one frame.
		LOG(LogSeverity::Verbose) << "Attempting to read message from pipe";
		Message::Header header{};
		if (!ReadExact(m_pipe, reinterpret_cast<char*>(&header), sizeof(header)))
		{
			return false;
		}

		if (header.length == 0 || header.length > MaxFrameLength)
		{
			LOG(LogSeverity::Severe) << "Message header has invalid length " << header.length << ", dropping it";
			return false;
		}

		Message message(static_cast<unsigned long>(sizeof(header)) + header.length);
		memcpy(message.m_packedData.get(), &header, sizeof(header));

		if (!ReadExact(m_pipe, message.m_packedData.get() + sizeof(header), header.length))
		{
			message.Clear();
			return message;
		}

		LOG(LogSeverity::Verbose) << "Read message from pipe, " << message.GetSize() << " bytes";
		return message;
	}
}
