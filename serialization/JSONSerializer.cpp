#include "JSONSerializer.hpp"
#include "../common/compat.hpp"
#include <cmath>
#include <stdlib.h>

#define SPRAWL_STRICT_JSON

namespace sprawl
{
	namespace serialization
	{
		JSONToken JSONToken::staticEmpty(JSONToken::JSONType::Empty);

		JSONToken const& JSONToken::operator[](sprawl::String const& key) const
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

			return it.Value();
		}

		JSONToken const& JSONToken::operator[](ssize_t index) const
		{
			if( m_holder->m_type != JSONType::Array || index >= m_holder->m_arrayChildren.Size() )
			{
				return staticEmpty;
			}

			return m_holder->m_arrayChildren[index];
		}

		JSONToken& JSONToken::operator[](sprawl::String const& key)
		{
			StringData keyData(key.c_str(), key.length());
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			auto it = m_holder->m_objectChildren->find( keyData );
			if( it == m_holder->m_objectChildren->end() )
			{
				keyData.CommitStorage();
				auto it = m_holder->m_objectChildren->Insert( JSONToken( keyData, JSONType::Empty ) );
				return it.Value();
			}

			return it.Value();
		}

		JSONToken& JSONToken::operator[](ssize_t index)
		{
			if( m_holder->m_type != JSONType::Array || index >= m_holder->m_arrayChildren.Size() )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			return m_holder->m_arrayChildren[index];
		}

		size_t JSONToken::Size()
		{
			switch( m_holder->m_type )
			{
			case JSONType::Object:
			{
				return m_holder->m_objectChildren->Size();
			}
			case JSONType::Array:
			{
				return size_t(m_holder->m_arrayChildren.Size());
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
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( token );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::PushBack(unsigned long long value)
		{
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( JSONType::Integer, value );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::PushBack(long long value)
		{
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( JSONType::Integer, value );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::PushBack(long double value)
		{
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( JSONType::Double, value );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::PushBack(bool value)
		{
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( JSONType::Boolean, value );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::PushBack(const char* const value)
		{
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( JSONType::String, sprawl::String(value) );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::PushBack(const char* const value, size_t length)
		{
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( JSONType::String, sprawl::String(value, length) );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::PushBack(sprawl::String const& value)
		{
			if( m_holder->m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}
			m_holder->m_arrayChildren.EmplaceBack( JSONType::String, value );
			return m_holder->m_arrayChildren.Back();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, JSONToken const& token)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, token) );
			return it.Value();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, unsigned long long value)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, JSONType::Integer, value) );
			return it.Value();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, long long value)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, JSONType::Integer, value) );
			return it.Value();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, long double value)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, JSONType::Double, value) );
			return it.Value();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, bool value)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, JSONType::Boolean, value) );
			return it.Value();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, const char* const value)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, JSONType::String, sprawl::String(value)) );
			return it.Value();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, const char* const value, size_t length)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, JSONType::String, sprawl::String(value, length)) );
			return it.Value();
		}

		JSONToken& JSONToken::Insert(sprawl::String const& name, sprawl::String const& value)
		{
			if( m_holder->m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data(), staticEmpty);
			}

			StringData nameData(name.c_str(), name.length());
			nameData.CommitStorage();
			auto it = m_holder->m_objectChildren->Insert( JSONToken(nameData, JSONType::String, value) );
			return it.Value();
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



		JSONToken::JSONToken(StringData const& myKey, JSONToken::JSONType statedType, sprawl::String const& data)
			: m_holder(Holder::Create())
			, m_key(myKey)
		{
			m_key.CommitStorage();
			::new(m_holder) Holder(statedType);
			m_holder->m_data = StringData(data.c_str(), data.length());
			m_holder->m_data.CommitStorage();
		}

		JSONToken::JSONToken(StringData const& myKey, JSONToken::JSONType statedType, bool data)
			: m_holder(Holder::Create())
			, m_key(myKey)
		{
			m_key.CommitStorage();
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

		JSONToken::JSONToken(StringData const& myKey, JSONToken::JSONType statedType)
			: m_holder(Holder::Create())
			, m_key(myKey)
		{
			m_key.CommitStorage();
			::new(m_holder) Holder(statedType);
		}

		JSONToken::JSONToken(StringData const& myKey, JSONToken::JSONType statedType, long long data)
			: m_holder(Holder::Create())
			, m_key(myKey)
		{
			m_key.CommitStorage();
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

		JSONToken::JSONToken(StringData const& myKey, JSONToken::JSONType statedType, unsigned long long data)
			: m_holder(Holder::Create())
			, m_key(myKey)
		{
			m_key.CommitStorage();
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

		JSONToken::JSONToken(StringData const& myKey, JSONToken::JSONType statedType, long double data)
			: m_holder(Holder::Create())
			, m_key(myKey)
		{
			m_key.CommitStorage();
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
			typedef memory::PoolAllocator<sizeof(JSONToken)> allocator;

			return (JSONToken*)allocator::alloc();
		}

		void JSONToken::Free(JSONToken* token)
		{
			typedef memory::PoolAllocator<sizeof(JSONToken)> allocator;

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
				for( auto kvp : *m_holder->m_objectChildren )
				{
					if(kvp.Value().IsEmpty())
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
					kvp.Value().BuildJSONString( outString, pretty, tabDepth + 1 );
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
				for( auto& it : m_holder->m_arrayChildren )
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

		void JSONToken::SkipWhitespace(char const*& data)
		{
			while(*data == ' ' || *data == '\t' || *data == '\n' || *data == '\r')
				++data;
		}

		void JSONToken::CollectString(char const*& data)
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

		void JSONToken::ParseString(char const*& data)
		{
			char const* startPoint = data + 1;

			CollectString(data);

			m_holder->m_data = StringData(startPoint, data - startPoint - 1);
		}

		void JSONToken::ParseNumber(char const*& data)
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
						m_holder->m_data = StringData(startPoint, data - startPoint);
						return;
					}
				}
			}
		}

		void JSONToken::ParseBool(char const*& data)
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

		void JSONToken::ParseArray(char const*& data)
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
					m_holder->m_arrayChildren.EmplaceBack( StringData(nullptr, 0), data, JSONType::Object );
					break;
				}
				case '[':
				{
					m_holder->m_arrayChildren.EmplaceBack( StringData(nullptr, 0), data, JSONType::Array );
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
					m_holder->m_arrayChildren.EmplaceBack( StringData(nullptr, 0), data, JSONType::Integer );
					break;
				}
				case '\"':
				{
					m_holder->m_arrayChildren.EmplaceBack( StringData(nullptr, 0), data, JSONType::String );
					break;
				}
				case 't':
				case 'f':
				{
					m_holder->m_arrayChildren.EmplaceBack( StringData(nullptr, 0), data, JSONType::Boolean );
					break;
				}
				case 'n':
				{
					m_holder->m_arrayChildren.EmplaceBack( JSONType::Null );
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

		void JSONToken::ParseObject(char const*& data)
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
					m_holder->m_objectChildren->Insert( JSONToken( key, data, JSONType::Object ) );
					break;
				}
				case '[':
				{
					m_holder->m_objectChildren->Insert( JSONToken( key, data, JSONType::Array ) );
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
					m_holder->m_objectChildren->Insert( JSONToken( key, data, JSONType::Integer ) );
					break;
				}
				case '\"':
				{
					m_holder->m_objectChildren->Insert( JSONToken( key, data, JSONType::String ) );
					break;
				}
				case 't':
				case 'f':
				{
					m_holder->m_objectChildren->Insert( JSONToken( key, data, JSONType::Boolean ) );
					break;
				}
				case 'n':
				{
					m_holder->m_objectChildren->Insert( JSONToken( JSONType::Null ) );
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

		JSONToken::JSONToken(StringData const& myKey, char const*& data, JSONToken::JSONType expectedType)
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
			// "Empty" and "Null" have no content to parse.
			// "Double" will never actually show up here - it will begin its life as "Integer" and grow into "Double" later!
			case JSONType::Empty: case JSONType::Double: case JSONType::Null: default: break;
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

		JSONToken::JSONToken(JSONToken const& other)
			: m_holder(other.m_holder)
			, m_key(other.m_key)
		{
			IncRef();
		}

		JSONToken::JSONToken(StringData const& myKey, JSONToken const& other)
			: m_holder(other.m_holder)
			, m_key(myKey)
		{
			IncRef();
		}

		JSONToken JSONToken::ShallowCopy()
		{
			JSONToken ret(m_key, m_holder->m_type);
			Holder* newHolder = ret.m_holder;
			//We do not want to copy object children directly. They need cleanup work.
			newHolder->m_data = m_holder->m_data;
			if(newHolder->m_type == JSONType::Array)
			{
				newHolder->m_arrayChildren = m_holder->m_arrayChildren;
			}
			if(newHolder->m_type == JSONType::Object)
			{
				for(auto kvp = m_holder->m_objectChildren->begin(); kvp; ++kvp)
				{
					newHolder->m_objectChildren->Insert( JSONToken(kvp.Key(), kvp.Value()) );
				}
			}
			return std::move(ret);
		}

		JSONToken JSONToken::DeepCopy()
		{
			JSONToken ret(m_key, m_holder->m_type);
			Holder* newHolder = ret.m_holder;
			//We do not want to copy object children directly. They need cleanup work.
			newHolder->m_data = m_holder->m_data;
			if(newHolder->m_type == JSONType::Array)
			{
				for(int i = 0; i < m_holder->m_arrayChildren.Size(); ++i)
				{
					newHolder->m_arrayChildren.EmplaceBack(std::move(m_holder->m_arrayChildren[i].DeepCopy()));
				}
			}
			if(newHolder->m_type == JSONType::Object)
			{
				for(auto kvp = m_holder->m_objectChildren->begin(); kvp; ++kvp)
				{
					newHolder->m_objectChildren->Insert( kvp.Value().DeepCopy() );
				}
			}
			return std::move(ret);
		}

		JSONToken& JSONToken::operator=(JSONToken const& other)
		{
			DecRef();
			m_holder = other.m_holder;
			IncRef();
			return *this;
		}

		JSONToken::iterator JSONToken::begin()
		{
			if(m_holder->m_type == JSONType::Array)
			{
				return iterator(m_holder->m_arrayChildren.begin());
			}
			else if(m_holder->m_type == JSONType::Object)
			{
				return iterator(m_holder->m_objectChildren->begin());
			}
			return iterator(nullptr);
		}

		JSONToken::iterator JSONToken::end()
		{
			if(m_holder->m_type == JSONType::Array)
			{
				return iterator(m_holder->m_arrayChildren.end());
			}
			else if(m_holder->m_type == JSONType::Object)
			{
				return iterator(m_holder->m_objectChildren->end());
			}
			return iterator(nullptr);
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
					case '/' : builder << '/'; break;
					case 'b': builder << '\b'; break;
					case 'f': builder << '\f'; break;
					case 'n': builder << '\n'; break;
					case 'r': builder << '\r'; break;
					case 't': builder << '\t'; break;
					case 'u':
					{
						++index;
						char digits[5];
						memcpy(digits, &data[index], 5);
						digits[4] = '\0';
						union UnicodeChar
						{
							int32_t asInt;
							char asChar[4];
						} ch;
						ch.asInt= strtol(digits, nullptr, 16);

						if (ch.asInt < 0x80) {
							builder << ch.asChar[0];
						}
						else if (ch.asInt < 0x800) {
							builder << char((ch.asInt>>6) | 0xC0);
							builder << char((ch.asInt & 0x3F) | 0x80);
						}
						else if (ch.asInt < 0x10000) {
							builder << char((ch.asInt>>12) | 0xE0);
							builder << char(((ch.asInt>>6) & 0x3F) | 0x80);
							builder << char((ch.asInt & 0x3F) | 0x80);
						}
						else if (ch.asInt < 0x110000) {
							builder << char((ch.asInt>>18) | 0xF0);
							builder << char(((ch.asInt>>12) & 0x3F) | 0x80);
							builder << char(((ch.asInt>>6) & 0x3F) | 0x80);
							builder << char((ch.asInt & 0x3F) | 0x80);
						}

						index += 3;
						break;
					}
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

		JSONToken::Holder* JSONToken::Holder::Create()
		{
			typedef memory::PoolAllocator<sizeof(JSONToken::Holder)> holderAlloc;

			return (JSONToken::Holder*)holderAlloc::alloc();

		}

		void JSONToken::Holder::Free(JSONToken::Holder* holder)
		{
			typedef memory::PoolAllocator<sizeof(JSONToken::Holder)> holderAlloc;

			holder->~Holder();
			holderAlloc::free(holder);
		}

		JSONToken::Holder::Holder(JSONType forType)
			: m_data(nullptr, 0)
			, m_type(forType)
			, m_objectChildren(nullptr)
			, m_arrayChildren(sprawl::collections::Capacity(0))
			, refCount(1)
		{
			typedef memory::PoolAllocator<sizeof(JSONToken::TokenMap)> mapAlloc;

			if(forType == JSONType::Array)
			{
				//Give the array a little starting space so we have fewer reallocs of it.
				m_arrayChildren.Reserve(16);
			}
			else if(forType == JSONType::Object)
			{
				m_objectChildren = (TokenMap*)mapAlloc::alloc();
				::new(m_objectChildren) TokenMap(64);
			}
		}

		JSONToken::Holder::~Holder()
		{
			typedef memory::PoolAllocator<sizeof(JSONToken::TokenMap)> mapAlloc;

			if(m_objectChildren)
			{
				m_objectChildren->~TokenMap();
				mapAlloc::free(m_objectChildren);
			}
		}


	}
}
