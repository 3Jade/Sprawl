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

			StringData const& GetKey() const
			{
				return m_key;
			}

			long long Int() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Integer )
				{
					SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), 0);
				}
#endif

				return ToInt(m_holder->m_data);
			}

			unsigned long long Unsigned() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Integer )
				{
					SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), 0);
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

			sprawl::String String() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::String )
				{
					SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), StringLiteral(""));
				}
#endif

				return UnescapeString(m_holder->m_data);
			}

			bool Bool() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Boolean )
				{
					SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), false);
				}
#endif

				return ToBool(m_holder->m_data);
			}

			long double Double() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Double )
				{
					SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), 0.0);
				}
#endif

				return ToDouble(m_holder->m_data);
			}

			JSONToken& NextSibling();

			JSONToken const& NextSibling() const;

			JSONToken const& operator[]( sprawl::String const& key ) const;

			JSONToken const& operator[]( size_t index ) const;

			JSONToken& operator[]( sprawl::String const& key );

			JSONToken& operator[]( size_t index );

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

			JSONToken& PushBack( JSONToken const& token );
			JSONToken& PushBack( signed char value ) { return PushBack((long long)(value)); }
			JSONToken& PushBack( short value ) { return PushBack((long long)(value)); }
			JSONToken& PushBack( int value ) { return PushBack((long long)(value)); }
			JSONToken& PushBack( long value ) { return PushBack((long long)(value)); }
			JSONToken& PushBack( unsigned char value ) { return PushBack((unsigned long long)(value)); }
			JSONToken& PushBack( unsigned short value ) { return PushBack((unsigned long long)(value)); }
			JSONToken& PushBack( unsigned int value ) { return PushBack((unsigned long long)(value)); }
			JSONToken& PushBack( unsigned long value ) { return PushBack((unsigned long long)(value)); }
			JSONToken& PushBack( unsigned long long value );
			JSONToken& PushBack( long long value );
			JSONToken& PushBack( float value ) { return PushBack((long double)(value)); }
			JSONToken& PushBack( double value ) { return PushBack((long double)(value)); }
			JSONToken& PushBack( long double value );
			JSONToken& PushBack( bool value );
			JSONToken& PushBack( const char* const value );
			JSONToken& PushBack( const char* const value, size_t length );
			JSONToken& PushBack( sprawl::String const& value );

			JSONToken& Insert( sprawl::String const& name, JSONToken const& token );
			JSONToken& Insert( sprawl::String const& name, signed char value ) { return Insert(name, (long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, short value ) { return Insert(name, (long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, int value ) { return Insert(name, (long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, long value ) { return Insert(name, (long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, unsigned char value ) { return Insert(name, (unsigned long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, unsigned short value ) { return Insert(name, (unsigned long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, unsigned int value ) { return Insert(name, (unsigned long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, unsigned long value ) { return Insert(name, (unsigned long long)(value)); }
			JSONToken& Insert( sprawl::String const& name, unsigned long long value );
			JSONToken& Insert( sprawl::String const& name, long long value );
			JSONToken& Insert( sprawl::String const& name, float value ) { return Insert(name, (long double)(value)); }
			JSONToken& Insert( sprawl::String const& name, double value ) { return Insert(name, (long double)(value)); }
			JSONToken& Insert( sprawl::String const& name, long double value );
			JSONToken& Insert( sprawl::String const& name, bool value );
			JSONToken& Insert( sprawl::String const& name, const char* const value );
			JSONToken& Insert( sprawl::String const& name, const char* const value, size_t length );
			JSONToken& Insert( sprawl::String const& name, sprawl::String const& value );

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

			JSONToken& FirstChild()
			{
				auto it = m_holder->m_objectChildren->begin();
				if(it)
				{
					return *it.Value();
				}
				return staticEmpty;
			}

		protected:
			JSONToken( StringData const& myKey, char const*& data, JSONType expectedType );
			JSONToken( JSONType statedType, sprawl::String const& data );
			JSONToken( JSONType statedType, bool data );
			JSONToken( JSONType statedType, long long data );
			JSONToken( JSONType statedType, unsigned long long data );
			JSONToken( JSONType statedType );
			JSONToken( JSONType statedType, long double data );

			static JSONToken* Create();
			static void Free(JSONToken* token);

		private:
			void BuildJSONString(sprawl::StringBuilder& outString, bool pretty, int tabDepth);
			void EnsureOwnership();
			void IncRef() { ++m_holder->refCount; }
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

			typedef sprawl::collections::HashMap<JSONToken*, sprawl::PtrConstMemberAccessor<JSONToken, StringData const&, &JSONToken::GetKey>> TokenMap;
			typedef TokenMap::iterator iterator;

			struct Holder
			{
				StringData m_data;
				JSONType m_type;
				TokenMap *m_objectChildren;
				TokenMap::iterator m_iter;
				std::vector<JSONToken> *m_arrayChildren;
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
			virtual void Reset() override { }
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
			void serialize_impl(T* var, const uint32_t /*bytes*/, sprawl::String const& name, bool)
			{
				JSONToken* currentToken = m_currentToken.back();
				if(IsSaving())
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						currentToken->PushBack(*var);
					}
					else
					{
						currentToken->Insert(name, *var);
					}
				}
				else
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						currentToken->operator[](m_currentArrayIndex.back()++).Val(*var);
					}
					else
					{
						JSONToken& token = currentToken->operator[](name);
						if(token.IsEmpty())
						{
							SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), );
						}
						token.Val(*var);
						m_currentObjectIndex.back() = &token.NextSibling();
					}
				}
			}

			void serialize_impl(char* var, const uint32_t bytes, sprawl::String const& name, bool)
			{
				JSONToken* currentToken = m_currentToken.back();
				if(IsSaving())
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						currentToken->PushBack(var, bytes);
					}
					else
					{
						currentToken->Insert(name, var, bytes);
					}
				}
				else
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						currentToken->operator[](m_currentArrayIndex.back()++).Val(var, bytes);
					}
					else
					{
						JSONToken& token = currentToken->operator[](name);
						if(token.IsEmpty())
						{
							SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), );
						}
						token.Val(var, bytes);
						m_currentObjectIndex.back() = &token.NextSibling();
					}
				}
			}

			void serialize_impl(std::string* var, const uint32_t /*bytes*/, sprawl::String const& name, bool)
			{
				JSONToken* currentToken = m_currentToken.back();
				if(IsSaving())
				{
					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						currentToken->PushBack(var->c_str(), var->length());
					}
					else
					{
						currentToken->Insert(name, var->c_str(), var->length());
					}
				}
				else
				{
					sprawl::String str;

					if(currentToken->Type() == JSONToken::JSONType::Array)
					{
						currentToken->operator[](m_currentArrayIndex.back()++).Val(str);
					}
					else
					{
						JSONToken& token = currentToken->operator[](name);
						if(token.IsEmpty())
						{
							SPRAWL_THROW_EXCEPTION(ex_serializer_overflow(), );
						}
						token.Val(str);
						m_currentObjectIndex.back() = &token.NextSibling();
					}

					var->assign(str.c_str(), str.length());
				}
			}

		public:
			virtual void serialize(int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB)  override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(short int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(char* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(float* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(double* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long double* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(bool* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned short int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned char* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(std::string* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(sprawl::String* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual uint32_t StartObject(sprawl::String const& str, bool = true) override
			{
				JSONToken& token = *m_currentToken.back();
				if(IsLoading())
				{
					if(token.Type() == JSONToken::JSONType::Array)
					{
						m_currentToken.push_back(&token[m_currentArrayIndex.back()]);
					}
					else
					{
						m_currentToken.push_back(&token[str]);
					}
					m_currentObjectIndex.push_back(&m_currentToken.back()->FirstChild());
					return uint32_t(m_currentToken.back()->Size());
				}
				else
				{
					if(token.Type() == JSONToken::JSONType::Array)
					{
						m_currentToken.push_back(&token.PushBack(JSONToken::object()));
					}
					else
					{
						m_currentToken.push_back(&token.Insert(str, JSONToken::object()));
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
						m_currentObjectIndex.back() = &m_currentObjectIndex.back()->NextSibling();
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
						m_currentToken.push_back(&token[m_currentArrayIndex.back()]);
					}
					else
					{
						m_currentToken.push_back(&token[str]);
					}
					m_currentArrayIndex.push_back(0);
					size = uint32_t(m_currentToken.back()->Size());
				}
				else
				{
					if(token.Type() == JSONToken::JSONType::Array)
					{
						m_currentToken.push_back(&token.PushBack(JSONToken::array()));
					}
					else
					{
						m_currentToken.push_back(&token.Insert(str, JSONToken::array()));
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
						m_currentObjectIndex.back() = &m_currentObjectIndex.back()->NextSibling();
					}
					else
					{
						++m_currentArrayIndex.back();
					}
				}
			}

			sprawl::String GetNextKey()
			{
				return m_currentObjectIndex.back()->GetKey().toString();
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
				m_currentObjectIndex.push_back(&m_rootToken.FirstChild());
			}

			virtual ~JSONSerializerBase()
			{
				//
			}

			JSONToken m_rootToken;
			std::vector<JSONToken*> m_currentToken;
			std::vector<int> m_currentArrayIndex;
			std::vector<JSONToken*> m_currentObjectIndex;

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

			virtual void Reset() override
			{
				m_currentToken.clear();
				m_currentArrayIndex.clear();
				m_rootToken = JSONToken::object();
				m_version = 0;
			}
			virtual SerializerBase& operator%(SerializationData<Serializer>&& var) override
			{
				sprawl::String str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase& operator%(SerializationData<JSONSerializer>&& var) override
			{
				sprawl::String str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			JSONSerializer() : JSONSerializerBase(), Serializer()
			{
				if(m_bWithMetadata)
					serialize(m_version, sizeof(m_version), "__version__", true);
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
			virtual SerializerBase* GetAnother(sprawl::String const& /*data*/) override { SPRAWL_THROW_EXCEPTION(std::exception(), nullptr); }
			virtual SerializerBase* GetAnother() override { return new JSONSerializer(false); }
		};

		class JSONDeserializer : public JSONSerializerBase, public Deserializer
		{
		public:
			//Reset everything to original state.
			virtual void Reset() override { Data(m_dataStr); }
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

			virtual SerializerBase& operator%(SerializationData<Deserializer>&& var) override
			{
				sprawl::String str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase& operator%(SerializationData<JSONDeserializer>&& var) override
			{
				sprawl::String str;
				*this % str;
				var.val.Data(str);
				return *this;
			}

			virtual void Data(sprawl::String const& str) override
			{
				m_dataStr = str;
				m_rootToken = JSONToken::fromString(m_dataStr);
			}

			virtual void Data(const char* data, size_t length) override
			{
				m_dataStr = sprawl::String(data, length);
				m_rootToken = JSONToken::fromString(m_dataStr);
			}


			JSONDeserializer(sprawl::String const& data) : JSONSerializerBase(), Deserializer()
			{
				Data(data);
			}
			JSONDeserializer(sprawl::String const& data, bool) : JSONSerializerBase(), Deserializer()
			{
				m_bWithMetadata = false;
				Data(data);
			}

			JSONDeserializer(const char* data, size_t length) : JSONSerializerBase(), Deserializer()
			{
				Data(data, length);
			}
			JSONDeserializer(const char* data, size_t length, bool) : JSONSerializerBase(), Deserializer()
			{
				m_bWithMetadata = false;
				Data(data, length);
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
			virtual SerializerBase* GetAnother(sprawl::String const& data) override { return new JSONDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { SPRAWL_THROW_EXCEPTION(std::exception(), nullptr); }
		private:
			sprawl::String m_dataStr;
		};
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
