#include "JSONSerializer.hpp"
#include "../common/compat.hpp"
#include <cmath>

#define SPRAWL_STRICT_JSON

namespace sprawl
{
	namespace serialization
	{
		JSONToken JSONToken::staticEmpty(JSONToken::JSONType::Empty);

		const JSONToken& JSONToken::operator[](sprawl::String const& key) const
		{
			if( m_holder->m_type != JSONType::Object )
			{
				return staticEmpty;
			}

			StringData keyData(key.c_str(), key.length());
			auto it = m_holder->m_objectChildren->find( keyData );
			if( it == m_holder->m_objectChildren->end() )
			{
				return staticEmpty;
			}

			return *it.Value();
		}

		const JSONToken& JSONToken::operator[](size_t index) const
		{
			if( m_holder->m_type != JSONType::Array || index >= m_holder->m_arrayChildren->size() )
			{
				return staticEmpty;
			}

			return (*m_holder->m_arrayChildren)[index];
		}

		JSONToken& JSONToken::operator[](sprawl::String const& key)
		{
			EnsureOwnership();
			StringData keyData(key.c_str(), key.length());
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			auto it = m_holder->m_objectChildren->find( keyData );
			if( it == m_holder->m_objectChildren->end() )
			{
				JSONToken* newToken = JSONToken::Create();
				new (newToken) JSONToken(JSONType::Empty);
				newToken->m_key = StringData( key.c_str(), key.length() );
				newToken->m_key.CommitStorage();
				newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
				return *newToken;
			}

			return *it.Value();
		}

		JSONToken& JSONToken::operator[](size_t index)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array || index >= m_holder->m_arrayChildren->size() )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			return (*m_holder->m_arrayChildren)[index];
		}

		size_t JSONToken::Size()
		{
			switch( m_holder->m_type )
			{
			case JSONType::Object:
			{
				return m_holder->m_objectChildren->size();
			}
			case JSONType::Array:
			{
				return m_holder->m_arrayChildren->size();
			}
			case JSONType::Null:
			case JSONType::Empty:
			{
				return 0;
			}
			case JSONType::Boolean:
			case JSONType::Double:
			case JSONType::Integer:
			case JSONType::String:
			default:
			{
				return 1;
			}
			}
		}

		sprawl::String JSONToken::ToJSONString(bool pretty)
		{
			sprawl::StringBuilder outString;
			BuildJSONString(outString, pretty, 1);
			return outString.Str();
		}

		JSONToken& JSONToken::PushBack(JSONToken const& token)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( token );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::PushBack(unsigned long long value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( JSONToken( JSONType::Integer, value ) );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::PushBack(long long value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( JSONToken( JSONType::Integer, value ) );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::PushBack(long double value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( JSONToken( JSONType::Double, value ) );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::PushBack(bool value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( JSONToken( JSONType::Boolean, value ) );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::PushBack(const char* const value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( JSONToken( JSONType::String, sprawl::String(value) ) );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::PushBack(const char* const value, size_t length)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( JSONToken( JSONType::String, sprawl::String(value, length) ) );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::PushBack(sprawl::String const& value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren->push_back( JSONToken( JSONType::String, value ) );
			return m_holder->m_arrayChildren->back();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, JSONToken const& token)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(token);
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken& JSONToken::Insert(const sprawl::String& name, unsigned long long value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(JSONType::Integer, value);
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken& JSONToken::Insert(const sprawl::String& name, long long value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(JSONType::Integer, value);
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken& JSONToken::Insert(const sprawl::String& name, long double value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(JSONType::Double, value);
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken& JSONToken::Insert(const sprawl::String& name, bool value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(JSONType::Boolean, value);
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken& JSONToken::Insert(const sprawl::String& name, const char* const value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(JSONType::String, sprawl::String(value));
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken& JSONToken::Insert(const sprawl::String& name, const char* const value, size_t length)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(JSONType::String, sprawl::String(value, length) );
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken& JSONToken::Insert(const sprawl::String& name, const sprawl::String& value)
		{
			EnsureOwnership();
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			JSONToken* newToken = JSONToken::Create();
			new (newToken) JSONToken(JSONType::String, value );
			newToken->m_key = StringData(name.c_str(), name.length());
			newToken->m_key.CommitStorage();
			newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
			return *newToken;
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, sprawl::String const& data)
			: m_holder(Holder::Create())
			, m_key(nullptr, 0)
		{
			::new(m_holder) Holder(statedType);
			m_holder->m_data = StringData(data.c_str(), data.length());
			m_holder->m_data.CommitStorage();
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, bool data)
			: m_holder(Holder::Create())
			, m_key(nullptr, 0)
		{
			::new(m_holder) Holder(statedType);
			if(data)
			{
				m_holder->m_data = StringData( "true", 4 );
			}
			else
			{
				m_holder->m_data = StringData( "false", 5 );
			}
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType)
			: m_holder(Holder::Create())
			, m_key(nullptr, 0)
		{
			::new(m_holder) Holder(statedType);
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, long long data)
			: m_holder(Holder::Create())
			, m_key(nullptr, 0)
		{
			::new(m_holder) Holder(statedType);
			char buf[128];
#ifdef _WIN32
			_snprintf( buf, 128, "%lld", data );
#else
			snprintf( buf, 128, "%lld", data );
#endif
			m_holder->m_data = StringData( buf, strlen( buf ) );
			m_holder->m_data.CommitStorage();
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, unsigned long long data)
			: m_holder(Holder::Create())
			, m_key(nullptr, 0)
		{
			::new(m_holder) Holder(statedType);
			char buf[128];
#ifdef _WIN32
			_snprintf( buf, 128, "%llu", data );
#else
			snprintf( buf, 128, "%llu", data );
#endif
			m_holder->m_data = StringData( buf, strlen( buf ) );
			m_holder->m_data.CommitStorage();
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, long double data)
			: m_holder(Holder::Create())
			, m_key(nullptr, 0)
		{
			::new(m_holder) Holder(statedType);
			char buf[128];
#ifdef _WIN32
			_snprintf( buf, 128, "%.20Lg", data );
#else
			snprintf( buf, 128, "%.20Lg", data );
#endif
			m_holder->m_data = StringData( buf, strlen( buf ) );
			m_holder->m_data.CommitStorage();
		}

		JSONToken* JSONToken::Create()
		{
			typedef memory::DynamicPoolAllocator<sizeof(JSONToken)> allocator;

			return (JSONToken*)allocator::alloc();
		}

		void JSONToken::Free(JSONToken* token)
		{
			typedef memory::DynamicPoolAllocator<sizeof(JSONToken)> allocator;

			token->~JSONToken();
			allocator::free(token);
		}

		void JSONToken::BuildJSONString(StringBuilder& outString, bool pretty, int tabDepth)
		{
			if( m_holder->m_type == JSONType::Object )
			{
				outString << "{";
				if( pretty )
				{
					outString << "\n";
				}
				else
				{
					outString << " ";
				}
				bool first = true;
				for( auto kvp = m_holder->m_objectChildren->begin(); kvp != m_holder->m_objectChildren->end(); ++kvp )
				{
					if(kvp.Value()->IsEmpty())
					{
						continue;
					}

					if( !first )
					{
						outString << ",";
						if( pretty )
						{
							outString << "\n";
						}
						else
						{
							outString << " ";
						}
					}

					if( pretty )
					{
						for( int i = 0; i < tabDepth; ++i )
						{
							outString << "\t";
						}
					}
					outString << "\"";
					outString << sprawl::String( sprawl::StringRef( kvp.Key().c_str(), kvp.Key().length() ) );
					outString << "\" : ";
					kvp.Value()->BuildJSONString( outString, pretty, tabDepth + 1 );
					first = false;
				}
				if( pretty )
				{
					outString << "\n";

					for( int i = 0; i < tabDepth - 1; ++i )
					{
						outString << "\t";
					}
				}
				else
				{
					outString << " ";
				}

				outString << "}";
			}
			else if( m_holder->m_type == JSONType::Array )
			{
				outString << "[";
				if( pretty )
				{
					outString << "\n";
				}
				else
				{
					outString << " ";
				}
				bool first = true;
				for( auto& it : *m_holder->m_arrayChildren )
				{
					if(it.IsEmpty())
					{
						continue;
					}
					if( !first )
					{
						outString << ",";
						if( pretty )
						{
							outString << "\n";
						}
						else
						{
							outString << " ";
						}
					}

					if( pretty )
					{
						for( int i = 0; i < tabDepth; ++i )
						{
							outString << "\t";
						}
					}
					it.BuildJSONString( outString, pretty, tabDepth + 1 );
					first = false;
				}
				if( pretty )
				{
					outString << "\n";

					for( int i = 0; i < tabDepth - 1; ++i )
					{
						outString << "\t";
					}
				}
				else
				{
					outString << " ";
				}

				outString << "]";
			}
			else if( m_holder->m_type == JSONType::Null )
			{
				outString << "null";
			}
			else if(m_holder->m_type == JSONType::String)
			{
				outString << "\"";
				outString << EscapeString(m_holder->m_data);
				outString << "\"";
			}
			else
			{
				outString << sprawl::String( sprawl::StringRef( m_holder->m_data.c_str(), m_holder->m_data.length() ) );
			}
		}

		/// TODO PERFORMANCE: Replace sprawl::String here with a non-ref-counted string class that will simply store a pointer and a length with no allocations

		void JSONToken::SkipWhitespace(const char*& data)
		{
			while(*data == ' ' || *data == '\t' || *data == '\n' || *data == '\r')
				++data;
		}

		void JSONToken::CollectString(const char*& data)
		{
			++data;

			while(*data != '\"')
			{
				if(SPRAWL_UNLIKELY(*data == '\\'))
				{
					++data;
				}
				++data;
			}
			++data;
		}

		void JSONToken::ParseString(const char*& data)
		{
			char const* startPoint = data + 1;

			CollectString(data);

			m_holder->m_data = StringData(startPoint, data - startPoint - 1);
		}

		void JSONToken::ParseNumber(const char*& data)
		{
			char const* startPoint = data;

			for(;;)
			{
				switch( *data )
				{
				case '-':
				case '+':
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				{
					++data;
					break;
				}
				case '.':
				case 'e':
				case 'E':
				{
					if( m_holder->m_type == JSONType::Integer )
					{
						m_holder->m_type = JSONType::Double;
					}
					++data;
				break;
				}
				default:
				{
					goto Done;
				}
				}
			}

			Done:

			m_holder->m_data = StringData(startPoint, data - startPoint);
		}

		void JSONToken::ParseBool(const char*& data)
		{
			if(
				*data == 't'
#ifdef SPRAWL_STRICT_JSON
				&& *(data + 1) == 'r'
				&& *(data + 2) == 'u'
				&& *(data + 3) == 'e'
#endif
			)
			{
				m_holder->m_data = StringData(data, 4);
				data += 4;
			}
			else if(
				*data == 'f'
#ifdef SPRAWL_STRICT_JSON
				&& *(data + 1) == 'a'
				&& *(data + 2) == 'l'
				&& *(data + 3) == 's'
				&& *(data + 4) == 'e'
#endif
			)
			{
				m_holder->m_data = StringData(data, 5);
				data += 5;
			}
#ifdef SPRAWL_STRICT_JSON
			else
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), );
			}
#endif
			return;
		}

		void JSONToken::ParseArray(const char*& data)
		{
			++data;

			for(;;)
			{
				SkipWhitespace(data);
				if(*data == ']')
				{
					++data;
					return;
				}
				switch(*data)
				{
				case '{':
				{
					m_holder->m_arrayChildren->push_back( JSONToken( StringData(nullptr, 0), data, JSONType::Object ) );
					break;
				}
				case '[':
				{
					m_holder->m_arrayChildren->push_back( JSONToken( StringData(nullptr, 0), data, JSONType::Array ) );
					break;
				}
				case '-':
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				{
					m_holder->m_arrayChildren->push_back( JSONToken( StringData(nullptr, 0), data, JSONType::Integer ) );
					break;
				}
				case '\"':
				{
					m_holder->m_arrayChildren->push_back( JSONToken( StringData(nullptr, 0), data, JSONType::String ) );
					break;
				}
				case 't':
				case 'f':
				{
					m_holder->m_arrayChildren->push_back( JSONToken( StringData(nullptr, 0), data, JSONType::Boolean ) );
					break;
				}
				case 'n':
				{
					m_holder->m_arrayChildren->push_back( JSONToken( JSONType::Null ) );
					data += 4;
					break;
				}
#ifdef SPRAWL_STRICT_JSON
				default:
				{
					abort();
				}
#endif
				}

				SkipWhitespace(data);

#ifdef SPRAWL_STRICT_JSON
				if(*data != ',' && *data != ']')
				{
					abort();
				}
#endif

				if(*data == ']')
				{
					++data;
					break;
				}
				++data;
			}
		}

		void JSONToken::ParseObject(const char*& data)
		{
			++data;

			StringData key(nullptr, 0);

			for(;;)
			{
				SkipWhitespace(data);

				if(*data == '}')
				{
					++data;
					return;
				}

#ifdef SPRAWL_STRICT_JSON
				if(*data != '\"')
				{
					abort();
				}
#endif

				char const* keyStart = nullptr;
				char const* keyEnd = nullptr;

				keyStart = data + 1;

				CollectString(data);

				keyEnd = data - 1;
				key = StringData( keyStart, keyEnd - keyStart );

				SkipWhitespace(data);

#ifdef SPRAWL_STRICT_JSON
				if(*data != ':')
				{
					abort();
				}
#endif

				++data;

				SkipWhitespace(data);

				switch(*data)
				{
				case '{':
				{
					JSONToken* newToken = JSONToken::Create();
					new (newToken) JSONToken( key, data, JSONType::Object );
					newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
					break;
				}
				case '[':
				{
					JSONToken* newToken = JSONToken::Create();
					new (newToken) JSONToken( key, data, JSONType::Array );
					newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
					break;
				}
				case '-':
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				{
					JSONToken* newToken = JSONToken::Create();
					new (newToken) JSONToken( key, data, JSONType::Integer );
					newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
					break;
				}
				case '\"':
				{
					JSONToken* newToken = JSONToken::Create();
					new (newToken) JSONToken( key, data, JSONType::String );
					newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
					break;
				}
				case 't':
				case 'f':
				{
					JSONToken* newToken = JSONToken::Create();
					new (newToken) JSONToken( key, data, JSONType::Boolean );
					newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
					break;
				}
				case 'n':
				{
					JSONToken* newToken = JSONToken::Create();
					new (newToken) JSONToken( JSONType::Null );
					newToken->m_holder->m_iter = m_holder->m_objectChildren->insert( newToken );
#ifdef SPRAWL_STRICT_JSON
					if(*(data + 1) != 'u' || *(data + 2) != 'l' || *(data + 3) != 'l')
					{
						abort();
					}
#endif
					data += 4;
					break;
				}
#ifdef SPRAWL_STRICT_JSON
				default:
				{
					abort();
				}
#endif
				}

				SkipWhitespace(data);

#ifdef SPRAWL_STRICT_JSON
				if(*data != ',' && *data != '}')
				{
					abort();
				}
#endif

				if(*data == '}')
				{
					++data;
					break;
				}
				++data;
			}
		}

		JSONToken::JSONToken(StringData const& myKey, const char*& data, JSONToken::JSONType expectedType)
			: m_holder(Holder::Create())
			, m_key(myKey)
		{
			::new(m_holder) Holder(expectedType);

			switch(expectedType)
			{
			case JSONType::Boolean: ParseBool(data); break;
			case JSONType::Integer: ParseNumber(data); break;
			case JSONType::String: ParseString(data); break;
			case JSONType::Array: ParseArray(data); break;
			case JSONType::Object: ParseObject(data); break;
			default: break;
			}
		}

		JSONToken::~JSONToken()
		{
			DecRef();
		}

		JSONToken::JSONToken()
			: m_holder(Holder::Create())
			, m_key(nullptr, 0)
		{
			::new(m_holder) Holder(JSONType::Empty);
		}

		JSONToken::JSONToken(const JSONToken& other)
			: m_holder(other.m_holder)
			, m_key(nullptr, 0)
		{
			IncRef();
		}

		void JSONToken::EnsureOwnership()
		{
			if(!m_holder->Unique())
			{
				Holder* newHolder = Holder::Create();
				::new(newHolder) Holder(m_holder->m_type);
				//We do not want to copy object children directly. They need cleanup work.
				newHolder->m_data = m_holder->m_data;
				newHolder->m_iter = m_holder->m_iter;
				*newHolder->m_arrayChildren = *m_holder->m_arrayChildren;
				if(newHolder->m_type == JSONType::Object)
				{
					for(auto kvp = m_holder->m_objectChildren->begin(); kvp; ++kvp)
					{
						JSONToken* newToken = JSONToken::Create();
						new (newToken) JSONToken(*kvp.Value());
						newToken->m_key = kvp.Key();
						newToken->m_holder->m_iter = newHolder->m_objectChildren->insert( newToken );
					}
				}
				DecRef();
				m_holder = newHolder;
			}
		}

		JSONToken& JSONToken::operator=(const JSONToken& other)
		{
			m_holder = other.m_holder;
			IncRef();
			return *this;
		}

		long long JSONToken::ToInt(StringData const& str)
		{
			long long result = 0;
			size_t index = 0;
			bool negative = false;
			char const* const data = str.c_str();
			size_t const length = str.length();

			if( *data == '-' )
			{
				negative = true;
				++index;
			}
			for( ; index < length; ++index )
			{
				result *= 10;
				result += ( (int)( data[index] ) - 48 );
			}

			if( negative )
			{
				result *= -1;
			}

			return result;
		}

		unsigned long long JSONToken::ToUInt(StringData const& str)
		{
			unsigned long long result = 0;
			size_t index = 0;

			char const* const data = str.c_str();
			size_t const length = str.length();

			for( ; index < length; ++index )
			{
				result *= 10;
				result += ( (int)( data[index] ) - 48 );
			}

			return result;
		}

		long double JSONToken::ToDouble(StringData const& str)
		{
			long double result = 0;
			size_t index = 0;
			bool negative = false;
			double fractionSize = 1.0;
			bool inFraction = false;
			char const* const data = str.c_str();
			size_t const length = str.length();

			bool exp = false;
			double expVal = 0;
			bool expNegative = false;

			if( *data == '-' )
			{
				negative = true;
				++index;
			}
			for( ; index < length; ++index )
			{
				char c = data[index];
				if( c == '.' )
				{
					inFraction = true;
					continue;
				}
				if(c == 'e' || c == 'E')
				{
					exp = true;
					if(index != length - 1)
					{
						if(data[index+1] == '-')
						{
							++index;
							expNegative = true;
						}
						else if(data[index+1] == '+')
						{
							++index;
						}
						continue;
					}
					continue;
				}
				if(exp)
				{
					expVal *= 10;
					expVal += ( (int)( data[index] ) - 48 );
				}
				else
				{
					result *= 10;
					result += ( (int)( data[index] ) - 48 );
					if( inFraction )
					{
						fractionSize *= 10.0;
					}
				}
			}

			if( negative )
			{
				result *= -1;
			}

			result /= fractionSize;

			if(exp)
			{
				double mult = pow(10.0, expVal);
				if(expNegative)
				{
					result /= mult;
				}
				else
				{
					result *= mult;
				}
			}

			return result;
		}

		bool JSONToken::ToBool(StringData const& str)
		{
			char const* const data = str.c_str();
			size_t const length = str.length();
			if(
					length == 4 &&
					data[0] == 't' &&
					data[1] == 'r' &&
					data[2] == 'u' &&
					data[3] == 'e'
					)
			{
				return true;
			}
			return false;
		}

		sprawl::String JSONToken::EscapeString(StringData const& str)
		{
			sprawl::StringBuilder builder;
			const char* data = str.c_str();
			const size_t length = str.length();
			for(size_t index = 0; index < length; ++index)
			{
				switch(data[index])
				{
				case '\"': builder << "\\\""; break;
				case '\\': builder << "\\\\"; break;
				case '\b': builder << "\\b"; break;
				case '\f': builder << "\\f"; break;
				case '\n': builder << "\\n"; break;
				case '\r': builder << "\\r"; break;
				case '\t': builder << "\\t"; break;
				default: builder << data[index]; break;
				}
			}
			return builder.Str();
		}

		sprawl::String JSONToken::UnescapeString(StringData const& str)
		{
			sprawl::StringBuilder builder;
			bool inEscape = false;
			const char* data = str.c_str();
			const size_t length = str.length();
			for(size_t index = 0; index < length; ++index)
			{
				if(inEscape)
				{
					switch(data[index])
					{
					case '\"': builder << '\"'; break;
					case '\\' : builder << '\\'; break;
					case 'b': builder << '\b'; break;
					case 'f': builder << '\f'; break;
					case 'n': builder << '\n'; break;
					case 'r': builder << '\r'; break;
					case 't': builder << '\t'; break;
					default: builder << '\\' << data[index]; break;
					}
					inEscape = false;
				}
				else
				{
					if(data[index] == '\\')
					{
						inEscape = true;
					}
					else
					{
						builder << data[index];
					}
				}
			}
			return builder.Str();
		}

		JSONToken& JSONToken::NextSibling()
		{
			EnsureOwnership();
			if(m_holder->m_iter.More())
			{
				return *m_holder->m_iter.Next().Value();
			}
			return staticEmpty;
		}

		const JSONToken& JSONToken::NextSibling() const
		{
			if(m_holder->m_iter.More())
			{
				return *m_holder->m_iter.Next().Value();
			}
			return staticEmpty;
		}

		JSONToken::Holder* JSONToken::Holder::Create()
		{
			typedef memory::DynamicPoolAllocator<sizeof(JSONToken::Holder)> holderAlloc;

			return (JSONToken::Holder*)holderAlloc::alloc();

		}

		void JSONToken::Holder::Free(JSONToken::Holder* holder)
		{
			typedef memory::DynamicPoolAllocator<sizeof(JSONToken::Holder)> holderAlloc;

			holder->~Holder();
			holderAlloc::free(holder);
		}

		JSONToken::Holder::Holder(JSONType forType)
			: m_data(nullptr, 0)
			, m_type(forType)
			, m_objectChildren(nullptr)
			, m_iter(nullptr)
			, m_arrayChildren(nullptr)
			, refCount(1)
		{
			typedef memory::DynamicPoolAllocator<sizeof(std::vector<JSONToken>)> arrayAlloc;
			typedef memory::DynamicPoolAllocator<sizeof(JSONToken::TokenMap)> mapAlloc;

			if(forType == JSONType::Array)
			{
				m_arrayChildren = (std::vector<JSONToken>*)arrayAlloc::alloc();
				::new(m_arrayChildren) std::vector<JSONToken>();
			}
			else if(forType == JSONType::Object)
			{
				m_objectChildren = (TokenMap*)mapAlloc::alloc();
				::new(m_objectChildren) TokenMap(64);
			}
		}

		JSONToken::Holder::~Holder()
		{
			typedef memory::DynamicPoolAllocator<sizeof(std::vector<JSONToken>)> arrayAlloc;
			typedef memory::DynamicPoolAllocator<sizeof(JSONToken::TokenMap)> mapAlloc;

			if(m_objectChildren)
			{
				for(auto& child : *m_objectChildren)
				{
					JSONToken::Free(child);
				}
				m_objectChildren->~TokenMap();
				mapAlloc::free(m_objectChildren);
			}

			if(m_arrayChildren)
			{
				m_arrayChildren->~vector<JSONToken>();
				arrayAlloc::free(m_arrayChildren);
			}
		}


	}
}
