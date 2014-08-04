#pragma once

#ifndef SPRAWL_STRING_NO_STL_COMPAT
#	include <string>
#	include "../hash/Murmur3.hpp"
#endif

#include "StringCommon.hpp"
#include "StringBuilder.hpp"

namespace sprawl
{
	class StringLiteral;

	class String
	{
	public:
		class Holder
		{
		protected:
			friend class String;

			Holder();
			Holder(const char* data);
			Holder(const char* data, size_t length);
			Holder(const StringLiteral& literal);

			void IncRef();
			bool DecRef();

			static Holder* CreateHolder();
			static void FreeHolder(Holder* holder);

			~Holder();

			inline size_t GetHash() const
			{
				if(m_hashComputed)
				{
					return m_hash;
				}

				m_hash = sprawl::murmur3::Hash( m_data, m_length );
				m_hashComputed = true;
				return m_hash;
			}

			static SPRAWL_CONSTEXPR size_t staticDataSize = SPRAWL_STATIC_STRING_SIZE;

			char m_staticData[staticDataSize];
			char* m_dynamicData;
			const char* m_data;
			int m_refCount;
			size_t m_length;
			mutable size_t m_hash;
			mutable bool m_hashComputed;
		private:
			Holder(const Holder& other);
			Holder& operator=(const Holder& other);
		};

		String();
		String(const char* const data);
		String(const char* const data, size_t length);

		String(const String& other);
		String(String&& other);

#ifndef SPRAWL_STRING_NO_STL_COMPAT
		String(const std::string& stlString);
#endif

		String(const StringLiteral& stringLiteral);

		~String();

		inline size_t GetHash() const
		{
			return m_holder->GetHash();
		}

#ifndef SPRAWL_STRING_NO_STL_COMPAT
		std::string toStdString() const;
#endif

		const char* c_str() const
		{
			return m_holder->m_data;
		}

		size_t length() const
		{
			return m_holder->m_length;
		}

		String& operator=(const String& other);
		String& operator=(String&& other);

		inline bool operator==(const String& other) const
		{
			return (m_holder == other.m_holder) || ((m_holder->m_length == other.m_holder->m_length) && (SPRAWL_MEMCMP(m_holder->m_data, other.m_holder->m_data, m_holder->m_length) == 0));
		}

		bool operator!=(const String& other) const
		{
			return !operator==(other);
		}

		sprawl::String operator+(const sprawl::String& other) const;

		sprawl::String operator+(const char* other) const;

		sprawl::String& operator+=(const sprawl::String& other);

		sprawl::String& operator+=(const char* other);

		bool empty()
		{
			return m_holder->m_length == 0;
		}

		bool operator<(const String& other) const;

		const char& operator[](size_t index) const
		{
			return m_holder->m_data[index];
		}

#ifndef _WIN32
		template<
			typename T1,
			typename T2=void*,
			typename T3=void*,
			typename T4=void*,
			typename T5=void*,
			typename T6=void*,
			typename T7=void*,
			typename T8=void*,
			typename T9=void*,
			typename T10=void*,
			typename T11=void*,
			typename T12=void*,
			typename T13=void*,
			typename T14=void*,
			typename T15=void*,
			typename T16=void*,
			typename T17=void*,
			typename T18=void*,
			typename T19=void*,
			typename T20=void*
		>
		String format(
			T1 const& arg1,
			T2 const& arg2 = T2(),
			T3 const& arg3 = T3(),
			T4 const& arg4 = T4(),
			T5 const& arg5 = T5(),
			T6 const& arg6 = T6(),
			T7 const& arg7 = T7(),
			T8 const& arg8 = T8(),
			T9 const& arg9 = T9(),
			T10 const& arg10 = T10(),
			T11 const& arg11 = T11(),
			T12 const& arg12 = T12(),
			T13 const& arg13 = T13(),
			T14 const& arg14 = T14(),
			T15 const& arg15 = T15(),
			T16 const& arg16 = T16(),
			T17 const& arg17 = T17(),
			T18 const& arg18 = T18(),
			T19 const& arg19 = T19(),
			T20 const& arg20 = T20()
		)
		{
#if !SPRAWL_STRINGBUILDER_FAVOR_SPEED_OVER_MEMORY
			StringBuilder nullBuilder(0);

			ExecuteFormat(
				nullBuilder,
				arg1,
				arg2,
				arg3,
				arg4,
				arg5,
				arg6,
				arg7,
				arg8,
				arg9,
				arg10,
				arg11,
				arg12,
				arg13,
				arg14,
				arg15,
				arg16,
				arg17,
				arg18,
				arg19,
				arg20
			);

			size_t const length = nullBuilder.Size();

			StringBuilder builder(length, false);
#else
			size_t const startingLength = m_holder->m_length * 2 + 1;

			StringBuilder builder(startingLength, true);
#endif

			ExecuteFormat(
				builder,
				arg1,
				arg2,
				arg3,
				arg4,
				arg5,
				arg6,
				arg7,
				arg8,
				arg9,
				arg10,
				arg11,
				arg12,
				arg13,
				arg14,
				arg15,
				arg16,
				arg17,
				arg18,
				arg19,
				arg20
			);

			return builder.Str();
		}

	private:
		template<
			typename T1,
			typename T2,
			typename T3,
			typename T4,
			typename T5,
			typename T6,
			typename T7,
			typename T8,
			typename T9,
			typename T10,
			typename T11,
			typename T12,
			typename T13,
			typename T14,
			typename T15,
			typename T16,
			typename T17,
			typename T18,
			typename T19,
			typename T20
		>
		void ExecuteFormat(
			StringBuilder& builder,
			T1 const& arg1,
			T2 const& arg2,
			T3 const& arg3,
			T4 const& arg4,
			T5 const& arg5,
			T6 const& arg6,
			T7 const& arg7,
			T8 const& arg8,
			T9 const& arg9,
			T10 const& arg10,
			T11 const& arg11,
			T12 const& arg12,
			T13 const& arg13,
			T14 const& arg14,
			T15 const& arg15,
			T16 const& arg16,
			T17 const& arg17,
			T18 const& arg18,
			T19 const& arg19,
			T20 const& arg20
		)
		{
			int curIdx = -1;
			size_t lastIdx = 0;

			bool inBracket = false;

			char modifiers[10];
			size_t modifierPos = 0;
			bool inModifiers = false;

			#define SPRAWL_STRING_APPEND_ARG(argnum, modifiers) \
				switch(argnum) \
				{ \
					case 0: builder.AppendElementToBuffer(arg1, modifiers); break; \
					case 1: builder.AppendElementToBuffer(arg2, modifiers); break; \
					case 2: builder.AppendElementToBuffer(arg3, modifiers); break; \
					case 3: builder.AppendElementToBuffer(arg4, modifiers); break; \
					case 4: builder.AppendElementToBuffer(arg5, modifiers); break; \
					case 5: builder.AppendElementToBuffer(arg6, modifiers); break; \
					case 6: builder.AppendElementToBuffer(arg7, modifiers); break; \
					case 7: builder.AppendElementToBuffer(arg8, modifiers); break; \
					case 8: builder.AppendElementToBuffer(arg9, modifiers); break; \
					case 9: builder.AppendElementToBuffer(arg10, modifiers); break; \
					case 10: builder.AppendElementToBuffer(arg11, modifiers); break; \
					case 11: builder.AppendElementToBuffer(arg12, modifiers); break; \
					case 12: builder.AppendElementToBuffer(arg13, modifiers); break; \
					case 13: builder.AppendElementToBuffer(arg14, modifiers); break; \
					case 14: builder.AppendElementToBuffer(arg15, modifiers); break; \
					case 15: builder.AppendElementToBuffer(arg16, modifiers); break; \
					case 16: builder.AppendElementToBuffer(arg17, modifiers); break; \
					case 17: builder.AppendElementToBuffer(arg18, modifiers); break; \
					case 18: builder.AppendElementToBuffer(arg19, modifiers); break; \
					case 19: builder.AppendElementToBuffer(arg20, modifiers); break; \
					default: builder << "< ??? >"; break; \
				}

			size_t const formatLength = m_holder->m_length;
			char const* const data = m_holder->m_data;

			for(size_t i = 0; i < formatLength; ++i)
			{
				const char c = data[i];
				if(c == '{')
				{
					if(inBracket)
					{
						builder << '{';
					}
					inBracket = !inBracket;
					continue;
				}

				if(inBracket)
				{
					if(c == '}')
					{
						modifiers[modifierPos] = '\0';

						if(curIdx == -1)
						{
							SPRAWL_STRING_APPEND_ARG(lastIdx, modifiers);
							++lastIdx;
						}
						else
						{
							SPRAWL_STRING_APPEND_ARG(curIdx, modifiers);
							lastIdx = curIdx + 1;
						}
						modifiers[0] = '\0';
						modifierPos = 0;
						curIdx = -1;
						inBracket = false;
						inModifiers = false;
					}
					else if(c == ':' && !inModifiers)
					{
						inModifiers = true;
					}
					else if(inModifiers)
					{
						modifiers[modifierPos++] = c;
					}
					else if(isdigit(c))
					{
						if(curIdx == -1)
						{
							curIdx = c - '0';
						}
						else
						{
							curIdx *= 10;
							curIdx += c - '0';
						}
					}
					continue;
				}

				builder << c;
			}
		}
#endif
	private:
		Holder* m_holder;
		static Holder ms_emptyHolder;
	};

	class StringLiteral
	{
	public:
		template<size_t N>
		explicit StringLiteral(const char (&ptr)[N])
			: m_ptr(ptr)
			, m_length(N-1)
		{
			//
		}

		explicit StringLiteral(const char* ptr, size_t length)
			: m_ptr(ptr)
			, m_length(length)
		{
			//
		}

	protected:
		StringLiteral& operator=(const StringLiteral& other);
		StringLiteral(const StringLiteral& other);

		friend class String::Holder;
		const char* const m_ptr;
		size_t m_length;
	};
	typedef StringLiteral StringRef;
}

#ifndef SPRAWL_STRING_NO_STL_COMPAT
namespace std
{
	template<>
	struct hash<sprawl::String>
	{
		typedef sprawl::String argument_type;
		typedef std::size_t value_type;

		inline value_type operator()(const argument_type& str) const
		{
			return str.GetHash();
		}
	};
}
#endif
