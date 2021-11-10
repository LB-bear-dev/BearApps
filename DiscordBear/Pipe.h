#pragma once

namespace DiscordBear
{
	class Message;

	class Pipe final
	{
	public:
		Pipe() : m_pipe(nullptr), m_connected(false) {}
		Pipe(const Pipe&) = delete;
		Pipe(const Pipe&&) = delete;

		bool Connect();
		bool Connected();
		bool Disconnect();
		bool WriteMessage(const Message&& message);
		bool HasMessage();
		Message ReadMessage();
	private:
		HANDLE m_pipe;
		std::atomic<bool> m_connected;
	};
}