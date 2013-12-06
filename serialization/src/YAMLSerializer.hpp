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
	#pragma warning( disable: 4250 )
#endif

#include "Serializer.hpp"
#include <deque>
#include <sstream>
#include <iostream>

namespace sprawl
{
	namespace serialization
	{
		class YAMLSerializerBase : virtual public SerializerBase
		{
		public:
			typedef class YAMLSerializer serializer_type;
			typedef class YAMLDeserializer deserializer_type;
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
				datastream << "---\n";
				for(size_t i =0; i < m_serialVect.size(); i++)
				{
					auto& kvp = m_serialVect[i];
					datastream << kvp.first << ":" << kvp.second << "\n";
				}
				datastream << "...\n";
				return datastream.str();
			}
			virtual size_t Size() override
			{
				return Str().length();
			}
			bool More(){ return !m_serialVect.empty() || !m_thisArray.empty() || !m_thisObject.empty(); }
		protected:
			template<typename T>
			friend class ReplicableBase;
			using SerializerBase::serialize;
			template<typename T>
			void serialize_impl(T* var, const size_t bytes, const std::string& name, bool)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = bytes/sizeof(T);
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
						for(size_t i = 0; i < size; i++)
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

			void serialize_impl(char* var, const int, const std::string& name, bool)
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
							strcpy(var, strval.c_str());
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
						strcpy(var, strval.c_str());
					}
				}
				else
				{
					std::stringstream converter;
					converter << var;
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

			void serialize_impl(std::string* var, const int, const std::string& name, bool)
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
							*var = strval;
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
							*var = strval;
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
						*var = strval;
					}
				}
				else
				{
					std::stringstream converter;
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

			virtual void serialize(int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long long int* var, const size_t bytes, const std::string& name, bool PersistToDB)  override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(short int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(char* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(float* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(double* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long double* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(bool* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long long int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned short int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned char* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}
			virtual void serialize(std::string* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual size_t StartObject(const std::string& str, bool = true) override
			{
				State LastState = m_stateTracker.empty() ? State::None : m_stateTracker.back();
				m_stateTracker.push_back(State::Object);
				m_indent += 2;
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
					std::deque<std::pair<std::string, std::string>> YAMLdata;
					ParseYAML(strval, YAMLdata);
					m_thisObject.push_back(std::make_pair(str, YAMLdata));
					return YAMLdata.size();
				}
			}
			virtual void EndObject() override
			{
				m_stateTracker.pop_back();
				if(IsSaving())
				{
					auto& kvp = m_thisObject.back();
					std::string key = kvp.first;
					std::string objstr;
					if(m_stateTracker.empty() || m_stateTracker.back() != State::Array)
					{
						objstr += "\n";
					}
					for(size_t i=0; i<kvp.second.size(); i++)
					{
						if(m_stateTracker.empty() || m_stateTracker.back() != State::Array || i != 0)
						{
							for(int j=0; j<m_indent; j++)
							{
								objstr += " ";
							}
						}
						objstr +=  kvp.second[i].first + ": " + kvp.second[i].second;
						if(i != kvp.second.size() - 1)
						{
							objstr += "\n";
						}
					}
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
				m_indent -= 2;
			}

			virtual void StartArray(const std::string& str, size_t& size, bool = true) override
			{
				State LastState = m_stateTracker.empty() ? State::None : m_stateTracker.back();
				m_stateTracker.push_back(State::Array);
				m_indent += 2;
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
					std::deque<std::pair<std::string, std::string>> YAMLdata;
					ParseYAML(strval, YAMLdata);
					std::deque<std::string> YAMLdata_nokeys;
					for(auto& kvp : YAMLdata)
					{
						size++;
						YAMLdata_nokeys.push_back(kvp.second);
					}
					m_thisArray.push_back(std::make_pair(str, YAMLdata_nokeys));
				}
			}

			virtual void EndArray() override
			{
				m_stateTracker.pop_back();
				if(IsSaving())
				{
					auto& kvp = m_thisArray.back();
					std::string key = kvp.first;
					std::string arrstr;
					if(m_stateTracker.empty() || m_stateTracker.back() != State::Array)
					{
						arrstr += "\n";
					}
					for(size_t i=0; i<kvp.second.size(); i++)
					{
						if( m_stateTracker.empty() || m_stateTracker.back() != State::Array || i != 0 )
						{
							for(int j=0; j<m_indent; j++)
							{
								arrstr += " ";
							}
						}
						arrstr += "- ";
						arrstr += kvp.second[i];
						if( i != kvp.second.size()-1 )
						{
							arrstr += "\n";
						}
					}
					m_thisArray.pop_back();
					if(!m_thisObject.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
					{
						m_thisObject.back().second.push_back(std::make_pair(key, arrstr));
					}
					else if(!m_thisArray.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array)
					{
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
				m_indent -= 2;
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

			YAMLSerializerBase()
				: SerializerBase()
				, m_indent(0)
				, m_version(0)
				, m_bIsValid(true)
				, m_bWithMetadata(true)
			{}
			virtual ~YAMLSerializerBase() {}
		protected:
			static void ParseYAML(const std::string& str, std::deque<std::pair<std::string, std::string>>& ret)
			{
				std::string key, value;
				bool GotKey = false;
				int indent = 0;
				size_t i = 0;
				while((str[i] == '\n' || str[i] == '\r') && i < str.length())
				{
					i++;
				}
				if(str.substr(i, 3) == "---")
				{
					i+=3;
				}
				while((str[i] == '\n' || str[i] == '\r') && i < str.length())
				{
					i++;
				}
				bool bFoundDash = false;
				for( ; i < str.length(); i++)
				{
					if(str[i] == '\n' || str[i] == '\r')
					{
						if(bFoundDash)
						{
							throw std::exception();
						}
						indent = 0;
						continue;
					}
					if(str[i] == '-' && !bFoundDash)
					{
						bFoundDash = true;
					}
					else if(str[i] != ' ' && (str[i] != '-' || bFoundDash))
					{
						break;
					}
					indent++;
				}
				bool bInArray = bFoundDash;
				if(bInArray)
				{
					key = "-";
					value = "";
					GotKey = true;
					indent -= 2;
				}
				bool eol = false;
				for( ; i < str.length(); i++)
				{
					if(str[i] == ':' && !GotKey)
					{
						GotKey = true;
						continue;
					}
					else if(str[i] == '\n' || str[i] == '\r')
					{
						if(!GotKey)
						{
							GotKey = true;
							continue;
						}
						eol = true;
					}
					else
					{
						if(eol == true)
						{
							for(int j=0; j<indent;j++)
							{
								if(str[i] != ' ' && str[i] != '-')
								{
									throw ex_invalid_data();
								}
								i++;
							}


							if(bInArray == true)
							{
								if(str[i] == '-')
								{
									if(GotKey)
									{
										//Remove trailing newline.
										ret.push_back(std::make_pair(key, value.substr(0, value.length()-1)));
									}
									key = "-";
									i++;
									value = "";
									GotKey = true;
									eol = false;
									continue;
								}
								else if(str[i] == ' ')
								{
									//Account for indent.
									//Because of loop, i++ will be called twice, so we only need to do it once.
									i++;
									eol = false;
									continue;
								}
							}
							else if(str[i] != ' ' && str[i] != '-' && GotKey)
							{
								GotKey = false;
								//Remove trailing newline.
								ret.push_back(std::make_pair(key, value.substr(0, value.length()-1)));
								key = "";
								value = "";
							}
							eol = false;
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
				}
				if(key.substr(0,3) != "...")
				{
					ret.push_back(std::make_pair(key, value));
				}
			}

			enum class State { None, Array, Object };
			std::deque<State> m_stateTracker;
			std::deque<std::pair<std::string, std::deque<std::string>>> m_thisArray;
			std::deque<std::pair<std::string, std::deque<std::pair<std::string, std::string>>>> m_thisObject;
			std::deque<std::pair<std::string, std::string>> m_serialVect;
			int m_indent;

			//Copied and pasted to avoid indirection with virtual inheritance
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bWithMetadata;
		private:
			YAMLSerializerBase(const SerializerBase&);
			YAMLSerializerBase& operator=(const SerializerBase&);
		};

		class YAMLSerializer : public YAMLSerializerBase, public Serializer
		{
		public:
			using Serializer::operator%;
			using Serializer::IsLoading;

			using YAMLSerializerBase::serialize;
			using YAMLSerializerBase::IsBinary;
			using YAMLSerializerBase::IsMongoStream;
			using YAMLSerializerBase::IsReplicable;
			using YAMLSerializerBase::IsValid;
			using YAMLSerializerBase::Str;
			using YAMLSerializerBase::Data;
			using YAMLSerializerBase::GetVersion;
			using YAMLSerializerBase::SetVersion;
			using YAMLSerializerBase::Size;

			using YAMLSerializerBase::StartObject;
			using YAMLSerializerBase::EndObject;
			using YAMLSerializerBase::StartArray;
			using YAMLSerializerBase::EndArray;
			using YAMLSerializerBase::StartMap;
			using YAMLSerializerBase::EndMap;
			using YAMLSerializerBase::GetNextKey;
			using YAMLSerializerBase::GetDeletedKeys;

			virtual void Reset() override
			{
				m_thisArray.clear();
				m_thisObject.clear();
				m_serialVect.clear();
				m_stateTracker.clear();
			}
			virtual SerializerBase& operator%(SerializationData<Serializer>&& var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase& operator%(SerializationData<YAMLSerializer>&& var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			YAMLSerializer() : YAMLSerializerBase(), Serializer()
			{
				if(m_bWithMetadata)
					serialize(m_version, sizeof(m_version), "__version__", true);
			}

			YAMLSerializer(bool) : YAMLSerializerBase(), Serializer()
			{
				m_bWithMetadata = false;
			}
			virtual ~YAMLSerializer() {};
		protected:
			virtual SerializerBase* GetAnother(const std::string& /*data*/) override { throw std::exception(); }
			virtual SerializerBase* GetAnother() override { return new YAMLSerializer(false); }
		};

		class YAMLDeserializer : public YAMLSerializerBase, public Deserializer
		{
		public:
			//Reset everything to original state.
			virtual void Reset() override { Data(m_dataStr); }
			using Deserializer::operator%;
			using Deserializer::IsLoading;

			using YAMLSerializerBase::serialize;
			using YAMLSerializerBase::IsBinary;
			using YAMLSerializerBase::IsMongoStream;
			using YAMLSerializerBase::IsReplicable;
			using YAMLSerializerBase::IsValid;
			using YAMLSerializerBase::Str;
			using YAMLSerializerBase::Data;
			using YAMLSerializerBase::GetVersion;
			using YAMLSerializerBase::SetVersion;
			using YAMLSerializerBase::Size;

			using YAMLSerializerBase::StartObject;
			using YAMLSerializerBase::EndObject;
			using YAMLSerializerBase::StartArray;
			using YAMLSerializerBase::EndArray;
			using YAMLSerializerBase::StartMap;
			using YAMLSerializerBase::EndMap;
			using YAMLSerializerBase::GetNextKey;
			using YAMLSerializerBase::GetDeletedKeys;

			virtual SerializerBase& operator%(SerializationData<Deserializer>&& var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase& operator%(SerializationData<YAMLDeserializer>&& var) override
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
				ParseYAML(str, m_serialVect);
				m_bIsValid = true;
				if(m_bWithMetadata)
					serialize(m_version, sizeof(m_version), "__version__", true);
			}
			YAMLDeserializer(const std::string& data) : YAMLSerializerBase(), Deserializer()
			{
				Data(data);
			}
			YAMLDeserializer(const std::string& data, bool) : YAMLSerializerBase(), Deserializer()
			{
				m_bWithMetadata = false;
				Data(data);
			}
			YAMLDeserializer() : YAMLSerializerBase(), Deserializer()
			{
				m_bIsValid = false;
			}
			virtual ~YAMLDeserializer(){}
		protected:
			virtual SerializerBase* GetAnother(const std::string& data) override { return new YAMLDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { throw std::exception(); }
		private:
			std::string m_dataStr;
		};
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
