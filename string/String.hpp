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

		template<typename... Params>
		String format(Params const& ...params)
		{
#if !SPRAWL_STRINGBUILDER_FAVOR_SPEED_OVER_MEMORY
			StringBuilder nullBuilder(0);

			ExecuteFormat(nullBuilder, params...);

			size_t const length = nullBuilder.Size();

			StringBuilder builder(length, false);
#else
			size_t const startingLength = m_holder->m_length * 2 + 1;

			StringBuilder builder(startingLength, true);
#endif

			ExecuteFormat(builder, params...);

			return builder.Str();
		}

	private:

#if (defined(_WIN32) && _MSC_VER < 1800)
		template<int idx, typename T = _Nil, _MAX_CLASS_LIST>
		class FormatHelper;

		template<size_t idx>
		class FormatHelper<idx, _Nil, _MAX_NIL_LIST>
		{
		public:
			void Append(int pos, StringBuilder& builder, char* modifiers)
			{
				(void)(pos);
				(void)(modifiers);
				builder << "< ??? >";
			}
		};

		#define _CLASS_FORMATHELPER(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			template<int idx, typename T COMMA LIST(_CLASS_TYPEX)> \
			class FormatHelper<idx, T, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> \
				: public FormatHelper< idx+1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD) > \
			{ \
			public: \
				typedef FormatHelper< idx+1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD) > Base; \
				FormatHelper(T const& val COMMA LIST(_CONST_TYPEX_REF_ARG)) \
					: Base(LIST(_ARGX)) \
					, m_value(val) \
				{ \
					\
				} \
				 \
				void Append(int pos, StringBuilder& builder, char* modifiers) \
				{ \
					if(pos == idx) \
					{ \
						builder.AppendElementToBuffer(m_value, modifiers); \
					} \
					else \
					{ \
						Base::Append(pos, builder, modifiers); \
					} \
				} \
				 \
			private: \
				T const& m_value; \
			};

		_VARIADIC_EXPAND_0X(_CLASS_FORMATHELPER, , , , )
#else
		template<int idx, typename... Params>
		class FormatHelper;

		template<int idx>
		class FormatHelper<idx>
		{
		public:
			void Append(int pos, StringBuilder& builder, char* modifiers)
			{
				(void)(pos);
				(void)(modifiers);
				builder << "< ??? >";
			}
		};

		template<int idx, typename T>
		class FormatHelper<idx, T> : public FormatHelper<idx + 1>
		{
		public:
			typedef FormatHelper<idx + 1> Base;
			FormatHelper(T const& val)
				: Base()
				, m_value(val)
			{
				//
			}

			void Append(int pos, StringBuilder& builder, char* modifiers)
			{
				if(pos == idx)
				{
					builder.AppendElementToBuffer(m_value, modifiers);
				}
				else
				{
					Base::Append(pos, builder, modifiers);
				}
			}

		private:
			T const& m_value;
		};

		template<int idx, typename T, typename... Params>
		class FormatHelper<idx, T, Params...> : public FormatHelper<idx + 1, Params...>
		{
		public:
			typedef FormatHelper<idx + 1, Params...> Base;
			FormatHelper(T const& val, Params const& ...values)
				: Base(values...)
				, m_value(val)
			{
				//
			}

			void Append(int pos, StringBuilder& builder, char* modifiers)
			{
				if(pos == idx)
				{
					builder.AppendElementToBuffer(m_value, modifiers);
				}
				else
				{
					Base::Append(pos, builder, modifiers);
				}
			}

		private:
			T const& m_value;
		};
#endif

		template<typename... Params>
		void ExecuteFormat(	StringBuilder& builder, Params const& ...params)
		{
			FormatHelper<0, Params...> helper(params...);

			int curIdx = -1;
			size_t lastIdx = 0;

			bool inBracket = false;

			char modifiers[10];
			size_t modifierPos = 0;
			bool inModifiers = false;

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
							helper.Append(lastIdx, builder, modifiers);
							++lastIdx;
						}
						else
						{
							helper.Append(curIdx, builder, modifiers);
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
