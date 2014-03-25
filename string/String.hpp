#pragma once

#include <cstring>
#include <string>
#include "../hash/Murmur3.hpp"

#ifdef _WIN32
#	define SPRAWL_CONSTEXPR const
#else
#	define SPRAWL_CONSTEXPR constexpr
#endif

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

			static SPRAWL_CONSTEXPR size_t staticDataSize = 64;

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

		String(const StringLiteral& stringLiteral);

		~String();

		std::string toStdString() const;

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

		String& operator=(const String& other);

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

		bool operator!=(const String& other) const
		{
			return !operator==(other);
		}

		sprawl::String operator+(const sprawl::String& other) const;

		bool empty()
		{
			return !m_holder || m_holder->m_length == 0;
		}

		bool operator<(const String& other) const;

		const char& operator[](size_t index) const
		{
			return m_holder->m_data[index];
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
			, m_length(N-1)
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
