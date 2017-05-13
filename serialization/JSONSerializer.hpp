#pragma once

/*
 * This module is included as a part of libSprawl
 *
 * Copyright (C) 2013 Jaedyn K. Draper
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef _WIN32
	#pragma warning( push )
	#pragma warning( disable: 4250; disable: 4996 )
#endif

#include "Serializer.hpp"
#include "../collections/HashMap.hpp"
#include "../collections/Vector.hpp"
#include <vector>

namespace sprawl
{
	namespace serialization
	{
		class StringData
		{
		public:
			StringData( char const* const data, size_t length )
				: m_data( data )
				, m_length( length )
				, m_commitData( nullptr )
			{
				//
			}


			StringData( char const* const data )
				: m_data( data )
				, m_length( strlen( data ) )
				, m_commitData( nullptr )
			{
				//
			}

			void CommitStorage()
			{
				m_commitData = new char[m_length+1];
				memcpy(m_commitData, m_data, m_length);
				m_data = m_commitData;
			}

			~StringData()
			{
				if( m_commitData )
				{
					delete[] m_commitData;
				}
			}

			StringData( StringData const& other )
				: m_data( other.m_data )
				, m_length( other.m_length )
				, m_commitData( other.m_commitData )
			{
				if( m_commitData )
				{
					CommitStorage(); // Get our own copy of it, there's no refcounting
				}
			}

			StringData& operator=( StringData const& other )
			{
				if( m_commitData )
				{
					delete[] m_commitData;
				}
				m_data = other.m_data;
				m_length = other.m_length;
				if( other.m_commitData )
				{
					CommitStorage();
				}
				return *this;
			}

			bool operator==( StringData const& other ) const
			{
				if( m_length != other.m_length )
				return false;
				return !memcmp( m_data, other.m_data, m_length );
			}

			size_t length() const
			{
				return m_length;
			}

			char const* c_str() const
			{
				return m_data;
			}

			char operator[](size_t index) const
			{
				return m_data[index];
			}

			sprawl::String toString() const
			{
				return sprawl::String( m_data, m_length );
			}

			private:
			char const* m_data;
			size_t m_length;
			char* m_commitData;
		};
	}
}

namespace std
{
	template<>
	struct hash<sprawl::serialization::StringData>
	{
		typedef sprawl::serialization::StringData argument_type;
		typedef std::size_t value_type;

		inline value_type operator()(argument_type const& str) const
		{
			return sprawl::murmur3::Hash( str.c_str(), str.length() );
		}
	};
}

namespace sprawl
{
	namespace serialization
	{
		class JSONToken
		{
		public:
			static long long ToInt(StringData const& str);
			static unsigned long long ToUInt(StringData const& str);
			static long double ToDouble(StringData const& str);
			static bool ToBool(StringData const& str);
			static sprawl::String EscapeString(StringData const& str);
			static sprawl::String UnescapeString(StringData const& str);

			enum class JSONType
			{
				Empty = 0,
				Integer = 1,
				String = 2,
				Boolean = 3,
				Array = 4,
				Object = 5,
				Double = 6,
				Null = 7,
			};

			sprawl::String GetKey() const
			{
				return sprawl::StringLiteral(m_key.c_str(), m_key.length());
			}

			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<long long> Int() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Integer )
				{
					SPRAWL_THROW_EXCEPTION(sprawl::JsonTypeMismatch());
				}
#endif

				return ToInt(m_holder->m_data);
			}

			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<unsigned long long> Unsigned() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Integer )
				{
					SPRAWL_THROW_EXCEPTION(sprawl::JsonTypeMismatch());
				}
#endif

				return ToUInt(m_holder->m_data);
			}

			void Val(signed char& value)
			{
				value = (signed char)(Int());
			}

			void Val(short& value)
			{
				value = short(Int());
			}

			void Val(int& value)
			{
				value = int(Int());
			}

			void Val(long& value)
			{
				value = long(Int());
			}

			void Val(long long& value)
			{
				value = (long long)(Int());
			}

			void Val(unsigned char& value)
			{
				value = (unsigned char)(Int());
			}

			void Val(unsigned short& value)
			{
				value = (unsigned short)(Int());
			}

			void Val(unsigned int& value)
			{
				value = (unsigned int)(Int());
			}

			void Val(unsigned long& value)
			{
				value = (unsigned long)(Int());
			}

			void Val(unsigned long long& value)
			{
				value = (unsigned long long)(Int());
			}

			void Val(bool& value)
			{
				value = Bool();
			}

			void Val(double& value)
			{
				value = (double)Double();
			}

			void Val(float& value)
			{
				value = float(Double());
			}

			void Val(long double& value)
			{
				value = Double();
			}

			void Val(sprawl::String& value)
			{
				value = String();
			}

			void Val(char* value, size_t length)
			{
				sprawl::String data = String();
				size_t copyLength = length < data.length() ? length : data.length();
				memcpy(value, data.c_str(), copyLength);
				value[copyLength] = '\0';
			}

			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<sprawl::String> String() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::String )
				{
					SPRAWL_THROW_EXCEPTION(sprawl::JsonTypeMismatch());
				}
#endif

				return UnescapeString(m_holder->m_data);
			}

			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<bool> Bool() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Boolean )
				{
					SPRAWL_THROW_EXCEPTION(sprawl::JsonTypeMismatch());
				}
#endif

				return ToBool(m_holder->m_data);
			}

			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<long double> Double() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Double )
				{
					SPRAWL_THROW_EXCEPTION(sprawl::JsonTypeMismatch());
				}
#endif

				return ToDouble(m_holder->m_data);
			}

			JSONToken& NextSibling();

			JSONToken const& NextSibling() const;

			JSONToken const& operator[]( sprawl::String const& key ) const;

			JSONToken const& operator[]( ssize_t index ) const;

			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> operator[]( sprawl::String const& key );

			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> operator[](ssize_t index);

			JSONType Type()
			{
				return m_holder->m_type;
			}

			bool IsNull()
			{
				return (m_holder->m_type == JSONType::Null);
			}

			bool IsEmpty()
			{
				return (m_holder->m_type == JSONType::Empty);
			}

			bool HasKey(sprawl::String const& key)
			{
				return m_holder->m_objectChildren->Has(StringData(key.c_str(), key.length()));
			}

			size_t Size();

			sprawl::String ToJSONString( bool pretty = false );

			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( JSONToken const& token );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( signed char value ) { return PushBack((long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( short value ) { return PushBack((long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( int value ) { return PushBack((long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( long value ) { return PushBack((long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( unsigned char value ) { return PushBack((unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( unsigned short value ) { return PushBack((unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( unsigned int value ) { return PushBack((unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( unsigned long value ) { return PushBack((unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( unsigned long long value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( long long value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( float value ) { return PushBack((long double)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( double value ) { return PushBack((long double)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( long double value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( bool value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( const char* const value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( const char* const value, size_t length );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> PushBack( sprawl::String const& value );

			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, JSONToken const& token );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, signed char value ) { return Insert(name, (long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, short value ) { return Insert(name, (long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, int value ) { return Insert(name, (long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, long value ) { return Insert(name, (long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, unsigned char value ) { return Insert(name, (unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, unsigned short value ) { return Insert(name, (unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, unsigned int value ) { return Insert(name, (unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, unsigned long value ) { return Insert(name, (unsigned long long)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, unsigned long long value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, long long value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, float value ) { return Insert(name, (long double)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, double value ) { return Insert(name, (long double)(value)); }
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, long double value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, bool value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, const char* const value );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, const char* const value, size_t length );
			SPRAWL_WARN_UNUSED_RESULT ErrorState<JSONToken&> Insert( sprawl::String const& name, sprawl::String const& value );

			static JSONToken array()
			{
				return JSONToken( JSONType::Array, sprawl::String() );
			}

			static JSONToken object()
			{
				return JSONToken( JSONType::Object, sprawl::String() );
			}

			static JSONToken string( char const* const data )
			{
				return JSONToken( JSONType::String, sprawl::String( data ) );
			}

			static JSONToken string( char const* const data, size_t length )
			{
				return JSONToken( JSONType::String, sprawl::String( data, length ) );
			}

			static JSONToken string( sprawl::String const& data )
			{
				return JSONToken( JSONType::String, data );
			}

			static JSONToken number( signed char value ) { return number((long long)(value)); }
			static JSONToken number( short value ) { return number((long long)(value)); }
			static JSONToken number( int value ) { return number((long long)(value)); }
			static JSONToken number( long value ) { return number((long long)(value)); }
			static JSONToken number( unsigned char value ) { return number((unsigned long long)(value)); }
			static JSONToken number( unsigned short value ) { return number((unsigned long long)(value)); }
			static JSONToken number( unsigned int value ) { return number((unsigned long long)(value)); }
			static JSONToken number( unsigned long value ) { return number((unsigned long long)(value)); }

			static JSONToken number( long long data )
			{
				return JSONToken( JSONType::Integer, data );
			}

			static JSONToken number( unsigned long long data )
			{
				return JSONToken( JSONType::Integer, data );
			}

			static JSONToken number( long double data )
			{
				return JSONToken( JSONType::Double, data );
			}

			static JSONToken boolean( bool data )
			{
				return JSONToken( JSONType::Boolean, data );
			}

			static JSONToken null()
			{
				return JSONToken( JSONType::Null );
			}

			static JSONToken empty()
			{
				return JSONToken( JSONType::Empty );
			}

			static JSONToken fromString(sprawl::String const& jsonStr)
			{
				const char* data = jsonStr.c_str();
				return JSONToken(StringData(nullptr, 0), data, JSONType::Object);
			}

			~JSONToken();

			JSONToken();
			JSONToken(JSONToken const& other);
			JSONToken& operator=(JSONToken const& other);

			StringData const& KeyData() const
			{
				return m_key;
			}

			typedef sprawl::collections::HashMap<JSONToken, sprawl::ConstMemberAccessor<JSONToken, StringData const&, &JSONToken::KeyData>> TokenMap;
			class iterator;

			iterator begin();
			iterator end();

			JSONToken ShallowCopy();
			JSONToken DeepCopy();

		protected:
			friend class sprawl::collections::Vector<JSONToken>;
			JSONToken( StringData const& myKey, char const*& data, JSONType expectedType );
			JSONToken( JSONType statedType, sprawl::String const& data );
			JSONToken( JSONType statedType, bool data );
			JSONToken( JSONType statedType, long long data );
			JSONToken( JSONType statedType, unsigned long long data );
			JSONToken( JSONType statedType );
			JSONToken( JSONType statedType, long double data );
			JSONToken( StringData const& myKey, JSONType statedType, sprawl::String const& data );
			JSONToken( StringData const& myKey, JSONType statedType, bool data );
			JSONToken( StringData const& myKey, JSONType statedType, long long data );
			JSONToken( StringData const& myKey, JSONType statedType, unsigned long long data );
			JSONToken( StringData const& myKey, JSONType statedType );
			JSONToken( StringData const& myKey, JSONType statedType, long double data );
			JSONToken( StringData const& myKey, JSONToken const& other );

			static JSONToken* Create();
			static void Free(JSONToken* token);

		private:
			void BuildJSONString(sprawl::StringBuilder& outString, bool pretty, int tabDepth);
			void IncRef()
			{
				++m_holder->refCount;
			}
			void DecRef()
			{
				if(--m_holder->refCount == 0)
				{
					Holder::Free(m_holder);
				}
			}

			void SkipWhitespace(char const*& data);
			void CollectString(char const*& data);
			void ParseString(char const*& data);
			void ParseNumber(char const*& data);
			void ParseBool(char const*& data);
			void ParseArray(char const*& data);
			void ParseObject(char const*& data);

			struct Holder
			{
				StringData m_data;
				JSONType m_type;
				TokenMap* m_objectChildren;
				sprawl::collections::Vector<JSONToken> m_arrayChildren;
				int refCount;

				bool Unique() { return refCount == 1; }
				static Holder* Create();
				static void Free(Holder* holder);

				Holder(JSONType forType);

				~Holder();
			};

			Holder* m_holder;
			StringData m_key;
			static JSONToken staticEmpty;
		};

		class JSONToken::iterator
		{
		public:
			iterator(sprawl::collections::Vector<JSONToken>::iterator it)
				: mapIter(nullptr)
				, arrayIter(it)
				, type(JSONType::Array)
			{
				//
			}
			iterator(TokenMap::iterator it)
				: mapIter(it)
				, arrayIter(nullptr, nullptr)
				, type(JSONType::Object)
			{
				//
			}

			iterator(std::nullptr_t)
				: mapIter(nullptr)
				, arrayIter(nullptr, nullptr)
				, type(JSONType::Empty)
			{
				//
			}

			JSONToken& operator*()
			{
				if(type == JSONType::Array)
				{
					return *arrayIter;
				}
				return mapIter->Value();
			}

			JSONToken* operator->()
			{
				if(type == JSONType::Array)
				{
					return &*arrayIter;
				}
				return &mapIter->Value();
			}


			JSONToken const& operator*() const
			{
				if(type == JSONType::Array)
				{
					return *arrayIter;
				}
				return mapIter->Value();
			}

			JSONToken const* operator->() const
			{
				if(type == JSONType::Array)
				{
					return &*arrayIter;
				}
				return &mapIter->Value();
			}

			JSONType Type()
			{
				return type;
			}

			ssize_t Index() const
			{
				if(type == JSONType::Array)
				{
					return arrayIter.Index();
				}
				return -1;
			}

			sprawl::String Key() const
			{
				if(type == JSONType::Array)
				{
					return "";
				}
				return sprawl::String(mapIter->Key().c_str(), mapIter->Key().length());
			}

			JSONToken& Value()
			{
				if(type == JSONType::Array)
				{
					return *arrayIter;
				}
				return mapIter->Value();
			}

			JSONToken const& Value() const
			{
				if(type == JSONType::Array)
				{
					return *arrayIter;
				}
				return mapIter->Value();
			}

			iterator& operator++()
			{
				if(type == JSONType::Array)
				{
					++arrayIter;
				}
				else
				{
					++mapIter;
				}
				return *this;
			}

			iterator operator++(int)
			{
				iterator tmp(*this);
				++(*this);
				return tmp;
			}

			iterator const& operator++() const
			{
				if(type == JSONType::Array)
				{
					++arrayIter;
				}
				else
				{
					++mapIter;
				}
				return *this;
			}

			iterator const operator++(int) const
			{
				iterator tmp(*this);
				++(*this);
				return tmp;
			}

			bool operator==(iterator const& rhs) const
			{
				if(type == JSONType::Array)
				{
					return arrayIter == rhs.arrayIter;
				}
				return mapIter == rhs.mapIter;
			}

			bool operator!=(iterator const& rhs) const
			{
				return !this->operator==(rhs);
			}

			operator bool() const
			{
				return Valid();
			}

			bool operator!() const
			{
				return !Valid();
			}

			bool Valid() const
			{
				if(type == JSONType::Empty)
				{
					return false;
				}
				else if(type == JSONType::Array)
				{
					return arrayIter.Valid();
				}
				return mapIter.Valid();
			}

			bool More() const
			{
				if(type == JSONType::Empty)
				{
					return false;
				}
				else if(type == JSONType::Array)
				{
					return arrayIter.More();
				}
				return mapIter.More();
			}

			iterator Next()
			{
				return ++(*this);
			}

			iterator const Next() const
			{
				return ++(*this);
			}
		private:
			JSONToken::TokenMap::iterator mapIter;
			sprawl::collections::Vector<JSONToken>::iterator arrayIter;
			JSONType type;
		};

		class JSONSerializerBase : virtual public SerializerBase
		{
		public:
			typedef class JSONSerializer serializer_type;
			typedef class JSONDeserializer deserializer_type;
			virtual uint32_t GetVersion() override { return m_version; }
			virtual bool IsValid() override { return m_bIsValid; }
			virtual bool Error() override { return false; }
			virtual void SetVersion(uint32_t i) override
			{
				m_version = i;
				if(IsSaving() && m_bWithMetadata)
				{
					//Update version metadata
					m_rootToken["__version__"] = JSONToken::number(i);
				}
			}
			virtual ErrorState<void> Reset() override { return ErrorState<void>(); }
			using SerializerBase::Data;
			virtual const char* Data() override { return Str().c_str(); }
			virtual sprawl::String Str() override
			{
				return m_rootToken.ToJSONString(m_prettyPrint);
			}
			
			virtual size_t Size() override
			{
				return Str().length();
			}

			bool More()
			{
				return !m_currentObjectIndex.front()->IsEmpty();
			}
		protected:

			template<typename T>
			friend class ReplicableBase;
			using SerializerBase::serialize;

			template<typename T>
			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> serialize_impl(T* var, const uint32_t /*bytes*/, sprawl::String const& name, bool)
			{
				JSONToken* currentToken = m_currentToken.back();
				if(IsSaving())
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						SPRAWL_RETHROW(currentToken->PushBack(*var));
					}
					else
					{
						SPRAWL_RETHROW(currentToken->Insert(name, *var));
					}
				}
				else
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						JSONToken& token = currentToken->operator[](m_currentArrayIndex.back()++);
						token.Val(*var);
					}
					else
					{
						JSONToken& token = currentToken->operator[](name);
						if(token.IsEmpty())
						{
							SPRAWL_THROW_EXCEPTION(sprawl::JsonTypeMismatch());
						}
						token.Val(*var);
						++m_currentObjectIndex.back();
					}
				}
				return ErrorState<void>();
			}

			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> serialize_impl(char* var, const uint32_t bytes, sprawl::String const& name, bool)
			{
				JSONToken* currentToken = m_currentToken.back();
				if(IsSaving())
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						SPRAWL_RETHROW(currentToken->PushBack(var, bytes));
					}
					else
					{
						SPRAWL_RETHROW(currentToken->Insert(name, var, bytes));
					}
				}
				else
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						JSONToken& token = currentToken->operator[](m_currentArrayIndex.back()++);
						token.Val(var, bytes);
					}
					else
					{
						JSONToken& token = currentToken->operator[](name);
						if(token.IsEmpty())
						{
							SPRAWL_THROW_EXCEPTION(sprawl::JsonTypeMismatch());
						}
						token.Val(var, bytes);
						++m_currentObjectIndex.back();
					}
				}
				return ErrorState<void>();
			}

			SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> serialize_impl(std::string* var, const uint32_t /*bytes*/, sprawl::String const& name, bool)
			{
				JSONToken* currentToken = m_currentToken.back();
				if(IsSaving())
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						SPRAWL_RETHROW(currentToken->PushBack(var->c_str(), var->length()));
					}
					else
					{
						SPRAWL_RETHROW(currentToken->Insert(name, var->c_str(), var->length()));
					}
				}
				else
				{
					sprawl::String str;

					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						JSONToken& token = currentToken->operator[](m_currentArrayIndex.back()++);
						token.Val(str);
					}
					else
					{
						JSONToken& token = currentToken->operator[](name);
						if(token.IsEmpty())
						{
							SPRAWL_THROW_EXCEPTION(sprawl::InvalidJsonData());
						}
						token.Val(str);
						++m_currentObjectIndex.back();
					}

					var->assign(str.c_str(), str.length());
				}
				return ErrorState<void>();
			}

		public:
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(long long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB)  override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(short int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(char* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(float* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(double* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(long double* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(bool* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned long long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned short int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned char* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(std::string* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(sprawl::String* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				SPRAWL_RETHROW(serialize_impl(var, bytes, name, PersistToDB));
				return ErrorState<void>();
			}

			virtual uint32_t StartObject(sprawl::String const& str, bool = true) override
			{
				JSONToken& token = *m_currentToken.back();
				if(IsLoading())
				{
					if(token.Type() == JSONToken::JSONType::Array)
					{
						JSONToken& subtoken = token[m_currentArrayIndex.back()];
						m_currentToken.push_back(&subtoken);
					}
					else
					{
						JSONToken& subtoken = token[str];
						m_currentToken.push_back(&subtoken);
					}
					m_currentObjectIndex.push_back(m_currentToken.back()->begin());
					return uint32_t(m_currentToken.back()->Size());
				}
				else
				{
					if(token.Type() == JSONToken::JSONType::Array)
					{
						JSONToken& subtoken = token.PushBack(JSONToken::object());
						m_currentToken.push_back(&subtoken);
					}
					else
					{
						JSONToken& subtoken = token.Insert(str, JSONToken::object());
						m_currentToken.push_back(&subtoken);
					}
					return 0;
				}
			}

			virtual void EndObject() override
			{
				m_currentToken.pop_back();
				if(IsLoading())
				{
					m_currentObjectIndex.pop_back();
					if(m_currentToken.back()->Type() == JSONToken::JSONType::Object)
					{
						++m_currentObjectIndex.back();
					}
					else
					{
						++m_currentArrayIndex.back();
					}
				}
			}

			virtual void StartArray(sprawl::String const& str, uint32_t& size, bool = true) override
			{
				JSONToken& token = *m_currentToken.back();
				if(IsLoading())
				{
					if(token.Type() == JSONToken::JSONType::Array)
					{
						JSONToken& subtoken = token[m_currentArrayIndex.back()];
						m_currentToken.push_back(&subtoken);
					}
					else
					{
						JSONToken& subtoken = token[str];
						m_currentToken.push_back(&subtoken);
					}
					m_currentArrayIndex.push_back(0);
					size = uint32_t(m_currentToken.back()->Size());
				}
				else
				{
					if(token.Type() == JSONToken::JSONType::Array)
					{
						JSONToken& subtoken = token.PushBack(JSONToken::array());
						m_currentToken.push_back(&subtoken);
					}
					else
					{
						JSONToken& subtoken = token.Insert(str, JSONToken::array());
						m_currentToken.push_back(&subtoken);
					}
				}
			}

			virtual void EndArray() override
			{
				m_currentToken.pop_back();
				if(IsLoading())
				{
					m_currentArrayIndex.pop_back();
					if(m_currentToken.back()->Type() == JSONToken::JSONType::Object)
					{
						++m_currentObjectIndex.back();
					}
					else
					{
						++m_currentArrayIndex.back();
					}
				}
			}

			virtual sprawl::String GetNextKey() override
			{
				return m_currentObjectIndex.back()->GetKey();
			}

		protected:
			JSONSerializerBase()
				: SerializerBase()
				, m_rootToken(JSONToken::object())
				, m_currentToken()
				, m_currentArrayIndex()
				, m_currentObjectIndex()
				, m_prettyPrint(false)
				, m_version(0)
				, m_bIsValid(true)
				, m_bWithMetadata(true)
			{
				m_currentToken.push_back(&m_rootToken);
				m_currentObjectIndex.push_back(m_rootToken.begin());
			}

			virtual ~JSONSerializerBase()
			{
				//
			}

			JSONToken m_rootToken;
			std::vector<JSONToken*> m_currentToken;
			std::vector<int> m_currentArrayIndex;
			std::vector<JSONToken::iterator> m_currentObjectIndex;

			bool m_prettyPrint;

			//Copied and pasted to avoid indirection with virtual inheritance
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bWithMetadata;
		private:
			JSONSerializerBase(SerializerBase const&);
			JSONSerializerBase& operator=(SerializerBase const&);
		};

		class JSONSerializer : public JSONSerializerBase, public Serializer
		{
		public:
			using Serializer::operator%;
			using Serializer::IsLoading;

			using JSONSerializerBase::serialize;
			using JSONSerializerBase::IsBinary;
			using JSONSerializerBase::IsMongoStream;
			using JSONSerializerBase::IsReplicable;
			using JSONSerializerBase::IsValid;
			using JSONSerializerBase::Str;
			using JSONSerializerBase::Data;
			using JSONSerializerBase::GetVersion;
			using JSONSerializerBase::SetVersion;
			using JSONSerializerBase::Size;

			using JSONSerializerBase::StartObject;
			using JSONSerializerBase::EndObject;
			using JSONSerializerBase::StartArray;
			using JSONSerializerBase::EndArray;
			using JSONSerializerBase::StartMap;
			using JSONSerializerBase::EndMap;
			using JSONSerializerBase::GetNextKey;
			using JSONSerializerBase::GetDeletedKeys;

			virtual ErrorState<void> Reset() override
			{
				m_currentToken.clear();
				m_currentArrayIndex.clear();
				m_rootToken = JSONToken::object();
				m_version = 0;
				return ErrorState<void>();
			}
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<Serializer>&& var) override
			{
				sprawl::String str = var.val.Str();
				SPRAWL_RETHROW(*this % prepare_data(str, var.name, var.PersistToDB));
				return *this;
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<JSONSerializer>&& var) override
			{
				sprawl::String str = var.val.Str();
				SPRAWL_RETHROW(*this % prepare_data(str, var.name, var.PersistToDB));
				return *this;
			}

			JSONSerializer() : JSONSerializerBase(), Serializer()
			{
				if(m_bWithMetadata)
				{
					SPRAWL_ACTION_ON_ERROR(serialize(m_version, sizeof(m_version), "__version__", true), m_bIsValid = false);
				}
			}
			JSONSerializer(bool) : JSONSerializerBase(), Serializer()
			{
				m_bWithMetadata = false;
			}

			void SetPrettyPrint(bool prettyPrint)
			{
				m_prettyPrint = prettyPrint;
			}

			virtual ~JSONSerializer() {}
		protected:
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother(sprawl::String const& /*data*/) override { SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; }
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother() override { return new JSONSerializer(false); }
		};

		class JSONDeserializer : public JSONSerializerBase, public Deserializer
		{
		public:
			//Reset everything to original state.
			virtual ErrorState<void> Reset() override { SPRAWL_RETHROW(Data(m_dataStr)); return ErrorState<void>();}
			using Deserializer::operator%;
			using Deserializer::IsLoading;

			using JSONSerializerBase::serialize;
			using JSONSerializerBase::IsBinary;
			using JSONSerializerBase::IsMongoStream;
			using JSONSerializerBase::IsReplicable;
			using JSONSerializerBase::IsValid;
			using JSONSerializerBase::Str;
			using JSONSerializerBase::Data;
			using JSONSerializerBase::GetVersion;
			using JSONSerializerBase::SetVersion;
			using JSONSerializerBase::Size;

			using JSONSerializerBase::StartObject;
			using JSONSerializerBase::EndObject;
			using JSONSerializerBase::StartArray;
			using JSONSerializerBase::EndArray;
			using JSONSerializerBase::StartMap;
			using JSONSerializerBase::EndMap;
			using JSONSerializerBase::GetNextKey;
			using JSONSerializerBase::GetDeletedKeys;

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<Deserializer>&& var) override
			{
				sprawl::String str;
				SPRAWL_RETHROW(*this % str);
				SPRAWL_RETHROW(var.val.Data(str));
				return *this;
			}
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<JSONDeserializer>&& var) override
			{
				sprawl::String str;
				SPRAWL_RETHROW(*this % str);
				SPRAWL_RETHROW(var.val.Data(str));
				return *this;
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> Data(sprawl::String const& str) override
			{
				m_dataStr = str;
				m_rootToken = JSONToken::fromString(m_dataStr);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> Data(const char* data, size_t length) override
			{
				m_dataStr = sprawl::String(data, length);
				m_rootToken = JSONToken::fromString(m_dataStr);
				return ErrorState<void>();
			}


			JSONDeserializer(sprawl::String const& data) : JSONSerializerBase(), Deserializer()
			{
				SPRAWL_ACTION_ON_ERROR(Data(data), m_bIsValid = false);
			}
			JSONDeserializer(sprawl::String const& data, bool) : JSONSerializerBase(), Deserializer()
			{
				m_bWithMetadata = false;
				SPRAWL_ACTION_ON_ERROR(Data(data), m_bIsValid = false);
			}

			JSONDeserializer(const char* data, size_t length) : JSONSerializerBase(), Deserializer()
			{
				SPRAWL_ACTION_ON_ERROR(Data(data, length), m_bIsValid = false);
			}
			JSONDeserializer(const char* data, size_t length, bool) : JSONSerializerBase(), Deserializer()
			{
				m_bWithMetadata = false;
				SPRAWL_ACTION_ON_ERROR(Data(data, length), m_bIsValid = false);
			}

			JSONDeserializer() : JSONSerializerBase(), Deserializer()
			{
				m_bIsValid = false;
			}

			JSONDeserializer(bool) : JSONSerializerBase(), Deserializer()
			{
				m_bWithMetadata = false;
				m_bIsValid = false;
			}
			virtual ~JSONDeserializer(){}
		protected:
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother(sprawl::String const& data) override { return new JSONDeserializer(data, false); }
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother() override { SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; }
		private:
			sprawl::String m_dataStr;
		};
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
