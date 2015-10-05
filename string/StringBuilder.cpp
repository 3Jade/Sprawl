#include "StringBuilder.hpp"
#include "String.hpp"

namespace sprawl
{
	StringBuilder::StringBuilder(size_t const startingBufferSize, bool allowGrowth)
		: m_staticBuffer()
		, m_dynamicBuffer(nullptr)
		, m_buffer(nullptr)
		, m_bufferSize(startingBufferSize)
		, m_pos(0)
		, m_bufferPos(nullptr)
		, m_remainingCapacity(0)
		, m_allowGrowth(allowGrowth)
	{
		if(startingBufferSize == 0)
		{
			return;
		}

		if(startingBufferSize <= staticBufferSize)
		{
			m_buffer = m_staticBuffer;
			m_bufferSize = staticBufferSize;
			m_remainingCapacity = staticBufferSize;
		}
		else
		{
			m_dynamicBuffer = new char[startingBufferSize+1];
			m_buffer = m_dynamicBuffer;
			m_remainingCapacity = startingBufferSize+1;
		}
		m_bufferPos = m_buffer;
	}

	StringBuilder::~StringBuilder()
	{
		if(m_dynamicBuffer)
		{
			delete[] m_dynamicBuffer;
		}
	}

	template<typename T>
	void StringBuilder::checkedSnprintf_(char const* const pattern, T elem)
	{
		if(m_buffer && m_allowGrowth)
		{
#ifndef _WIN32
			size_t newBytes = snprintf(nullptr, 0, pattern, elem);
#else
			size_t newBytes = _snprintf(nullptr, 0, pattern, elem);
#endif
			checkGrow_(newBytes);
		}

#ifndef _WIN32
		m_pos += snprintf(m_bufferPos, m_remainingCapacity, pattern, elem);
#else
		m_pos += _snprintf(m_bufferPos, m_remainingCapacity, pattern, elem);
#endif

		if(m_buffer)
		{
			m_bufferPos = m_buffer + m_pos;
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
	}

	void StringBuilder::checkGrow_(size_t amount)
	{
		if(amount > m_remainingCapacity)
		{
			m_bufferSize = m_pos + (amount > m_pos ? amount : m_pos);

			char* buf = m_dynamicBuffer;

			m_dynamicBuffer = new char[m_bufferSize+1];
			memcpy(m_dynamicBuffer, m_buffer, m_pos);
			m_buffer = m_dynamicBuffer;

			if(buf)
			{
				delete[] buf;
			}

			m_bufferPos = m_buffer + m_pos;
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
	}

	StringBuilder& StringBuilder::operator<<(signed char const elem)
	{
		checkedSnprintf_("%hhd", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(short const elem)
	{
		checkedSnprintf_("%hd", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(int const elem)
	{
		checkedSnprintf_("%d", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(long int const elem)
	{
		checkedSnprintf_("%ld", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(long long int const elem)
	{
		checkedSnprintf_("%lld", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned char const elem)
	{
		checkedSnprintf_("%hhu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned short const elem)
	{
		checkedSnprintf_("%hu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned int const elem)
	{
		checkedSnprintf_("%u", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned long int const elem)
	{
		checkedSnprintf_("%lu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned long long int const elem)
	{
		checkedSnprintf_("%llu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(float const elem)
	{
		checkedSnprintf_("%f", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(double const elem)
	{
		checkedSnprintf_("%f", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(long double const elem)
	{
		checkedSnprintf_("%Lf", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(void const* const elem)
	{
		checkedSnprintf_("%p", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(bool const elem)
	{
		AppendElementToBuffer(elem, nullptr);
		return *this;
	}
	StringBuilder& StringBuilder::operator<<(char const elem)
	{
		AppendElementToBuffer(elem, nullptr);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(char const* const elem)
	{
		AppendElementToBuffer(elem, nullptr);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(char* const elem)
	{
		AppendElementToBuffer(elem, nullptr);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(StringLiteral const& elem)
	{
		AppendElementToBuffer(elem, nullptr);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(String const& elem)
	{
		AppendElementToBuffer(elem, nullptr);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(std::string const& elem)
	{
		AppendElementToBuffer(elem, nullptr);
		return *this;
	}

	void StringBuilder::AppendElementToBuffer(signed char const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%hhd", elem);
			return;
		}
		char buf[15];

		char c = 'd';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%shh%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(short const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%hd", elem);
			return;
		}
		char buf[15];

		char c = 'd';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%sh%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(int const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%d", elem);
			return;
		}
		char buf[15];

		char c = 'd';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%s%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(long int const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%ld", elem);
			return;
		}
		char buf[15];

		char c = 'd';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%sl%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(long long int const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%lld", elem);
			return;
		}
		char buf[15];

		char c = 'd';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%sll%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned char const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%hhu", elem);
			return;
		}
		char buf[15];

		char c = 'u';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%shh%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned short const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%hu", elem);
			return;
		}
		char buf[15];

		char c = 'u';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%sh%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned int const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%u", elem);
			return;
		}
		char buf[15];

		char c = 'u';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%s%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned long int const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%lu", elem);
			return;
		}
		char buf[15];

		char c = 'u';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%sl%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned long long int const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%llu", elem);
			return;
		}
		char buf[15];

		char c = 'u';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'x' || c2 == 'X' || c2 == 'o')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%sll%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(float const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%g", elem);
			return;
		}
		char buf[15];

		char c = 'g';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'f' || c2 == 'F' || c2 == 'e' || c2 == 'E' || c2 == 'g' || c2 == 'G' || c2 == 'a' || c2 == 'A')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%s%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(double const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%g", elem);
			return;
		}
		char buf[15];

		char c = 'g';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'f' || c2 == 'F' || c2 == 'e' || c2 == 'E' || c2 == 'g' || c2 == 'G' || c2 == 'a' || c2 == 'A')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%s%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(long double const elem, char const* const modifiers)
	{
		if(modifiers == nullptr || *modifiers == 0)
		{
			checkedSnprintf_("%g", elem);
			return;
		}
		char buf[15];

		char c = 'g';

		char modifiedModifiers[10];
		size_t pos = 0;
		for(int i = 0; i < 10; ++i)
		{
			char c2 = modifiers[i];
			if(c2 == '\0')
			{
				modifiedModifiers[pos] = c2;
				break;
			}
			if(c2 == 'f' || c2 == 'F' || c2 == 'e' || c2 == 'E' || c2 == 'g' || c2 == 'G' || c2 == 'a' || c2 == 'A')
			{
				c = c2;
			}
			else
			{
				modifiedModifiers[pos++] = c2;
			}
		}

		sprintf(buf, "%%%sL%c", modifiedModifiers, c);

		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(void const* const elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%sp", modifiers);
		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(void* const elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%sp", modifiers);
		checkedSnprintf_(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(bool const elem, char const* const /*modifiers*/)
	{
		if(elem)
		{
			AppendElementToBuffer(sprawl::StringLiteral("True"), nullptr);
		}
		else
		{
			AppendElementToBuffer(sprawl::StringLiteral("False"), nullptr);
		}
	}

	void StringBuilder::AppendElementToBuffer(char const elem, char const* const /*modifiers*/)
	{
		if(m_buffer)
		{
			if(m_allowGrowth)
			{
				checkGrow_(1);
			}
			*m_bufferPos = elem;
			++m_bufferPos;
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
		++m_pos;
	}

	void StringBuilder::AppendElementToBuffer(char const* const elem, char const* const /*modifiers*/)
	{
		if(!elem)
		{
			AppendElementToBuffer(StringLiteral("(null)"), nullptr);
			return;
		}
		size_t len = strlen(elem);
		if(m_buffer)
		{
			if(m_allowGrowth)
			{
				checkGrow_(len);
			}
			memcpy(m_bufferPos, elem, len);
			m_bufferPos += len;
			m_pos += len;
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
		else
		{
			m_pos += len;
		}
	}

	void StringBuilder::AppendElementToBuffer(char* const elem, char const* const /*modifiers*/)
	{
		if(!elem)
		{
			AppendElementToBuffer(StringLiteral("(null)"), nullptr);
			return;
		}
		size_t len = strlen(elem);
		if(m_buffer)
		{
			if(m_allowGrowth)
			{
				checkGrow_(len);
			}
			memcpy(m_bufferPos, elem, len);
			m_bufferPos += len;
			m_pos += len;
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
		else
		{
			m_pos += len;
		}
	}

	void StringBuilder::AppendElementToBuffer(StringLiteral const& elem, char const* const /*modifiers*/)
	{
		if(m_buffer)
		{
			if(m_allowGrowth)
			{
				checkGrow_(elem.GetLength());
			}
			memcpy(m_bufferPos, elem.GetPtr(), elem.GetLength());
			m_bufferPos += elem.GetLength();
			m_pos += elem.GetLength();
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
		else
		{
			m_pos += elem.GetLength();
		}
	}

	void StringBuilder::AppendElementToBuffer(String const& elem, char const* const /*modifiers*/)
	{
		if(m_buffer)
		{
			if(m_allowGrowth)
			{
				checkGrow_(elem.length());
			}
			memcpy(m_bufferPos, elem.c_str(), elem.length());
			m_bufferPos += elem.length();
			m_pos += elem.length();
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
		else
		{
			m_pos += elem.length();
		}
	}

	void StringBuilder::AppendElementToBuffer(std::string const& elem, char const* const /*modifiers*/)
	{
		if(m_buffer)
		{
			if(m_allowGrowth)
			{
				checkGrow_(elem.length());
			}
			memcpy(m_bufferPos, elem.c_str(), elem.length());
			m_bufferPos += elem.length();
			m_pos += elem.length();
			m_remainingCapacity = m_bufferSize - m_pos + 1;
		}
		else
		{
			m_pos += elem.length();
		}
	}

	String StringBuilder::Str()
	{
		return String(m_buffer, m_pos);
	}

	String StringBuilder::TempStr()
	{
		m_buffer[m_pos] = '\0';
		return String(StringRef(m_buffer, m_pos));
	}
}
