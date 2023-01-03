#pragma once
namespace DiscordBear
{
	class Message final
	{
	public:
		enum class Opcode : unsigned long
		{
			Handshake = 0,
			Frame = 1,
			Close = 2,
			Ping = 3,
			Pong = 4,
		};

		struct Header
		{
			Opcode opcode;
			unsigned long length;
		};

		Message(Opcode opCode, std::string&& message);

		unsigned long GetSize() const;
		unsigned long GetMessageSize() const;

		const Header* GetHeader() const;
		std::string GetMessage() const;
		const void* GetData() const;

		void Clear();
		bool IsValid();

	private:
		std::unique_ptr<char[]> m_packedData;
		unsigned long m_byteSize;
		unsigned long m_headerSize;

		//Internal use only for pipe message read
		friend class Pipe;
		Message(unsigned long byteSize);
	};
}