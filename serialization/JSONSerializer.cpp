#include "JSONSerializer.hpp"

namespace sprawl
{
	namespace serialization
	{
		JSONToken JSONToken::none(JSONToken::JSONType::None);

		const JSONToken&JSONToken::operator[](const JSONToken::StringData& key) const
		{
			if( m_type != JSONType::Object )
			{
				return none;
			}

			auto it = m_objectChildren.find( key );
			if( it == m_objectChildren.end() )
			{
				return none;
			}

			return it->second;
		}

		const JSONToken&JSONToken::operator[](size_t index) const
		{
			if( m_type != JSONType::Array || index >= m_arrayChildren.size() )
			{
				return none;
			}

			return m_arrayChildren[index];
		}

		JSONToken&JSONToken::operator[](const JSONToken::StringData& key)
		{
			if( m_type != JSONType::Object )
			{
				return none;
			}

			auto it = m_objectChildren.find( key );
			if( it == m_objectChildren.end() )
			{
				return none;
			}

			return it->second;
		}

		JSONToken&JSONToken::operator[](size_t index)
		{
			if( m_type != JSONType::Array || index >= m_arrayChildren.size() )
			{
				return none;
			}

			return m_arrayChildren[index];
		}

		size_t JSONToken::Size()
		{
			switch( m_type )
			{
			case JSONType::Object:
			{
				return m_objectChildren.size();
			}
			case JSONType::Array:
			{
				return m_arrayChildren.size();
			}
			case JSONType::None:
			case JSONType::Unknown:
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

		std::string JSONToken::ToJSONString(bool pretty, int tabDepth)
		{
			std::string outString;
			if( m_type == JSONType::Object )
			{
				outString += "{";
				if( pretty )
				{
					outString += "\n";
				}
				else
				{
					outString += " ";
				}
				bool first = true;
				for( auto& kvp : m_objectChildren )
				{
					if( !first )
					{
						outString += ",";
						if( pretty )
						{
							outString += "\n";
						}
						else
						{
							outString += " ";
						}
					}

					if( pretty )
					{
						for( int i = 0; i < tabDepth; ++i )
						{
							outString += "\t";
						}
					}
					outString += "\"" + kvp.first.ToString() + "\"";
					outString += " : ";
					outString += kvp.second.ToJSONString( pretty, tabDepth + 1 );
					first = false;
				}
				if( pretty )
				{
					outString += "\n";

					for( int i = 0; i < tabDepth - 1; ++i )
					{
						outString += "\t";
					}
				}
				else
				{
					outString += " ";
				}

				outString += "}";
			}
			else if( m_type == JSONType::Array )
			{
				outString += "[";
				if( pretty )
				{
					outString += "\n";
				}
				else
				{
					outString += " ";
				}
				bool first = true;
				for( auto& it : m_arrayChildren )
				{
					if( !first )
					{
						outString += ",";
						if( pretty )
						{
							outString += "\n";
						}
						else
						{
							outString += " ";
						}
					}

					if( pretty )
					{
						for( int i = 0; i < tabDepth; ++i )
						{
							outString += "\t";
						}
					}
					outString += it.ToJSONString( pretty, tabDepth + 1 );
					first = false;
				}
				if( pretty )
				{
					outString += "\n";

					for( int i = 0; i < tabDepth - 1; ++i )
					{
						outString += "\t";
					}
				}
				else
				{
					outString += " ";
				}

				outString += "]";
			}
			else
			{
				if( m_type == JSONType::String )
				{
					outString += "\"";
				}
				outString += m_data.ToString();
				if( m_type == JSONType::String )
				{
					outString += "\"";
				}
			}
			return outString;
		}

		void JSONToken::PushBack(JSONToken& token)
		{
			if( m_type != JSONType::Array )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data());
			}
			m_arrayChildren.push_back( token );
		}

		void JSONToken::Insert(JSONToken::StringData name, JSONToken& token)
		{
			if( m_type != JSONType::Object )
			{
				SPRAWL_THROW_EXCEPTION(ex_invalid_data());
			}
			token.m_key = name;
			m_objectChildren.insert( std::make_pair( name, token ) );
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, JSONToken::StringData data)
			: m_data( data )
			, m_key( nullptr, 0 )
			, m_type( statedType )
			, m_objectChildren( )
			, m_arrayChildren( )
		{
			//
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, bool data)
			: m_data( nullptr, 0 )
			, m_key( nullptr, 0 )
			, m_type( statedType )
			, m_objectChildren( )
			, m_arrayChildren( )
		{
			if(data)
			{
				m_data = StringData( "true", 4 );
			}
			else
			{
				m_data = StringData( "false", 5 );
			}
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType)
			: m_data( nullptr, 0 )
			, m_key( nullptr, 0 )
			, m_type( statedType )
			, m_objectChildren( )
			, m_arrayChildren( )
		{
			//
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, int data)
			: m_data( nullptr, 0 )
			, m_key( nullptr, 0 )
			, m_type( statedType )
			, m_objectChildren( )
			, m_arrayChildren( )
		{
			char buf[128];
#ifdef _WIN32
			_snprintf( buf, 128, "%d", data );
#else
			snprintf( buf, 128, "%d", data );
#endif
			m_data = StringData( buf, strlen( buf ) );
			m_data.CommitStorage();
		}

		JSONToken::JSONToken(JSONToken::JSONType statedType, double data)
			: m_data( nullptr, 0 )
			, m_key( nullptr, 0 )
			, m_type( statedType )
			, m_objectChildren( )
			, m_arrayChildren( )
		{
			char buf[128];
#ifdef _WIN32
			_snprintf( buf, 128, "%f", data );
#else
			snprintf( buf, 128, "%f", data );
#endif
			m_data = StringData( buf, strlen( buf ) );
			m_data.CommitStorage();
		}

		JSONToken::JSONToken(JSONToken::StringData myKey, const char* const data, int& outPosition, JSONToken::JSONType expectedType)
			: m_data( nullptr, 0 )
			, m_key( myKey )
			, m_type( expectedType )
			, m_objectChildren( )
			, m_arrayChildren( )
		{

			enum class ParseState
			{
				GetKey = 0,
				GetColon = 1,
				GetData = 2,
				GetComma = 3,
			};

			if( m_type == JSONType::Boolean )
			{
				char c = data[outPosition];
				while( c == ' ' || c == '\n' || c == '\r' || c == '\t' )
				{
					++outPosition;
					c = data[outPosition];
				}

				if( data[outPosition] == 't' &&
						data[outPosition + 1] == 'r' &&
						data[outPosition + 2] == 'u' &&
						data[outPosition + 3] == 'e'
						)
				{
					m_data = StringData( &data[outPosition], 4 );
					outPosition += 3;
				}
				else if(
						data[outPosition] == 'f' &&
						data[outPosition + 1] == 'a' &&
						data[outPosition + 2] == 'l' &&
						data[outPosition + 3] == 's' &&
						data[outPosition + 4] == 'e'
						)
				{
					m_data = StringData( &data[outPosition], 5 );
					outPosition += 4;
				}
#ifdef SPRAWL_STRICT_JSON
				else
				{
					SPRAWL_THROW_EXCEPTION(ex_invalid_data());
				}
#endif
				return;
			}

			if( m_type != JSONType::Integer )
			{
				++outPosition;
			}

			int startPoint = outPosition;

			char const* keyStart = nullptr;
			char const* keyEnd = nullptr;
			StringData key( nullptr, 0 );
			bool done = false;
			bool isEscaped = false;
			bool nextIsEscaped = false;
			bool inQuotes = ( m_type == JSONType::String );

			ParseState state = ( m_type == JSONType::Object ) ? ParseState::GetKey : ParseState::GetData;

			if( m_type == JSONType::Integer && data[startPoint] == '-' )
			{
				++outPosition;
			}

			while(!done)
			{
				isEscaped = nextIsEscaped;
				nextIsEscaped = false;
				char c = data[outPosition];

#ifdef SPRAWL_STRICT_JSON
				if( c != ' ' && c != '\n' && c != '\r' && c != '\t' )
				{
					if( ( state == ParseState::GetComma && c != ',' && c != '}' ) || ( state == ParseState::GetColon && c != ':' ) )
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}

					if(
							( m_type == JSONType::Integer || m_type == JSONType::Double ) &&
							c != '0' &&
							c != '1' &&
							c != '2' &&
							c != '3' &&
							c != '4' &&
							c != '5' &&
							c != '6' &&
							c != '7' &&
							c != '8' &&
							c != '9' &&
							c != ',' &&
							c != '.'
							)
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
				}
#endif
				switch( c )
				{
				case '{':
				{
					state = ParseState::GetComma;
					if( m_type == JSONType::Array )
					{
						m_arrayChildren.push_back( JSONToken( key, data, outPosition, JSONType::Object ) );
					}
					else if( m_type == JSONType::Object )
					{
						m_objectChildren.insert( std::make_pair( key, JSONToken( key, data, outPosition, JSONType::Object ) ) );
					}
#ifdef SPRAWL_STRICT_JSON
					else if( m_type != JSONType::String )
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
					break;
				}
				case '[':
				{
					state = ParseState::GetComma;
					if( m_type == JSONType::Array )
					{
						m_arrayChildren.push_back( JSONToken( key, data, outPosition, JSONType::Array ) );
					}
					else if( m_type == JSONType::Object )
					{
						m_objectChildren.insert( std::make_pair( key, JSONToken( key, data, outPosition, JSONType::Array ) ) );
					}
#ifdef SPRAWL_STRICT_JSON
					else if( m_type != JSONType::String )
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
					break;
				}
				case ']':
				{
					if( m_type == JSONType::Array )
					{
						done = true;
					}
#ifdef SPRAWL_STRICT_JSON
					else if( m_type != JSONType::String )
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
					break;
				}
				case '}':
				{
					if( m_type == JSONType::Object )
					{
						done = true;
					}
#ifdef SPRAWL_STRICT_JSON
					else if( m_type != JSONType::String )
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
					break;
				}
				case ':':
				{
					if( m_type == JSONType::Object )
					{
						state = ParseState::GetData;
					}
#ifdef SPRAWL_STRICT_JSON
					else if( m_type != JSONType::String )
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
					break;
				}
				case '\\':
				{
					if( inQuotes )
					{
						nextIsEscaped = true;
					}
#ifdef SPRAWL_STRICT_JSON
					else
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
					break;
				}
				case ',':
				{
					if( m_type == JSONType::Object )
					{
						state = ParseState::GetKey;
						keyStart = nullptr;
						keyEnd = nullptr;
					}
					else if( m_type == JSONType::Array )
					{
						state = ParseState::GetData;
					}
					else if( m_type != JSONType::String )
					{
						done = true;
					}
					break;
				}
				case '\"':
				{
					if( isEscaped )
					{
						break;
					}
					if( state == ParseState::GetKey )
					{
						if( keyStart == nullptr )
						{
							inQuotes = true;
							keyStart = &data[outPosition + 1];
						}
						else if( keyEnd == nullptr )
						{
							inQuotes = false;
							keyEnd = &data[outPosition];
							key = StringData( keyStart, keyEnd - keyStart );
						}
#ifdef SPRAWL_STRICT_JSON
						else
						{
							SPRAWL_THROW_EXCEPTION(ex_invalid_data());
						}
#endif
					}
					else
					{
						if( m_type == JSONType::Array )
						{
							m_arrayChildren.push_back( JSONToken( key, data, outPosition, JSONType::String ) );
						}
						else if( m_type == JSONType::Object )
						{
							m_objectChildren.insert( std::make_pair( key, JSONToken( key, data, outPosition, JSONType::String ) ) );
						}
						else if( m_type == JSONType::String )
						{
							done = true;
						}
#ifdef SPRAWL_STRICT_JSON
						else
						{
							SPRAWL_THROW_EXCEPTION(ex_invalid_data());
						}
#endif
					}
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
					if( m_type == JSONType::Array )
					{
						m_arrayChildren.push_back( JSONToken( key, data, outPosition, JSONType::Integer ) );
					}
					else if( m_type == JSONType::Object )
					{
						m_objectChildren.insert( std::make_pair( key, JSONToken( key, data, outPosition, JSONType::Integer ) ) );
					}
					break;
				}
				case 't':
				case 'f':
				{
					if( state == ParseState::GetData )
					{
						if( m_type == JSONType::Array )
						{
							m_arrayChildren.push_back( JSONToken( key, data, outPosition, JSONType::Boolean ) );
						}
						else if( m_type == JSONType::Object )
						{
							m_objectChildren.insert( std::make_pair( key, JSONToken( key, data, outPosition, JSONType::Boolean ) ) );
						}
					}
					break;
				}
				case '.':
				{
					if( m_type == JSONType::Integer )
					{
						m_type = JSONType::Double;
					}
#ifdef SPRAWL_STRICT_JSON
					else if( !inQuotes )
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
					break;
				}
				case ' ':
				case '\n':
				case '\r':
				case '\t':
				{
					break;
				}
				default:
				{
					if( m_type == JSONType::String || ( m_type == JSONType::Object && state == ParseState::GetKey ) )
					{
						break;
					}
#ifdef SPRAWL_STRICT_JSON
					else
					{
						SPRAWL_THROW_EXCEPTION(ex_invalid_data());
					}
#endif
				}
				}
				if(!done)
					++outPosition;
			}
			m_data = StringData( &data[startPoint], outPosition - startPoint );

			if( m_type == JSONType::Integer || m_type == JSONType::Double )
			{
				--outPosition;
			}
		}

		int JSONToken::StringData::ToInt() const
		{
			int result = 0;
			size_t index = 0;
			bool negative = false;
			if( *m_data == '-' )
			{
				negative = true;
				++index;
			}
			for( ; index < m_length; ++index )
			{
				result *= 10;
				result += ( (int)( m_data[index] ) - 48 );
			}

			if( negative )
			{
				result *= -1;
			}

			return result;
		}

		double JSONToken::StringData::ToDouble() const
		{
			double result = 0;
			size_t index = 0;
			bool negative = false;
			double fractionSize = 1.0;
			bool inFraction = false;

			if( *m_data == '-' )
			{
				negative = true;
				++index;
			}
			for( ; index < m_length; ++index )
			{
				char c = m_data[index];
				if( c == '.' )
				{
					inFraction = true;
					continue;
				}
				result *= 10;
				result += ( (int)( m_data[index] ) - 48 );
				if( inFraction )
				{
					fractionSize *= 10.0;
				}
			}

			if( negative )
			{
				result *= -1;
			}

			result /= fractionSize;

			return result;
		}

		bool JSONToken::StringData::ToBool() const
		{
			if(
					m_length == 4 &&
					m_data[0] == 't' &&
					m_data[1] == 'r' &&
					m_data[2] == 'u' &&
					m_data[3] == 'e'
					)
			{
				return true;
			}
			return false;
		}


	}
}
