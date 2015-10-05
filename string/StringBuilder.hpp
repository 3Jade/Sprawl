#pragma once

#include "StringCommon.hpp"

#ifndef SPRAWL_STRING_NO_STL_COMPAT
#	include <string>
#endif

#ifndef SPRAWL_STRINGBUILDER_FAVOR_SPEED_OVER_MEMORY
#	define SPRAWL_STRINGBUILDER_FAVOR_SPEED_OVER_MEMORY 1
#endif

namespace sprawl
{
	class String;
	class StringLiteral;

	class StringBuilder
	{
	public:
		StringBuilder(size_t startingBufferSize = 512, bool allowGrowth = true);

		StringBuilder& operator<<(signed char const elem);
		StringBuilder& operator<<(short const elem);
		StringBuilder& operator<<(int const elem);
		StringBuilder& operator<<(long int const elem);
		StringBuilder& operator<<(long long int const elem);
		StringBuilder& operator<<(unsigned char const elem);
		StringBuilder& operator<<(unsigned short const elem);
		StringBuilder& operator<<(unsigned int const elem);
		StringBuilder& operator<<(unsigned long int const elem);
		StringBuilder& operator<<(unsigned long long int const elem);
		StringBuilder& operator<<(float const elem);
		StringBuilder& operator<<(double const elem);
		StringBuilder& operator<<(long double const elem);
		StringBuilder& operator<<(void const* const elem);
		StringBuilder& operator<<(bool const elem);

		StringBuilder& operator<<(char const elem);
		StringBuilder& operator<<(char const* const elem);
		StringBuilder& operator<<(char* const elem);
		StringBuilder& operator<<(String const& elem);
		StringBuilder& operator<<(StringLiteral const& elem);

#ifndef SPRAWL_STRING_NO_STL_COMPAT
		StringBuilder& operator<<(std::string const& elem);
#endif

		void AppendElementToBuffer(signed char const elem, char const* const modifiers);
		void AppendElementToBuffer(short const elem, char const* const modifiers);
		void AppendElementToBuffer(int const elem, char const* const modifiers);
		void AppendElementToBuffer(long int const elem, char const* const modifiers);
		void AppendElementToBuffer(long long int const elem, char const* const modifiers);
		void AppendElementToBuffer(unsigned char const elem, char const* const modifiers);
		void AppendElementToBuffer(unsigned short const elem, char const* const modifiers);
		void AppendElementToBuffer(unsigned int const elem, char const* const modifiers);
		void AppendElementToBuffer(unsigned long int const elem, char const* const modifiers);
		void AppendElementToBuffer(unsigned long long int const elem, char const* const modifiers);
		void AppendElementToBuffer(float const elem, char const* const modifiers);
		void AppendElementToBuffer(double const elem, char const* const modifiers);
		void AppendElementToBuffer(long double const elem, char const* const modifiers);
		void AppendElementToBuffer(void const* const elem, char const* const modifiers);
		void AppendElementToBuffer(void* const elem, char const* const modifiers);
		void AppendElementToBuffer(bool const elem, char const* const modifiers);
		void AppendElementToBuffer(char const elem, char const* const modifiers);
		void AppendElementToBuffer(char const* const elem, char const* const modifiers);
		void AppendElementToBuffer(char* const elem, char const* const modifiers);
		void AppendElementToBuffer(String const& elem, char const* const modifiers);
		void AppendElementToBuffer(StringLiteral const& elem, char const* const modifiers);
#ifndef SPRAWL_STRING_NO_STL_COMPAT
		void AppendElementToBuffer(std::string const& elem, char const* const modifiers);
#endif
		template<typename T>
		void AppendElementToBuffer(T const& elem, char const* const /*modifiers*/, typename std::enable_if<!std::is_pointer<T>::value>::type* = 0)
		{
			*this << elem;
		}

		template<typename T>
		StringBuilder& operator<<(T const& elem)
		{
			*this << (void const* const)&elem;
			return *this;
		}

		template<typename T>
		void AppendElementToBuffer(T const& elem, char const* const modifiers, typename std::enable_if<std::is_pointer<T>::value>::type* = 0)
		{
			if(elem != nullptr && strchr(modifiers, 'p') == nullptr)
			{
				*this << *elem;
			}
			else
			{
				char buf[15];
				sprintf(buf, "%%%sp", modifiers);
				checkedSnprintf_(buf, (void*)elem);
			}
		}

		String Str();
		String TempStr();

		~StringBuilder();

		size_t Size()
		{
			return m_pos;
		}

		void Reset()
		{
			m_pos = 0;
			m_bufferPos = m_buffer;
			m_remainingCapacity = m_bufferSize;
		}

	private:

		template<typename T>
		void checkedSnprintf_(char const* const pattern, T elem);

		void checkGrow_(size_t amount);

		static SPRAWL_CONSTEXPR size_t staticBufferSize = SPRAWL_STATIC_STRING_SIZE;

		char m_staticBuffer[staticBufferSize];
		char* m_dynamicBuffer;
		char* m_buffer;
		size_t m_bufferSize;
		size_t m_pos;

		char* m_bufferPos;
		size_t m_remainingCapacity;

		bool m_allowGrowth;
	};
}
