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
	void StringBuilder::checked_snprintf(char const* const pattern, T elem)
	{
		if(m_buffer && m_allowGrowth)
		{
#ifndef _WIN32
			size_t newPos = m_pos + snprintf(nullptr, 0, pattern, elem);
#else
			size_t newPos = m_pos + _snprintf(nullptr, 0, pattern, elem);
#endif

			if(newPos > (m_bufferSize * 0.75))
			{
				while(newPos > (m_bufferSize * 0.75))
				{
					m_bufferSize = m_bufferSize * 2 + 1;
				}

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

	StringBuilder& StringBuilder::operator<<(signed char const elem)
	{
		checked_snprintf("%hhd", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(short const elem)
	{
		checked_snprintf("%hd", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(int const elem)
	{
		checked_snprintf("%d", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(long int const elem)
	{
		checked_snprintf("%ld", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(long long int const elem)
	{
		checked_snprintf("%lld", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned char const elem)
	{
		checked_snprintf("%hhu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned short const elem)
	{
		checked_snprintf("%hu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned int const elem)
	{
		checked_snprintf("%u", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned long int const elem)
	{
		checked_snprintf("%lu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(unsigned long long int const elem)
	{
		checked_snprintf("%llu", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(float const elem)
	{
		checked_snprintf("%f", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(double const elem)
	{
		checked_snprintf("%f", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(long double const elem)
	{
		checked_snprintf("%Lf", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(void const* const elem)
	{
		checked_snprintf("%p", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(bool const elem)
	{
		if(elem)
		{
			checked_snprintf("%s", "True");
		}
		else
		{
			checked_snprintf("%s", "False");
		}
		return *this;
	}
	StringBuilder& StringBuilder::operator<<(char const elem)
	{
		checked_snprintf("%c", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(char const* const elem)
	{
		checked_snprintf("%s", elem);
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(String const& elem)
	{
		checked_snprintf("%s", elem.c_str());
		return *this;
	}

	StringBuilder& StringBuilder::operator<<(std::string const& elem)
	{
		checked_snprintf("%s", elem.c_str());
		return *this;
	}

	void StringBuilder::AppendElementToBuffer(signed char const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(short const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(int const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(long int const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(long long int const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned char const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned short const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned int const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned long int const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(unsigned long long int const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(float const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(double const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(long double const elem, char const* const modifiers)
	{
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

		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(void const* const elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%sp", modifiers);
		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(void* const elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%sp", modifiers);
		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(bool const elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%ss", modifiers);
		if(elem)
		{
			checked_snprintf(buf, "True");
		}
		else
		{
			checked_snprintf(buf, "False");
		}
	}
	void StringBuilder::AppendElementToBuffer(char const elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%sc", modifiers);
		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(char const* const elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%ss", modifiers);
		checked_snprintf(buf, elem);
	}

	void StringBuilder::AppendElementToBuffer(String const& elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%ss", modifiers);
		checked_snprintf(buf, elem.c_str());
	}

	void StringBuilder::AppendElementToBuffer(std::string const& elem, char const* const modifiers)
	{
		char buf[15];
		sprintf(buf, "%%%ss", modifiers);
		checked_snprintf(buf, elem.c_str());
	}

	String StringBuilder::Str()
	{
		return String(m_buffer, m_pos);
	}
}
