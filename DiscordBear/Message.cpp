#include "PCH.h"
#include "Message.h"


namespace DiscordBear
{
	Message::Message(Message::Opcode opCode, const std::string& message)
	{
		Header header;
		header.opcode = opCode;
		header.length = static_cast<unsigned long>(message.length());

		m_headerSize = sizeof(Header);
		m_byteSize = m_headerSize + header.length;
		m_packedData = std::unique_ptr<char[]>(new char[m_byteSize + 1]);

		Clear();

		memcpy(m_packedData.get(), &header, m_headerSize);
		memcpy(m_packedData.get() + m_headerSize, message.c_str(), message.length());
	}

	Message::Message(unsigned long byteSize)
	{
		m_headerSize = sizeof(Header);
		m_byteSize = byteSize;
		m_packedData = std::unique_ptr<char[]>(new char[m_byteSize + 1]);

		Clear();
	}

	unsigned long Message::GetSize() const
	{
		return m_byteSize;
	}

	unsigned long Message::GetMessageSize() const
	{
		// A message that was cut short can be smaller than a header, don't read past what we own.
		if (m_byteSize < m_headerSize)
		{
			return 0;
		}

		return GetHeader().length;
	}

	Message::Header Message::GetHeader() const
	{
		// The buffer is plain bytes, copy the header out rather than casting it to a stricter alignment.
		Header header{};
		if (m_byteSize >= m_headerSize)
		{
			memcpy(&header, m_packedData.get(), sizeof(header));
		}

		return header;
	}

	std::string Message::GetMessage() const
	{
		// The header's length is whatever the sender claimed, never read more than the buffer holds.
		const unsigned long available = m_byteSize < m_headerSize ? 0 : m_byteSize - m_headerSize;
		const unsigned long length = GetMessageSize() < available ? GetMessageSize() : available;
		return std::string((const char*)(m_packedData.get() + m_headerSize), length);
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
		return m_byteSize > m_headerSize && GetHeader().length > 0;
	}
}