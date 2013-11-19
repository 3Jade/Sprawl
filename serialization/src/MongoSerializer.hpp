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

#include "Serializer.hpp"
#include "JSONSerializer.hpp"
#include <mongo/client/dbclient.h>
#include <deque>

namespace sprawl
{
	namespace serialization
	{
		class ex_duplicate_key_error: public std::exception
		{
		public:
			ex_duplicate_key_error(const std::string& key_) throw()
				: key(key_)
			{
				//
			}

			const char* what() const throw ()
			{
				std::string str = "MongoSerializer does not support fields with duplicate keys: ";
				str += key;
				return str.c_str();
			}
			~ex_duplicate_key_error() throw ()
			{
			}
		private:
			std::string key;
		};
		class MongoSerializerBase : virtual public SerializerBase
		{
		public:
			typedef class MongoSerializer serializer_type;
			typedef class MongoDeserializer deserializer_type;
			virtual void SetVersion(uint32_t i) override
			{
				m_version = i;
				if(IsSaving() && m_bWithMetadata)
				{
					//Update version metadata
					mongo::BSONObj obj = builder->obj();
					delete builder;
					builder = new mongo::BSONObjBuilder();
					builder->append("__version__", m_version);
					builder->appendElementsUnique(obj);
				}
			}
			virtual void Reset() override
			{
				if(IsSaving())
				{
					this->m_bIsValid = true;
					for(auto arraybuilder : arraybuilders)
					{
						delete arraybuilder.second;
					}
					arraybuilders.clear();
					for(auto objbuilder : objectbuilders)
					{
						delete objbuilder.second;
					}
					objectbuilders.clear();
					delete builder;
					builder = new mongo::BSONObjBuilder();
				}
				else
				{
					arrays.clear();
					objects.clear();
				}
			}
			using SerializerBase::Data;
			virtual const char *Data() override { return builder->asTempObj().objdata(); }
			virtual std::string Str() override
			{
				if(IsSaving())
				{
					return std::string(builder->asTempObj().objdata(), builder->asTempObj().objsize());
				}
				else
				{
					return obj.toString();
				}
			}
			std::string JSONStr()
			{
				if(IsSaving())
				{
					return builder->asTempObj().jsonString();
				}
				else
				{
					return obj.jsonString();
				}

			}

			virtual int Size() override
			{
				if(IsSaving())
				{
					return builder->asTempObj().objsize();
				}
				else
				{
					return obj.objsize();
				}
			}
			bool More() { return true; }
			mongo::BSONObj tempObj()
			{
				if(IsSaving())
				{
					return builder->asTempObj();
				}
				else
				{
					return obj;
				}
			}
			mongo::BSONObj Obj()
			{
				if(IsSaving())
				{
					m_bIsValid = false;
					return builder->obj();
				}
				else
				{
					return obj;
				}
			}
		protected:
			using SerializerBase::serialize;

			virtual void serialize(int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(int))
				{
					size = bytes/sizeof(int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Int();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Int();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Int();
					}
					else
					{
						*var = obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(long int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(long int))
				{
					size = bytes/sizeof(long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append((long long int)var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append((long long int)*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, (long long int)*var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, (long long int)*var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Long();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Long();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Long();
					}
					else
					{
						*var = obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(long long int *var, const size_t bytes, const std::string &name, bool PersistToDB)  override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(long long int))
				{
					size = bytes/sizeof(long long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Long();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Long();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Long();
					}
					else
					{
						*var = obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(short int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(short int))
				{
					size = bytes/sizeof(short int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Int();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Int();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Int();
					}
					else
					{
						*var = obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(char *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						arraybuilders.back().second->append(*var);
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					std::string str;
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						str = arrays.back().second.front().String();
						arrays.back().second.pop_front();
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						str = objects.back()[name].String();
					}
					else
					{
						str = obj[name].String();
					}
					strcpy(var, str.c_str());
				}
			}

			virtual void serialize(float *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(float))
				{
					size = bytes/sizeof(float);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Double();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Double();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Double();
					}
					else
					{
						*var = obj[name].Double();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(double *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(double))
				{
					size = bytes/sizeof(double);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Double();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Double();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Double();
					}
					else
					{
						*var = obj[name].Double();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(long double */*var*/, const size_t /*bytes*/, const std::string &/*name*/, bool /*PersistToDB*/) override
			{
				throw std::exception();
			}

			virtual void serialize(bool *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(bool))
				{
					size = bytes/sizeof(bool);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Bool();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Bool();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Bool();
					}
					else
					{
						*var = obj[name].Bool();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(unsigned int))
				{
					size = bytes/sizeof(unsigned int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Int();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Int();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Int();
					}
					else
					{
						*var = obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned long int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(unsigned long int))
				{
					size = bytes/sizeof(unsigned long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append((long long int)var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append((long long int)*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, (long long int)*var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, (long long int)*var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Long();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Long();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Long();
					}
					else
					{
						*var = obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned long long int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(unsigned long long int))
				{
					size = bytes/sizeof(unsigned long long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append((long long int)var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append((long long int)*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, (long long int)*var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, (long long int)*var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Long();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Long();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Long();
					}
					else
					{
						*var = obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned short int *var, const size_t bytes, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray;
				size_t size;
				if(bytes != sizeof(unsigned short int))
				{
					size = bytes/sizeof(unsigned short int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								arraybuilders.back().second->append(var[i]);
							}
						}
						else
						{
							arraybuilders.back().second->append(*var);
						}
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = arrays.back().second.front().Int();
								arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = arrays.back().second.front().Int();
							arrays.back().second.pop_front();
						}
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].Int();
					}
					else
					{
						*var = obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned char *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						arraybuilders.back().second->append(*var);
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					std::string str;
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						str = arrays.back().second.front().String();
						arrays.back().second.pop_front();
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						str = objects.back()[name].String();
					}
					else
					{
						str = obj[name].String();
					}
					strcpy((char*)var, str.c_str());
				}
			}
			virtual void serialize(std::string *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				if(!PersistToDB || DisablePersistence)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!arraybuilders.empty() && StateTracker.back() == State::Array)
					{
						arraybuilders.back().second->append(*var);
					}
					else if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						objectbuilders.back().second->append(name, *var);
					}
					else
					{
						if(builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						builder->append(name, *var);
					}
				}
				else
				{
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						*var = arrays.back().second.front().String();
						arrays.back().second.pop_front();
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						*var = objects.back()[name].String();
					}
					else
					{
						*var = obj[name].String();
					}
				}
			}

			virtual int StartObject(const std::string &str, bool PersistToDB = true) override
			{
				if(DisablePersistence || !PersistToDB)
				{
					DisablePersistence++;
					return 0;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					//Array fields don't care about name.
					if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					else if(arraybuilders.empty() || StateTracker.empty() || StateTracker.back() != State::Array)
					{
						if(builder->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					objectbuilders.push_back(std::make_pair(str, new mongo::BSONObjBuilder()));
					StateTracker.push_back(State::Object);
					return 0; //doesn't matter.
				}
				else
				{
					if(!arrays.empty() && !StateTracker.empty() && StateTracker.back() == State::Array && arrays.back().first == str)
					{
						mongo::BSONObj o = arrays.back().second.front().Obj();
						arrays.back().second.pop_front();
						objects.push_back(o);
					}
					else if(!objects.empty() && !StateTracker.empty() && StateTracker.back() == State::Object)
					{
						mongo::BSONObj o = objects.back();
						objects.push_back(o[str].Obj());
					}
					else
					{
						objects.push_back(obj[str].Obj());
					}
					StateTracker.push_back(State::Object);
					return objects.back().nFields();
				}
			}

			virtual void EndObject() override
			{
				if(DisablePersistence)
				{
					DisablePersistence--;
					return;
				}
				StateTracker.pop_back();
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					std::string key = objectbuilders.back().first;
					mongo::BSONObj o = objectbuilders.back().second->obj();
					delete objectbuilders.back().second;
					objectbuilders.pop_back();
					if(!arraybuilders.empty() && !StateTracker.empty() && StateTracker.back() == State::Array)
					{
						arraybuilders.back().second->append(o);
					}
					else if(!objectbuilders.empty() && !StateTracker.empty() && StateTracker.back() == State::Object)
					{
						objectbuilders.back().second->append(key, o);
					}
					else
					{
						builder->append(key, o);
					}
				}
				else
				{
					objects.pop_back();
				}
			}

			virtual void StartArray(const std::string &str, size_t &size, bool PersistToDB = true) override
			{
				if(DisablePersistence || !PersistToDB)
				{
					DisablePersistence++;
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					//Array fields don't care about name.
					if(!objectbuilders.empty() && StateTracker.back() == State::Object)
					{
						if(objectbuilders.back().second->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					else if(arraybuilders.empty() || StateTracker.empty() || StateTracker.back() != State::Array)
					{
						if(builder->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					arraybuilders.push_back(std::make_pair(str, new mongo::BSONArrayBuilder()));
				}
				else
				{
					if(!arrays.empty() && !StateTracker.empty() && StateTracker.back() == State::Array && arrays.back().first == str)
					{
						std::vector<mongo::BSONElement> elements = arrays.back().second.front().Array();
						arrays.back().second.pop_front();
						arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
						size = elements.size();
					}
					else if(!objects.empty() && !StateTracker.empty() && StateTracker.back() == State::Object)
					{
						mongo::BSONObj o = objects.back();
						std::vector<mongo::BSONElement> elements = o[str].Array();
						arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
						size = elements.size();
					}
					else
					{
						std::vector<mongo::BSONElement> elements = obj[str].Array();
						arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
						size = elements.size();
					}
				}
				StateTracker.push_back(State::Array);
			}

			virtual void EndArray() override
			{
				if(DisablePersistence)
				{
					DisablePersistence--;
					return;
				}
				StateTracker.pop_back();
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					std::string key = arraybuilders.back().first;
					mongo::BSONArray arr = arraybuilders.back().second->arr();
					delete arraybuilders.back().second;
					arraybuilders.pop_back();
					if(!arraybuilders.empty() && !StateTracker.empty() && StateTracker.back() == State::Array)
					{
						arraybuilders.back().second->append(arr);
					}
					else if(!objectbuilders.empty() && !StateTracker.empty() && StateTracker.back() == State::Object)
					{
						objectbuilders.back().second->append(key, arr);
					}
					else
					{
						builder->append(key, arr);
					}
				}
				else
				{
					arrays.pop_back();
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
					if(!arrays.empty() && StateTracker.back() == State::Array)
					{
						return "";
					}
					else if(!objects.empty() && StateTracker.back() == State::Object)
					{
						return objects.back().firstElement().fieldName();
					}
					else
					{
						throw ex_serializer_overflow();
					}
				}
				else
				{
					if(obj.nFields() == 0)
					{
						throw ex_serializer_overflow();
					}
					return obj.firstElement().fieldName();
				}
			}

			MongoSerializerBase() : SerializerBase(), DisablePersistence(0), obj(), builder(new mongo::BSONObjBuilder()), arraybuilders(), objectbuilders(), arrays(), objects(), StateTracker() {}
			virtual ~MongoSerializerBase()
			{
				for(auto arraybuilder : arraybuilders)
				{
					delete arraybuilder.second;
				}
				arraybuilders.clear();
				for(auto objbuilder : objectbuilders)
				{
					delete objbuilder.second;
				}
				delete builder;
			}

		protected:
			int DisablePersistence;
			enum class State { None, Array, Object };
			mongo::BSONObj obj;
			mongo::BSONObjBuilder *builder;
			std::deque<std::pair<std::string, mongo::BSONArrayBuilder*>> arraybuilders;
			std::deque<std::pair<std::string, mongo::BSONObjBuilder*>> objectbuilders;
			std::deque<std::pair<std::string, std::deque<mongo::BSONElement>>> arrays;
			std::deque<mongo::BSONObj> objects;
			std::deque<State> StateTracker;
		private:
			MongoSerializerBase(const SerializerBase&);
			MongoSerializerBase &operator=(const SerializerBase&);
		};

		class MongoSerializer : public MongoSerializerBase, public Serializer
		{
		public:
			using Serializer::operator%;

			virtual SerializerBase &operator%(SerializationData<Serializer> &&var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase &operator%(SerializationData<MongoSerializer> &&var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			//Reserve the first sizeof(int32_t)*3 bytes of space to hold metadata (size, version, and checksum).
			MongoSerializer()
			{
				m_bIsLoading = false;
				m_bIsValid = true;
				SetVersion(0);
				m_bInitialized = true;
			}
			MongoSerializer(bool)
			{
				m_bIsLoading = false;
				m_bIsValid = true;
				SetVersion(0);
				m_bInitialized = true;
				m_bWithMetadata = false;
			}

			virtual ~MongoSerializer() {};
		protected:
			virtual SerializerBase* GetAnother(const std::string& /*data*/) override { throw std::exception(); }
			//Anything binary (including BSON) doesn't work here, it makes Mongo freak out if an object is embedded as a key like this. So we'll embed JSON instead.
			virtual SerializerBase* GetAnother() override { return new JSONSerializer(false); }
		};

		class MongoDeserializer : public MongoSerializerBase, public Deserializer
		{
		public:
			//Reset everything to original state.
			virtual void Reset() override { MongoSerializerBase::Reset(); Data(datastr); }
			using Deserializer::operator%;
			virtual SerializerBase &operator%(SerializationData<Deserializer> &&var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase &operator%(SerializationData<MongoDeserializer> &&var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}

			using MongoSerializerBase::Data;
			void Data(const mongo::BSONObj &o)
			{
				obj = o;
				m_bInitialized = true;
				m_bIsValid = true;
			}

			virtual void Data(const std::string &str) override
			{
				if(str[0] == '{')
				{
					obj = mongo::fromjson( str.c_str() );
				}
				else
				{
					obj = mongo::BSONObj(str.c_str());
				}
				m_bInitialized = true;
				m_bIsValid = true;
			}

			MongoDeserializer(const std::string &data)
			{
				m_bIsLoading = true;
				Data(data);
				m_version = obj["__version__"].Int();
			}

			MongoDeserializer(const std::string& data, bool)
			{
				m_bIsLoading = true;
				Data(data);
				m_bWithMetadata = false;
			}

			MongoDeserializer(const mongo::BSONObj &o)
			{
				m_bIsLoading = true;
				Data(o);
				m_version = obj["__version__"].Int();
			}
			virtual ~MongoDeserializer(){}
		private:
			std::string datastr;
		protected:
			//Anything binary (including BSON) doesn't work here, it makes Mongo freak out if an object is embedded as a key like this. So we'll embed JSON instead.
			virtual SerializerBase* GetAnother(const std::string& data) override { return new JSONDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { throw std::exception(); }
		};
	}
}
