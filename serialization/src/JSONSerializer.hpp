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
					SerialVect.front().second = strval.str();
				}
			}
			virtual void Reset() override { }
			using SerializerBase::Data;
			virtual const char *Data() override { return Str().c_str(); }
			virtual std::string Str() override
			{
				std::stringstream datastream;
				datastream << "{ ";
				for(size_t i =0; i < SerialVect.size(); i++)
				{
					auto &kvp = SerialVect[i];
					datastream << "\"" << kvp.first << "\" : " << kvp.second;
					if(i != SerialVect.size() - 1)
					{
						datastream << ", ";
					}
				}
				datastream << " }";
				return datastream.str();
			}
			virtual size_t Size() override
			{
				return Str().length();
			}
			bool More(){ return !SerialVect.empty() || !this_array.empty() || !this_object.empty(); }
		protected:
			template<typename T>
			friend class ReplicableBase;
			using SerializerBase::serialize;
			template<typename T>
			void serialize_impl(T *var, const size_t bytes, const std::string &name, bool)
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
					if(!StateTracker.empty())
					{
						if(!this_array.empty() && StateTracker.back() == State::Array)
						{
							if(bIsArray)
							{
								int i = 0;
								while(!this_array.back().second.empty())
								{
									strval = this_array.back().second.front();
									std::stringstream converter(strval);
									T newvar;
									converter >> newvar;
									var[i] = newvar;
									i++;
									this_array.back().second.erase(this_array.back().second.begin());
								}
							}
							else
							{
								strval = this_array.back().second.front();
								this_array.back().second.erase(this_array.back().second.begin());
								std::stringstream converter(strval);
								converter >> *var;
							}
						}
						else if(!this_object.empty() && StateTracker.back() == State::Object)
						{
							bool bFound = false;
							for(auto it = this_object.back().second.begin(); it != this_object.back().second.end(); it++)
							{
								if(it->first == name)
								{
									strval = it->second;
									this_object.back().second.erase(it);
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
						for(auto it = SerialVect.begin(); it != SerialVect.end(); it++)
						{
							if(it->first == name)
							{
								strval = it->second;
								SerialVect.erase(it);
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
							this_array.back().second.push_back(converter.str());
						}
					}
					else
					{
						converter << *var;
						if(!StateTracker.empty())
						{
							if(!this_array.empty() && StateTracker.back() == State::Array)
							{
								this_array.back().second.push_back(converter.str());
							}
							else if(!this_object.empty() && StateTracker.back() == State::Object)
							{
								this_object.back().second.push_back(std::make_pair(name, converter.str()));
							}
						}
						else
						{
							SerialVect.push_back(std::make_pair(name, converter.str()));
						}
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			void serialize_impl(char *var, const size_t, const std::string &name, bool)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsLoading())
				{
					std::string strval;
					if(!StateTracker.empty())
					{
						if(!this_array.empty() && StateTracker.back() == State::Array)
						{
							strval = this_array.back().second.front();
							this_array.back().second.erase(this_array.back().second.begin());
							strcpy(var, strval.substr(1, strval.length()-2).c_str());
						}
						else if(!this_object.empty() && StateTracker.back() == State::Object)
						{
							bool bFound = false;
							for(auto it = this_object.back().second.begin(); it != this_object.back().second.end(); it++)
							{
								if(it->first == name)
								{
									strval = it->second;
									this_object.back().second.erase(it);
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
						for(auto it = SerialVect.begin(); it != SerialVect.end(); it++)
						{
							if(it->first == name)
							{
								strval = it->second;
								SerialVect.erase(it);
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
					if(!StateTracker.empty())
					{
						if(!this_array.empty() && StateTracker.back() == State::Array)
						{
							this_array.back().second.push_back(converter.str());
						}
						else if(!this_object.empty() && StateTracker.back() == State::Object)
						{
							this_object.back().second.push_back(std::make_pair(name, converter.str()));
						}
					}
					else
					{
						SerialVect.push_back(std::make_pair(name, converter.str()));
					}
				}
			}

			void serialize_impl(std::string *var, const size_t, const std::string &name, bool)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsLoading())
				{
					std::string strval;
					if(!StateTracker.empty())
					{
						if(!this_array.empty() && StateTracker.back() == State::Array)
						{
							strval = this_array.back().second.front();
							this_array.back().second.erase(this_array.back().second.begin());
							*var = strval.substr(1, strval.length()-2);
						}
						else if(!this_object.empty() && StateTracker.back() == State::Object)
						{
							bool bFound = false;
							for(auto it = this_object.back().second.begin(); it != this_object.back().second.end(); it++)
							{
								if(it->first == name)
								{
									strval = it->second;
									this_object.back().second.erase(it);
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
						for(auto it = SerialVect.begin(); it != SerialVect.end(); it++)
						{
							if(it->first == name)
							{
								strval = it->second;
								SerialVect.erase(it);
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
					if(!StateTracker.empty())
					{
						if(!this_array.empty() && StateTracker.back() == State::Array)
						{
							this_array.back().second.push_back(converter.str());
						}
						else if(!this_object.empty() && StateTracker.back() == State::Object)
						{
							this_object.back().second.push_back(std::make_pair(name, converter.str()));
						}
					}
					else
					{
						SerialVect.push_back(std::make_pair(name, converter.str()));
					}
				}
			}

			virtual void serialize(int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long long int *var, const size_t bytes, const std::string &name, bool PersistToDB)  override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(short int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(char *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(float *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(double *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(long double *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(bool *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned long long int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned short int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual void serialize(unsigned char *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}
			virtual void serialize(std::string *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, bytes, name, PersistToDB);
			}

			virtual size_t StartObject(const std::string &str, bool = true) override
			{
				State LastState = StateTracker.empty() ? State::None : StateTracker.back();
				StateTracker.push_back(State::Object);
				if(IsSaving())
				{
					this_object.push_back(std::make_pair(str, std::deque<std::pair<std::string, std::string>>()));
					return 0; //doesn't matter.
				}
				else
				{
					bool bFound = false;
					std::string strval;
					if(!this_object.empty() && LastState == State::Object)
					{
						for(auto it = this_object.back().second.begin(); it != this_object.back().second.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								this_object.back().second.erase(it);
								bFound = true;
								break;
							}
						}
					}
					else if(!this_array.empty() && LastState == State::Array)
					{
						for(auto it = this_array.begin(); it != this_array.end(); it++)
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
						for(auto it = SerialVect.begin(); it != SerialVect.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								SerialVect.erase(it);
								bFound = true;
								break;
							}
						}
					}
					std::deque<std::pair<std::string, std::string>> jsondata;
					ParseJSON(strval, jsondata);
					this_object.push_back(std::make_pair(str, jsondata));
					return jsondata.size();
				}
			}
			virtual void EndObject() override
			{
				StateTracker.pop_back();
				if(IsSaving())
				{
					auto &kvp = this_object.back();
					std::string key = kvp.first;
					std::string objstr = "{ ";
					for(size_t i=0; i<kvp.second.size(); i++)
					{
						objstr += "\"" +  kvp.second[i].first + "\" : " + kvp.second[i].second;
						if(i != kvp.second.size() - 1)
						{
							objstr += ", ";
						}
					}
					objstr += " }";
					this_object.pop_back();
					if(!this_array.empty() && !StateTracker.empty() && StateTracker.back() == State::Array)
					{
						this_array.back().second.push_back(objstr);
					}
					else if(!this_object.empty() && !StateTracker.empty() && StateTracker.back() == State::Object)
					{
						this_object.back().second.push_back(std::make_pair(key, objstr));
					}
					else
					{
						SerialVect.push_back(std::make_pair(key, objstr));
					}
				}
				else
				{
					this_object.pop_back();
				}
			}

			virtual void StartArray(const std::string &str, size_t &size, bool = true) override
			{
				State LastState = StateTracker.empty() ? State::None : StateTracker.back();
				StateTracker.push_back(State::Array);
				if(IsSaving())
				{
					this_array.push_back(std::make_pair(str, std::deque<std::string>()));
				}
				else
				{
					bool bFound = false;
					std::string strval;
					size = 0;
					if(!this_array.empty() && LastState == State::Array)
					{
						strval = this_array.back().second.front();
						this_array.back().second.erase(this_array.back().second.begin());
					}
					else if(!this_object.empty() && LastState == State::Object)
					{
						for(auto it = this_object.back().second.begin(); it != this_object.back().second.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								this_object.back().second.erase(it);
								bFound = true;
								break;
							}
						}
					}
					else
					{
						for(auto it = SerialVect.begin(); it != SerialVect.end(); it++)
						{
							if(it->first == str)
							{
								strval = it->second;
								SerialVect.erase(it);
								bFound = true;
								break;
							}
						}
					}
					std::deque<std::pair<std::string, std::string>> jsondata;
					ParseJSON(strval, jsondata);
					std::deque<std::string> jsondata_nokeys;
					//Because arrays aren't in kvp format, the keys here will be in first, and second will be empty
					for(auto &kvp : jsondata)
					{
						size++;
						jsondata_nokeys.push_back(kvp.first);
					}
					this_array.push_back(std::make_pair(str, jsondata_nokeys));
				}
			}

			virtual void EndArray() override
			{
				StateTracker.pop_back();
				if(IsSaving())
				{
					auto &kvp = this_array.back();
					std::string key = kvp.first;
					std::string arrstr = "[ ";
					for(size_t i=0; i<kvp.second.size(); i++)
					{
						arrstr += kvp.second[i];
						if(i != kvp.second.size() - 1)
						{
							arrstr += ", ";
						}
					}
					arrstr += " ]";
					this_array.pop_back();
					if(!this_object.empty() && !StateTracker.empty() && StateTracker.back() == State::Object)
					{
						this_object.back().second.push_back(std::make_pair(key, arrstr));
					}
					else if(!this_array.empty() && !StateTracker.empty() && StateTracker.back() == State::Array)
					{
						if(key != this_array.back().first)
						{
							arrstr = "\"" + key + "\" : " + arrstr;
						}
						this_array.back().second.push_back(arrstr);
					}
					else
					{
						SerialVect.push_back(std::make_pair(key, arrstr));
					}
					std::stringstream converter;
				}
				else
				{
					this_array.pop_back();
				}
			}

			std::string GetNextKey()
			{
				if(IsSaving())
				{
					return "";
				}
				if(!StateTracker.empty())
				{
					if(!this_array.empty() && StateTracker.back() == State::Array)
					{
						return "";
					}
					else if(!this_object.empty() && StateTracker.back() == State::Object)
					{
						return this_object.back().second.front().first;
					}
					else
					{
						throw ex_serializer_overflow();
					}
				}
				else
				{
					if(SerialVect.empty())
					{
						throw ex_serializer_overflow();
					}
					return SerialVect.front().first;
				}
			}

			JSONSerializerBase()
				: SerializerBase()
				, m_version(0)
				, m_bIsValid(true)
				, m_bWithMetadata(true)
			{}
			virtual ~JSONSerializerBase() {}
		protected:
			static void ParseJSON(const std::string &str, std::deque<std::pair<std::string, std::string>> &ret)
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
			std::deque<State> StateTracker;
			std::deque<std::pair<std::string, std::deque<std::string>>> this_array;
			std::deque<std::pair<std::string, std::deque<std::pair<std::string, std::string>>>> this_object;
			std::deque<std::pair<std::string, std::string>> SerialVect;

			//Copied and pasted to avoid indirection with virtual inheritance
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bWithMetadata;
		private:
			JSONSerializerBase(const SerializerBase&);
			JSONSerializerBase &operator=(const SerializerBase&);
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
				this_array.clear();
				this_object.clear();
				SerialVect.clear();
				StateTracker.clear();
			}
			virtual SerializerBase &operator%(SerializationData<Serializer> &&var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase &operator%(SerializationData<JSONSerializer> &&var) override
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

			virtual ~JSONSerializer() {}
		protected:
			virtual SerializerBase* GetAnother(const std::string& /*data*/) override { throw std::exception(); }
			virtual SerializerBase* GetAnother() override { return new JSONSerializer(false); }
		};

		class JSONDeserializer : public JSONSerializerBase, public Deserializer
		{
		public:
			//Reset everything to original state.
			virtual void Reset() override { Data(datastr); }
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

			virtual SerializerBase &operator%(SerializationData<Deserializer> &&var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase &operator%(SerializationData<JSONDeserializer> &&var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}

			virtual void Data(const std::string &str) override
			{
				datastr = str;
				SerialVect.clear();
				ParseJSON(str, SerialVect);
				m_bIsValid = true;
				if(m_bWithMetadata)
					serialize(m_version, sizeof(m_version), "__version__", true);
			}
			JSONDeserializer(const std::string &data) : JSONSerializerBase(), Deserializer()
			{
				Data(data);
			}
			JSONDeserializer(const std::string &data, bool) : JSONSerializerBase(), Deserializer()
			{
				m_bWithMetadata = false;
				Data(data);
			}

			JSONDeserializer() : JSONSerializerBase(), Deserializer()
			{
				m_bIsValid = false;
			}
			virtual ~JSONDeserializer(){}
		protected:
			virtual SerializerBase* GetAnother(const std::string& data) override { return new JSONDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { throw std::exception(); }
		private:
			std::string datastr;
		};
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
