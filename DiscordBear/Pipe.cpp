#include "PCH.h"
#include "Pipe.h"
#include "Message.h"
#include "Log.h"
namespace DiscordBear
{
	bool Pipe::Connect()
	{
		wchar_t pipeName[]{ L"\\\\?\\pipe\\discord-ipc-0" };
		const unsigned long pipeDigit = sizeof(pipeName) / sizeof(wchar_t) - 2;
		pipeName[pipeDigit] = L'0';
		for (;;)
		{
			LOG(LogSeverity::Info) << "Trying connection to pipe " << pipeName;
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
				LOG(LogSeverity::Info) << "Connected to pipe " << pipeName;
				m_connected = true;
				return m_connected;
			}

			auto lastError = GetLastError();
			if (lastError == ERROR_FILE_NOT_FOUND)
			{
				LOG(LogSeverity::NonSevere) << "Could not find pipe at " << pipeName;
				if (pipeName[pipeDigit] < L'9') {
					pipeName[pipeDigit]++;
					continue;
				}
			}
			else if (lastError == ERROR_PIPE_BUSY)
			{
				LOG(LogSeverity::NonSevere) << "Pipe at " << pipeName << " is busy";
				if (!WaitNamedPipeW(pipeName, 10000))
				{
					LOG(LogSeverity::Severe) << "Pipe at " << pipeName << " is busy and could not be reached after wait timeout. Connection rejected";
					m_connected = false;
					return m_connected;
				}
				continue;
			}

			LOG(LogSeverity::Severe) << "Could not connect to pipe " << pipeName << ", unknown issue";
			m_connected = false;
			return m_connected;
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
			LOG(LogSeverity::Severe) << "Pipe is not connected, message " << message.GetMessage() << " will not be sent";
			return false;
		}

		LOG(LogSeverity::Verbose) << "Attempting to write message "
#if defined(_DEBUG)
			<< message.GetMessage();
#else
			;
#endif

		DWORD cbWrite = 0;
		auto success = WriteFile(
			m_pipe,
			message.GetData(),
			message.GetSize(),
			&cbWrite,
			nullptr);

		if (!success)
		{
			LOG(LogSeverity::Severe) << "Message " << message.GetMessage() << " not sent successfully, error code: " << GetLastError();
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

		DWORD bytesToRead = 0;

		while (bytesToRead == 0)
		{
			PeekNamedPipe(
				m_pipe,
				nullptr,
				0,
				nullptr,
				&bytesToRead,
				nullptr);

			static constexpr auto sleepTime = std::chrono::milliseconds(10);
			std::this_thread::sleep_for(sleepTime);
		}

		Message message(bytesToRead+1);

		LOG(LogSeverity::Verbose) << "Attempting to read message from pipe";
		DWORD cbRead = 0;
		auto success = ReadFile(
			m_pipe,
			message.m_packedData.get(),
			message.m_byteSize,
			&cbRead,
			nullptr);

		((char*)message.GetData())[bytesToRead] = 0;

		if (!success)
		{
			LOG(LogSeverity::Severe) << "Did not successfully recieve message, error code: " << GetLastError();
			message.Clear();
		}
		else
		{
			LOG(LogSeverity::Verbose) << "Read message from pipe " 
#if defined(_DEBUG)
				<< message.GetMessage();
#else
				;
#endif
		}

		return message;
	}
}