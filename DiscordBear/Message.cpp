#include "PCH.h"
#include "Message.h"

namespace DiscordBear
{
	Message::Message(Message::Opcode opCode, std::string&& message)
	{
		Header header;
		header.opcode = opCode;
		header.length = static_cast<unsigned long>(message.length());

		m_byteSize = sizeof(Header) + header.length;
		m_packedData = std::unique_ptr<char[]>(new char[m_byteSize]);

		memcpy(m_packedData.get(), &header, sizeof(Header));
		memcpy(m_packedData.get() + sizeof(Header), message.c_str(), message.length());
	}

	Message::Message(unsigned long byteSize)
	{
		m_byteSize = byteSize;
		m_packedData = std::unique_ptr<char[]>(new char[m_byteSize]);
	}

	unsigned long Message::GetSize() const
	{
		return m_byteSize;
	}

	unsigned long Message::GetMessageSize() const
	{
		return ((Header*)m_packedData.get())->length;
	}

	const Message::Header* Message::GetHeader() const
	{
		return ((Header*)m_packedData.get());
	}

	std::string Message::GetMessage() const
	{
		std::string rawMsg = (const char*)(m_packedData.get() + sizeof(Header));
		rawMsg[GetMessageSize()] = '\0';
		return rawMsg;
	}

	const void* Message::GetData() const
	{
		return m_packedData.get();
	}

	void Message::Clear()
	{
		memset(m_packedData.get(), 0, m_byteSize);
	}

	bool Message::IsValid()
	{
		return m_byteSize > sizeof(Header) && GetHeader()->length > 0;
	}
}
