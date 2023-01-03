#include "PCH.h"
#include "Message.h"

#pragma optimize("", off)

namespace DiscordBear
{
	Message::Message(Message::Opcode opCode, std::string&& message)
	{
		Header header;
		header.opcode = opCode;
		header.length = static_cast<unsigned long>(message.length());

		m_headerSize = sizeof( Header );
		m_byteSize = m_headerSize + header.length;
		m_packedData = std::unique_ptr<char[]>(new char[m_byteSize+1]);

		Clear();

		memcpy(m_packedData.get(), &header, m_headerSize );
		memcpy(m_packedData.get() + m_headerSize, message.c_str(), message.length());
	}

	Message::Message(unsigned long byteSize)
	{
		m_headerSize = sizeof( Header );
		m_byteSize = byteSize;
		m_packedData = std::unique_ptr<char[]>(new char[m_byteSize+1]);

		Clear();
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
		return std::string((const char*)(m_packedData.get() + m_headerSize), GetMessageSize());
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
		return m_byteSize > m_headerSize && GetHeader()->length > 0;
	}
}
