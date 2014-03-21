#pragma once

#include <cstring>
#include <string>
#include "../hash/Murmur3.hpp"

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

			Holder(const char* data);
			Holder(const char* data, size_t length);
			Holder(const StringLiteral& literal);

			void IncRef();
			bool DecRef();

			static Holder* CreateHolder();
			static void FreeHolder(Holder* holder);

			~Holder();

			static constexpr size_t staticDataSize = 64;

			char m_staticData[staticDataSize];
			char* m_dynamicData;
			const char* m_data;
			int m_refCount;
			size_t m_length;
		private:
			Holder(const Holder& other);
			Holder& operator=(const Holder& other);
		};

		String();
		String(const char* const data);
		String(const char* const data, size_t length);

		String(const String& other);
		String(String&& other);

		String(const std::string& stlString);

		String(const StringLiteral& stringLiteral)
			: m_holder(Holder::CreateHolder())
		{
			new (m_holder) Holder(stringLiteral);
		}

		~String();

		std::string toStdString() const
		{
			if(!m_holder)
			{
				static std::string emptyStr;
				return emptyStr;
			}
			return std::string(m_holder->m_data, m_holder->m_length);
		}

		const char* c_str() const
		{
			if(!m_holder)
			{
				return "";
			}
			return m_holder->m_data;
		}

		size_t length() const
		{
			if(!m_holder)
			{
				return 0;
			}
			return m_holder->m_length;
		}

		String& operator=(const String& other)
		{
			if(m_holder && m_holder->DecRef())
			{
				Holder::FreeHolder(m_holder);
			}

			m_holder = other.m_holder;

			if(m_holder)
			{
				m_holder->IncRef();
			}
			return *this;
		}

		bool operator==(const String& other) const
		{
			if(m_holder == other.m_holder)
			{
				return true;
			}
			if(m_holder && other.m_holder)
			{
				if(m_holder->m_length != other.m_holder->m_length)
				{
					return false;
				}
				return ( memcmp(m_holder->m_data, other.m_holder->m_data, m_holder->m_length) == 0);
			}
			return false;
		}

		bool operator<(const String& other) const
		{
			if(m_holder == other.m_holder)
			{
				return false;
			}
			if(!m_holder)
			{
				return false;
			}
			if(!other.m_holder)
			{
				return true;
			}
			size_t length = other.m_holder->m_length < m_holder->m_length ? other.m_holder->m_length : m_holder->m_length;
			const char* const left = m_holder->m_data;
			const char* const right = other.m_holder->m_data;
			for(size_t i = 0; i < length; ++i)
			{
				if(left[i] == right[i])
					continue;

				return left[i] < right[i];
			}
			return false;
		}

	private:
		Holder* m_holder;
	};

	class StringLiteral
	{
	public:
		template<size_t N>
		explicit StringLiteral(const char (&ptr)[N])
			: m_ptr(ptr)
			, m_length(N)
		{
			//
		}

	protected:
		friend class String::Holder;
		const char* const m_ptr;
		size_t m_length;
	};
}

namespace std
{
	template<>
	struct hash<sprawl::String>
	{
		typedef sprawl::String argument_type;
		typedef std::size_t value_type;

		value_type operator()(const argument_type& str) const
		{
			return sprawl::murmur3::Hash( str.c_str(), str.length() );
		}
	};
}
