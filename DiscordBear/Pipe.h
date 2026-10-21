#pragma once
#include <atomic>
#include <chrono>

namespace DiscordBear
{
	class Message;

	class Pipe final
	{
	public:
		Pipe() : m_pipe(nullptr), m_connected(false), m_cancel(nullptr) {}
		Pipe(const Pipe&) = delete;
		Pipe(const Pipe&&) = delete;

		// Waits for discord to open a pipe. Stops early if *cancel becomes true.
		bool Connect(const std::atomic<bool>* cancel = nullptr);
		bool Connected();
		bool Disconnect();
		bool WriteMessage(const Message&& message);
		bool HasMessage();
		// Waits up to timeout for a reply to start arriving. Returns an invalid message on timeout, cancel or error.
		// If the pipe breaks Connected() turns false.
		Message ReadMessage(std::chrono::milliseconds timeout = std::chrono::seconds(10));

	private:
		enum class ScanResult
		{
			Found,
			NotFound,
			Error
		};

		ScanResult ScanForPipe();

		// The pipe stays owned (and its handle open) until Disconnect() or the next Connect(), so a broken pipe
		// only flips the connected flag and the owner can notice and reconnect.
		void MarkBroken();
		void CloseIfOpen();

		HANDLE m_pipe;
		std::atomic<bool> m_connected;
		// The flag given to Connect(), also ends the wait for a reply in ReadMessage() so Disconnect() isn't held up.
		const std::atomic<bool>* m_cancel;
	};
}