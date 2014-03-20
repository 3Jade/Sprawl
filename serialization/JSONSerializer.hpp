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
#include <deque>
#include <sstream>
#include <iostream>

namespace sprawl
{
	namespace serialization
	{
		//TODO: Method of initializing StringData that allocates space for string to deal with scoping?
		//TODO: Get rid of dynamic allocation somehow
		//TODO: Use custom hash map that maintains order of insertion when iterating (such as multiaccess)
		class JSONToken
		{
		public:
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
					m_commitData = new char[m_length];
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

				StringData( const StringData& other )
					: m_data( other.m_data )
					, m_length( other.m_length )
					, m_commitData( other.m_commitData )
				{
					if( m_commitData )
					{
						CommitStorage(); // Get our own copy of it, there's no refcounting
					}
				}

				StringData& operator=( const StringData& other )
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

				size_t Length() const
				{
					return m_length;
				}

				char const* const Data() const
				{
					return m_data;
				}

				char operator[](size_t index) const
				{
					return m_data[index];
				}

				std::string ToString() const
				{
					return std::string(m_data, m_length);
				}

				int ToInt() const
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

				double ToDouble() const
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

				bool ToBool() const
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

			private:
				char const* m_data;
				size_t m_length;
				char* m_commitData;
			};

			struct StringDataHash
			{
				std::size_t operator()( StringData const& str ) const
				{
					//TODO: Better hashing.
					std::size_t hash = 0;

					char const* const data = str.Data();
					size_t length = str.Length();
					for( size_t i = 0; i < length; ++i )
					{
						/* hash = hash * 33 ^ c */
						hash = ((hash << 5) + hash) ^ (size_t)(data[i]);
					}

					return hash;
				}
			};

			struct StringDataCmp
			{
				bool operator()(const StringData& x, const StringData& y) const
				{
					size_t xlen = x.Length();
					size_t ylen = y.Length();

					if( xlen != ylen )
					{
						return xlen < ylen;
					}
					return memcmp( x.Data(), y.Data(), xlen ) < 0;
				}
			};
			enum class JSONType
			{
				Unknown = 0,
				Integer = 1,
				String = 2,
				Boolean = 3,
				Array = 4,
				Object = 5,
				Double = 6,
			};

			StringData const& GetKey() const
			{
				return m_key;
			}

			int Int() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Integer )
				{
					throw ex_serializer_overflow();
				}
#endif

				return m_data.ToInt();
			}

			std::string String() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::String )
				{
					throw ex_serializer_overflow();
				}
#endif

				return m_data.ToString();
			}

			bool Bool() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Boolean )
				{
					throw ex_serializer_overflow();
				}
#endif

				return m_data.ToBool();
			}

			double Double() const
			{
#ifdef SPRAWL_STRICT_JSON
				if( m_type != JSONType::Double )
				{
					throw ex_serializer_overflow();
				}
#endif

				return m_data.ToDouble();
			}

			JSONToken const& operator[]( StringData const& key ) const
			{
				if( m_type != JSONType::Object )
				{
					throw ex_serializer_overflow();
				}

				auto it = m_objectChildren.find( key );
				if( it == m_objectChildren.end() )
				{
					throw ex_serializer_overflow();
				}

				return it->second;
			}

			JSONToken const& operator[]( size_t index ) const
			{
				if( m_type != JSONType::Array || index >= m_arrayChildren.size() )
				{
					throw ex_serializer_overflow();
				}

				return m_arrayChildren[index];
			}

			JSONToken& operator[]( StringData const& key )
			{
				if( m_type != JSONType::Object )
				{
					throw ex_serializer_overflow();
				}

				auto it = m_objectChildren.find( key );
				if( it == m_objectChildren.end() )
				{
					throw ex_serializer_overflow();
				}

				return it->second;
			}

			JSONToken& operator[]( size_t index )
			{
				if( m_type != JSONType::Array || index >= m_arrayChildren.size() )
				{
					throw ex_serializer_overflow();
				}

				return m_arrayChildren[index];
			}

			JSONType Type()
			{
				return m_type;
			}

			size_t Size()
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
					default:
					{
						return 1;
					}
				}
			}

			std::string ToJSONString( bool pretty = false, int tabDepth = 1 )
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

			void PushBack( JSONToken& token )
			{
				if( m_type != JSONType::Array )
				{
					throw ex_invalid_data();
				}
				m_arrayChildren.push_back( token );
			}

			void Insert( StringData name, JSONToken& token )
			{
				if( m_type != JSONType::Object )
				{
					throw ex_invalid_data();
				}
				token.m_key = name;
				m_objectChildren.insert( std::make_pair( name, token ) );
			}

			static JSONToken array()
			{
				return JSONToken( JSONType::Array, StringData( nullptr, 0 ) );
			}

			static JSONToken object()
			{
				return JSONToken( JSONType::Object, StringData( nullptr, 0 ) );
			}

			static JSONToken string( char const* const data )
			{
				return JSONToken( JSONType::String, StringData( data ) );
			}

			static JSONToken number( int data )
			{
				return JSONToken( JSONType::Integer, data );
			}

			static JSONToken number( double data )
			{
				return JSONToken( JSONType::Double, data );
			}

			static JSONToken boolean( bool data )
			{
				return JSONToken( JSONType::Boolean, data );
			}

		protected:
			JSONToken( JSONType statedType, StringData data )
				: m_data( data )
				, m_key( nullptr, 0 )
				, m_type( statedType )
				, m_objectChildren( )
				, m_arrayChildren( )
			{
				//
			}

			JSONToken( JSONType statedType, bool data )
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


			JSONToken( JSONType statedType, int data )
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


			JSONToken( JSONType statedType, double data )
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

		public:
			JSONToken( StringData myKey, char const* const data, int& outPosition, JSONType expectedType )
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
						throw ex_invalid_data();
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
							throw ex_invalid_data();
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
							throw ex_invalid_data();
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
								throw ex_invalid_data();
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
								throw ex_invalid_data();
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
								throw ex_invalid_data();
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
								throw ex_invalid_data();
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
								throw ex_invalid_data();
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
								throw ex_invalid_data();
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
									throw ex_invalid_data();
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
									throw ex_invalid_data();
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
								throw ex_invalid_data();
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
								throw ex_invalid_data();
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

		private:
			StringData m_data;
			StringData m_key;
			JSONType m_type;
			std::map<StringData, JSONToken, StringDataCmp> m_objectChildren;
			std::vector<JSONToken> m_arrayChildren;
		};

		class JSONSerializerBase : virtual public SerializerBase
		{
		public:
			typedef class JSONSerializer serializer_type;
			typedef class JSONDeserializer deserializer_type;
			virtual uint32_t GetVersion() override { return m_version; }
			virtual bool IsValid() override { return m_bIsValid; }
			virtual void SetVersion(uint32_t i) override
			{
				m_version = i;
				if(IsSaving() && m_bWithMetadata)
				{
					//Update version metadata
					std::stringstream strval;
					strval << i;
					m_serialVect.front().second = strval.str();
				}
			}
			virtual void Reset() override { }
			using SerializerBase::Data;
			virtual const char* Data() override { return Str().c_str(); }
			virtual std::string Str() override
			{
				std::stringstream datastream;
				datastream << "{";
				if( m_prettyPrint )
				{
					datastream << "\n\t";
				}
				else
				{
					datastream << " ";
				}
				for(size_t i =0; i < m_serialVect.size(); i++)
				{
					auto& kvp = m_serialVect[i];
					datastream << "\"" << kvp.first << "\" : " << kvp.second;
					if(i != m_serialVect.size() - 1)
					{
						datastream << ",";
						if( m_prettyPrint )
						{
							datastream << "\n\t";
						}
						else
						{
							datastream << " ";
						}
					}
					else if( m_prettyPrint )
					{
						datastream << "\n";
					}
				}
				if( !m_prettyPrint )
				{
					datastream << " ";
				}
				datastream << "}";
				return datastream.str();
			}
			virtual size_t Size() override
			{
				return Str().length();
			}
			bool More(){ return !m_serialVect.empty() || !m_thisArray.empty() || !m_thisObject.empty(); }
		protected:
			char const* const GetSeparator()
			{
				if(m_prettyPrint)
				{
					return "\n\t";
				}
				else
				{
					return " ";
				}
			}

			template<typename T>
			friend class ReplicableBase;
			using SerializerBase::serialize;
			template<typename T>
			void serialize_impl(T* var, const uint32_t bytes, const std::string& name, bool)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				uint32_t size = bytes/sizeof(T);
				if(bytes > sizeof(T))
				{
					StartArray(name, size, true);
					bIsArray = true;
				}
				if(IsLoading())
				{
					std::string strval;
					if(!m_stateTracker.empty())
					{
						if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
						{
							if(bIsArray)
							{
								int i = 0;
								while(!m_thisArray.back().second.empty())
								{
									strval = m_thisArray.back().second.front();
									std::stringstream converter(strval);
									T newvar;
									converter >> newvar;
									var[i] = newvar;
									i++;
									m_thisArray.back().second.erase(m_thisArray.back().second.begin());
								}
							}
							else
							{
								strval = m_thisArray.back().second.front();
								m_thisArray.back().second.erase(m_thisArray.back().second.begin());
								std::stringstream converter(strval);
								converter >> *var;
							}
						}
						else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
						{
							bool bFound = false;
							for(auto it = m_thisObject.back().second.begin(); it != m_thisObject.back().second.end(); it++)
							{
								if(it->first == name)
								{
									strval = it->second;
									m_thisObject.back().second.erase(it);
									bFound = true;
									break;
								}
							}
							if(!bFound)
							{
								throw ex_serializer_overflow();
							}
							std::stringstream converter(strval);
							converter >> *var;
						}
					}
					else
					{
						bool bFound = false;
						for(auto it = m_serialVect.begin(); it != m_serialVect.end(); it++)
						{
							if(it->first == name)
							{
								strval = it->second;
								m_serialVect.erase(it);
								bFound = true;
								break;
							}
						}
						if(!bFound)
						{
							throw ex_serializer_overflow();
						}
						std::stringstream converter(strval);
						converter >> *var;
					}
				}
				else
				{
					std::stringstream converter;
					if(bIsArray)
					{
						for(uint32_t i = 0; i < size; i++)
						{
							converter.str("");
							converter << var[i];
							m_thisArray.back().second.push_back(converter.str());
						}
					}
					else
					{
						converter << *var;
						if(!m_stateTracker.empty())
						{
							if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
							{
								m_thisArray.back().second.push_back(converter.str());
							}
							else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
							{
								m_thisObject.back().second.push_back(std::make_pair(name, converter.str()));
							}
						}
						else
						{
							m_serialVect.push_back(std::make_pair(name, converter.str()));
						}
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			void serialize_impl(bool* var, const uint32_t, const std::string& name, bool)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsLoading())
				{
					std::string strval;
					if(!m_stateTracker.empty())
					{
						if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
						{
							strval = m_thisArray.back().second.front();
							m_thisArray.back().second.erase(m_thisArray.back().second.begin());
							if(strval == "true")
							{
								*var = true;
							}
							else
							{
								*var = false;
							}
						}
						else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
						{
							bool bFound = false;
							for(auto it = m_thisObject.back().second.begin(); it != m_thisObject.back().second.end(); it++)
							{
								if(it->first == name)
								{
									strval = it->second;
									m_thisObject.back().second.erase(it);
									bFound = true;
									break;
								}
							}
							if(!bFound)
							{
								throw ex_serializer_overflow();
							}

							if(strval == "true")
							{
								*var = true;
							}
							else
							{
								*var = false;
							}
						}
					}
					else
					{
						bool bFound = false;
						for(auto it = m_serialVect.begin(); it != m_serialVect.end(); it++)
						{
							if(it->first == name)
							{
								strval = it->second;
								m_serialVect.erase(it);
								bFound = true;
								break;
							}
						}
						if(!bFound)
						{
							throw ex_serializer_overflow();
						}

						if(strval == "true")
						{
							*var = true;
						}
						else
						{
							*var = false;
						}
					}
				}
				else
				{
					std::string strval( *var ? "true" : "false" );
					if(!m_stateTracker.empty())
					{
						if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
						{
							m_thisArray.back().second.push_back(strval);
						}
						else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
						{
							m_thisObject.back().second.push_back(std::make_pair(name, strval));
						}
					}
					else
					{
						m_serialVect.push_back(std::make_pair(name, strval));
					}
				}
			}

			void serialize_impl(char* var, const uint32_t, const std::string& name, bool)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsLoading())
				{
					std::string strval;
					if(!m_stateTracker.empty())
					{
						if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
						{
							strval = m_thisArray.back().second.front();
							m_thisArray.back().second.erase(m_thisArray.back().second.begin());
							strcpy(var, strval.substr(1, strval.length()-2).c_str());
						}
						else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
						{
							bool bFound = false;
							for(auto it = m_thisObject.back().second.begin(); it != m_thisObject.back().second.end(); it++)
							{
								if(it->first == name)
								{
									strval = it->second;
									m_thisObject.back().second.erase(it);
									bFound = true;
									break;
								}
							}
							if(!bFound)
							{
								throw ex_serializer_overflow();
							}
							strcpy(var, strval.substr(1, strval.length()-2).c_str());
						}
					}
					else
					{
						bool bFound = false;
						for(auto it = m_serialVect.begin(); it != m_serialVect.end(); it++)
						{
							if(it->first == name)
							{
								strval = it->second;
								m_serialVect.erase(it);
								bFound = true;
								break;
							}
						}
						if(!bFound)
						{
							throw ex_serializer_overflow();
						}
						strcpy(var, strval.substr(1, strval.length()-2).c_str());
					}
				}
				else
				{
					std::stringstream converter;
					converter << "\"" << var << "\"";
					if(!m_stateTracker.empty())
					{
						if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
						{
							m_thisArray.back().second.push_back(converter.str());
						}
						else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
						{
							m_thisObject.back().second.push_back(std::make_pair(name, converter.str()));
						}
					}
					else
					{
						m_serialVect.push_back(std::make_pair(name, converter.str()));
					}
				}
			}

			void serialize_impl(std::string* var, const uint32_t, const std::string& name, bool)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsLoading())
				{
					std::string strval;
					if(!m_stateTracker.empty())
					{
						if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
						{
							strval = m_thisArray.back().second.front();
							m_thisArray.back().second.erase(m_thisArray.back().second.begin());
							*var = strval.substr(1, strval.length()-2);
						}
						else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
						{
							bool bFound = false;
							for(auto it = m_thisObject.back().second.begin(); it != m_thisObject.back().second.end(); it++)
							{
								if(it->first == name)
								{
									strval = it->second;
									m_thisObject.back().second.erase(it);
									bFound = true;
									break;
								}
							}
							if(!bFound)
							{
								throw ex_serializer_overflow();
							}
							*var = strval.substr(1, strval.length()-2);
						}
					}
					else
					{
						bool bFound = false;
						for(auto it = m_serialVect.begin(); it != m_serialVect.end(); it++)
						{
							if(it->first == name)
							{
								strval = it->second;
								m_serialVect.erase(it);
								bFound = true;
								break;
							}
						}
						if(!bFound)
						{
							throw ex_serializer_overflow();
						}
						*var = strval.substr(1, strval.length()-2);
					}
				}
				else
				{
					std::stringstream converter;
					converter << "\"" << *var << "\"";
					if(!m_stateTracker.empty())
					{
						if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
						{
							m_thisArray.back().second.push_back(converter.str());
						}
						else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
						{
							m_thisObject.back().second.push_back(std::make_pair(name, converter.str()));
						}
					}
					else
					{
						m_serialVect.push_back(std::make_pair(name, converter.str()));
					}
				}
			}

		public:
			virtual void serialize(int* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long int* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long long int* var, const uint32_t bytes, const std::string& name, bool PersistToDB)  override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(short int* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(char* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(float* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(double* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long double* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(bool* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned int* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long int* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long long int* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned short int* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned char* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}
			virtual void serialize(std::string* var, const uint32_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual uint32_t StartObject(const std::string& str, bool = true) override
			{
				++m_tabDepth;
				State LastState = m_stateTracker.empty() ? State::None : m_stateTracker.back();
				m_stateTracker.push_back(State::Object);
				if(IsSaving())
				{
					m_thisObject.push_back(std::make_pair(str, std::deque<std::pair<std::string, std::string>>()));
					return 0; //doesn't matter.
				}
				else
				{
					bool bFound = false;
					std::string strval;
					if(!m_thisObject.empty() && LastState == State::Object)
					{
						for(auto it = m_thisObject.back().second.begin(); it != m_thisObject.back().second.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								m_thisObject.back().second.erase(it);
								bFound = true;
								break;
							}
						}
					}
					else if(!m_thisArray.empty() && LastState == State::Array)
					{
						for(auto it = m_thisArray.begin(); it != m_thisArray.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second.front();
								it->second.erase(it->second.begin());
								bFound = true;
								break;

							}
						}
					}
					else
					{
						for(auto it = m_serialVect.begin(); it != m_serialVect.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								m_serialVect.erase(it);
								bFound = true;
								break;
							}
						}
					}
					std::deque<std::pair<std::string, std::string>> jsondata;
					ParseJSON(strval, jsondata);
					m_thisObject.push_back(std::make_pair(str, jsondata));
					return (uint32_t)jsondata.size();
				}
			}
			virtual void EndObject() override
			{
				m_stateTracker.pop_back();
				if(IsSaving())
				{
					auto& kvp = m_thisObject.back();
					std::string key = kvp.first;
					std::string objstr = "{";
					if(m_prettyPrint)
					{
						objstr += "\n";
					}
					else
					{
						objstr += " ";
					}
					for(size_t i=0; i<kvp.second.size(); i++)
					{
						if( m_prettyPrint )
						{
							for( int i = 0; i < m_tabDepth; ++i )
							{
								objstr += "\t";
							}
						}
						objstr += "\"" +  kvp.second[i].first + "\" : " + kvp.second[i].second;
						if(i != kvp.second.size() - 1)
						{
							objstr += ",";
							if( m_prettyPrint )
							{
								objstr += "\n";
							}
							else
							{
								objstr += " ";
							}
						}
						else if( m_prettyPrint )
						{
							objstr += "\n";
						}
					}
					--m_tabDepth;
					if( m_prettyPrint )
					{
						for( int i = 0; i < m_tabDepth; ++i )
						{
							objstr += "\t";
						}
					}
					else
					{
						objstr += " ";
					}
					objstr += "}";
					m_thisObject.pop_back();
					if(!m_thisArray.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array)
					{
						m_thisArray.back().second.push_back(objstr);
					}
					else if(!m_thisObject.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
					{
						m_thisObject.back().second.push_back(std::make_pair(key, objstr));
					}
					else
					{
						m_serialVect.push_back(std::make_pair(key, objstr));
					}
				}
				else
				{
					m_thisObject.pop_back();
				}
			}

			virtual void StartArray(const std::string& str, uint32_t& size, bool = true) override
			{
				++m_tabDepth;
				State LastState = m_stateTracker.empty() ? State::None : m_stateTracker.back();
				m_stateTracker.push_back(State::Array);
				if(IsSaving())
				{
					m_thisArray.push_back(std::make_pair(str, std::deque<std::string>()));
				}
				else
				{
					bool bFound = false;
					std::string strval;
					size = 0;
					if(!m_thisArray.empty() && LastState == State::Array)
					{
						strval = m_thisArray.back().second.front();
						m_thisArray.back().second.erase(m_thisArray.back().second.begin());
					}
					else if(!m_thisObject.empty() && LastState == State::Object)
					{
						for(auto it = m_thisObject.back().second.begin(); it != m_thisObject.back().second.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								m_thisObject.back().second.erase(it);
								bFound = true;
								break;
							}
						}
					}
					else
					{
						for(auto it = m_serialVect.begin(); it != m_serialVect.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								m_serialVect.erase(it);
								bFound = true;
								break;
							}
						}
					}
					std::deque<std::pair<std::string, std::string>> jsondata;
					ParseJSON(strval, jsondata);
					std::deque<std::string> jsondata_nokeys;
					//Because arrays aren't in kvp format, the keys here will be in first, and second will be empty
					for(auto& kvp : jsondata)
					{
						size++;
						jsondata_nokeys.push_back(kvp.first);
					}
					m_thisArray.push_back(std::make_pair(str, jsondata_nokeys));
				}
			}

			virtual void EndArray() override
			{
				m_stateTracker.pop_back();
				if(IsSaving())
				{
					auto& kvp = m_thisArray.back();
					std::string key = kvp.first;
					std::string arrstr = "[";
					if( m_prettyPrint )
					{
						arrstr += "\n";
					}
					else
					{
						arrstr += " ";
					}
					for(size_t i=0; i<kvp.second.size(); i++)
					{
						if( m_prettyPrint )
						{
							for( int i = 0; i < m_tabDepth; ++i )
							{
								arrstr += "\t";
							}
						}
						arrstr += kvp.second[i];
						if(i != kvp.second.size() - 1)
						{
							arrstr += ",";

							if( m_prettyPrint )
							{
								arrstr += "\n";
							}
							else
							{
								arrstr += " ";
							}
						}
						else if( m_prettyPrint )
						{
							arrstr += "\n";
						}
					}
					--m_tabDepth;
					if( m_prettyPrint )
					{
						for( int i = 0; i < m_tabDepth; ++i )
						{
							arrstr += "\t";
						}
					}
					else
					{
						arrstr += " ";
					}
					arrstr += "]";
					m_thisArray.pop_back();
					if(!m_thisObject.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
					{
						m_thisObject.back().second.push_back(std::make_pair(key, arrstr));
					}
					else if(!m_thisArray.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array)
					{
						if(key != m_thisArray.back().first)
						{
							arrstr = "\"" + key + "\" : " + arrstr;
						}
						m_thisArray.back().second.push_back(arrstr);
					}
					else
					{
						m_serialVect.push_back(std::make_pair(key, arrstr));
					}
				}
				else
				{
					m_thisArray.pop_back();
				}
			}

			std::string GetNextKey()
			{
				if(IsSaving())
				{
					return "";
				}
				if(!m_stateTracker.empty())
				{
					if(!m_thisArray.empty() && m_stateTracker.back() == State::Array)
					{
						return "";
					}
					else if(!m_thisObject.empty() && m_stateTracker.back() == State::Object)
					{
						return m_thisObject.back().second.front().first;
					}
					else
					{
						throw ex_serializer_overflow();
					}
				}
				else
				{
					if(m_serialVect.empty())
					{
						throw ex_serializer_overflow();
					}
					return m_serialVect.front().first;
				}
			}

		protected:
			JSONSerializerBase()
				: SerializerBase()
				, m_version(0)
				, m_bIsValid(true)
				, m_bWithMetadata(true)
				, m_tabDepth(1)
				, m_prettyPrint(false)
			{}
			virtual ~JSONSerializerBase() {}

			static void ParseJSON(const std::string& str, std::deque<std::pair<std::string, std::string>>& ret)
			{
				int TokenLevel = 0;
				std::string key, value;
				bool GotKey = false;
				bool in_quotes = false;
				for(size_t i=0; i < str.length(); i++)
				{
					switch(str[i])
					{
						case '{':
						case '[':
							if(!in_quotes)
							{
								TokenLevel++;
								if(TokenLevel == 1)
								{
									break;
								}
							}
							if(GotKey)
							{
								value += str[i];
							}
							else
							{
								key += str[i];
							}
							break;
						case '}':
						case ']':
							if(!in_quotes)
							{
								TokenLevel--;
								if(TokenLevel == 0)
								{
									break;
								}
							}
							if(GotKey)
							{
								value += str[i];
							}
							else
							{
								key += str[i];
							}
							break;
						case ':':
							if(!in_quotes && TokenLevel == 1)
							{
								GotKey = true;
								break;
							}
						case ',':
							if(!in_quotes && TokenLevel == 1)
							{
								GotKey = false;
								ret.push_back(std::make_pair(key, value));
								key = "";
								value = "";
								break;
							}
						case ' ':
						case '\t':
						case '\n':
						case '\r':
							if(!in_quotes && TokenLevel == 1)
							{
								break;
							}
						default:
							if(GotKey)
							{
								value += str[i];
							}
							else
							{
								key += str[i];
							}
							break;
						case '\"':
							if( TokenLevel == 1 && (i == 0 || str[i-1] != '\\'))
							{
								in_quotes = !in_quotes;
								if(!GotKey)
								{
									break;
								}
							}
							if(GotKey)
							{
								value += str[i];
							}
							else
							{
								key += str[i];
							}
							break;
					}
				}
				ret.push_back(std::make_pair(key, value));
			}

			enum class State { None, Array, Object };
			std::deque<State> m_stateTracker;
			std::deque<std::pair<std::string, std::deque<std::string>>> m_thisArray;
			std::deque<std::pair<std::string, std::deque<std::pair<std::string, std::string>>>> m_thisObject;
			std::deque<std::pair<std::string, std::string>> m_serialVect;

			//Copied and pasted to avoid indirection with virtual inheritance
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bWithMetadata;
			int m_tabDepth;
			bool m_prettyPrint;
		private:
			JSONSerializerBase(const SerializerBase&);
			JSONSerializerBase& operator=(const SerializerBase&);
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
				m_thisArray.clear();
				m_thisObject.clear();
				m_serialVect.clear();
				m_stateTracker.clear();
				serialize(m_version, sizeof(m_version), "__version__", true);
			}
			virtual SerializerBase& operator%(SerializationData<Serializer>&& var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase& operator%(SerializationData<JSONSerializer>&& var) override
			{
				std::string str = var.val.Str();
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
			virtual SerializerBase* GetAnother(const std::string& /*data*/) override { throw std::exception(); }
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
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase& operator%(SerializationData<JSONDeserializer>&& var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}

			virtual void Data(const std::string& str) override
			{
				m_dataStr = str;
				m_serialVect.clear();
				ParseJSON(str, m_serialVect);
				m_bIsValid = true;
				if(m_bWithMetadata)
					serialize(m_version, sizeof(m_version), "__version__", true);
			}

			virtual void Data(const char* data, size_t length) override
			{
				m_dataStr = std::string(data, length);
				m_serialVect.clear();
				ParseJSON(m_dataStr, m_serialVect);
				m_bIsValid = true;
				if(m_bWithMetadata)
					serialize(m_version, sizeof(m_version), "__version__", true);
			}


			JSONDeserializer(const std::string& data) : JSONSerializerBase(), Deserializer()
			{
				Data(data);
			}
			JSONDeserializer(const std::string& data, bool) : JSONSerializerBase(), Deserializer()
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
			virtual SerializerBase* GetAnother(const std::string& data) override { return new JSONDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { throw std::exception(); }
		private:
			std::string m_dataStr;
		};
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif