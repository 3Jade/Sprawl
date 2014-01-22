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

#include <mongo/client/dbclient.h>
#include "Serializer.hpp"
#include "JSONSerializer.hpp"
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
			virtual uint32_t GetVersion() override { return m_version; }
			virtual bool IsValid() override { return m_bIsValid; }
			virtual bool IsMongoStream() override { return true; }
			virtual void SetVersion(uint32_t i) override
			{
				m_version = i;
			}
			virtual void Reset() override
			{
				if(IsSaving())
				{
					this->m_bIsValid = true;
					for(auto arraybuilder : m_arrayBuilders)
					{
						delete arraybuilder.second;
					}
					m_arrayBuilders.clear();
					for(auto objbuilder : m_objectBuilders)
					{
						delete objbuilder.second;
					}
					m_objectBuilders.clear();
					delete m_builder;
					m_builder = new mongo::BSONObjBuilder();
				}
				else
				{
					m_arrays.clear();
					m_objects.clear();
				}
			}
			using SerializerBase::Data;
			virtual const char* Data() override
			{
				EnsureVersion();
				return m_builder->asTempObj().objdata();
			}
			virtual std::string Str() override
			{
				if(IsSaving())
				{
					EnsureVersion();
					return std::string(m_builder->asTempObj().objdata(), m_builder->asTempObj().objsize());
				}
				else
				{
					return m_obj.toString();
				}
			}
			std::string JSONStr()
			{
				if(IsSaving())
				{
					EnsureVersion();
					return m_builder->asTempObj().jsonString();
				}
				else
				{
					return m_obj.jsonString();
				}

			}

			virtual size_t Size() override
			{
				if(IsSaving())
				{
					return m_builder->asTempObj().objsize();
				}
				else
				{
					return m_obj.objsize();
				}
			}
			bool More() { return true; }
			mongo::BSONObj tempObj()
			{
				if(IsSaving())
				{
					EnsureVersion();
					return m_builder->asTempObj();
				}
				else
				{
					return m_obj;
				}
			}
			mongo::BSONObj Obj()
			{
				if(IsSaving())
				{
					EnsureVersion();
					m_bIsValid = false;
					return m_builder->obj();
				}
				else
				{
					return m_obj;
				}
			}
			void EnsureVersion()
			{
				if(m_bWithMetadata)
				{
					//This shouldn't happen unless someone's being dumb.
					//And even if they're being dumb, it shouldn't matter unless they're being extra dumb.
					//...but some people are extra dumb.
					if(m_builder->hasField("DataVersion"))
					{
						mongo::BSONObj obj = m_builder->obj();
						obj = obj.removeField("DataVersion");
						delete m_builder;
						m_builder = new mongo::BSONObjBuilder();
						m_builder->appendElements(obj);
					}
					m_builder->append("DataVersion", m_version);
				}
			}
		protected:
			template<typename T>
			friend class ReplicableBase;
			using SerializerBase::serialize;

			friend class MongoReplicableDeserializer;
			friend class MongoReplicableSerializer;
			virtual void serialize(mongo::OID* var, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(*var);
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						*var = m_arrays.back().second.front().OID();
						m_arrays.back().second.pop_front();
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].OID();
					}
					else
					{
						*var = m_obj[name].OID();
					}
				}
			}

			virtual void serialize(mongo::BSONObj* var, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(*var);
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						*var = m_arrays.back().second.front().Obj().copy();
						m_arrays.back().second.pop_front();
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Obj().copy();
					}
					else
					{
						*var = m_obj[name].Obj().copy();
					}
				}
			}

			virtual void serialize(mongo::Date_t* var, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(*var);
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						*var = m_arrays.back().second.front().Date();
						m_arrays.back().second.pop_front();
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Date();
					}
					else
					{
						*var = m_obj[name].Date();
					}
				}
			}


			virtual void serialize(int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(int))
				{
					size = bytes/sizeof(int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = m_arrays.back().second.front().Int();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = m_arrays.back().second.front().Int();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Int();
					}
					else
					{
						*var = m_obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(long int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(long int))
				{
					size = bytes/sizeof(long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append((long long int)var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append((long long int)*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, (long long int)*var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, (long long int)*var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = (long)m_arrays.back().second.front().Long();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = (long)m_arrays.back().second.front().Long();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = (long)m_objects.back()[name].Long();
					}
					else
					{
						*var = (long)m_obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(long long int* var, const size_t bytes, const std::string& name, bool PersistToDB)  override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(long long int))
				{
					size = bytes/sizeof(long long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = m_arrays.back().second.front().Long();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = m_arrays.back().second.front().Long();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Long();
					}
					else
					{
						*var = m_obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(short int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(short int))
				{
					size = bytes/sizeof(short int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = (short)m_arrays.back().second.front().Int();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = (short)m_arrays.back().second.front().Int();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = (short)m_objects.back()[name].Int();
					}
					else
					{
						*var = (short)m_obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(char* var, const size_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(*var);
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					std::string str;
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						str = m_arrays.back().second.front().String();
						m_arrays.back().second.pop_front();
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						str = m_objects.back()[name].String();
					}
					else
					{
						str = m_obj[name].String();
					}
					strcpy(var, str.c_str());
				}
			}

			virtual void serialize(float* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(float))
				{
					size = bytes/sizeof(float);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = (float)m_arrays.back().second.front().Double();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = (float)m_arrays.back().second.front().Double();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = (float)m_objects.back()[name].Double();
					}
					else
					{
						*var = (float)m_obj[name].Double();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(double* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(double))
				{
					size = bytes/sizeof(double);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = m_arrays.back().second.front().Double();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = m_arrays.back().second.front().Double();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Double();
					}
					else
					{
						*var = m_obj[name].Double();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(long double* /*var*/, const size_t /*bytes*/, const std::string& /*name*/, bool /*PersistToDB*/) override
			{
				throw std::exception();
			}

			virtual void serialize(bool* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(bool))
				{
					size = bytes/sizeof(bool);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = m_arrays.back().second.front().Bool();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = m_arrays.back().second.front().Bool();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Bool();
					}
					else
					{
						*var = m_obj[name].Bool();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(unsigned int))
				{
					size = bytes/sizeof(unsigned int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = m_arrays.back().second.front().Int();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = m_arrays.back().second.front().Int();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Int();
					}
					else
					{
						*var = m_obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned long int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(unsigned long int))
				{
					size = bytes/sizeof(unsigned long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append((long long int)var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append((long long int)*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, (long long int)*var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, (long long int)*var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = (unsigned long)m_arrays.back().second.front().Long();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = (unsigned long)m_arrays.back().second.front().Long();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = (unsigned long)m_objects.back()[name].Long();
					}
					else
					{
						*var = (unsigned long)m_obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned long long int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(unsigned long long int))
				{
					size = bytes/sizeof(unsigned long long int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append((long long int)var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append((long long int)*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, (long long int)*var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, (long long int)*var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = m_arrays.back().second.front().Long();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = m_arrays.back().second.front().Long();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].Long();
					}
					else
					{
						*var = m_obj[name].Long();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned short int* var, const size_t bytes, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				bool bIsArray = false;
				size_t size = 0;
				if(bytes != sizeof(unsigned short int))
				{
					size = bytes/sizeof(unsigned short int);
					bIsArray = true;
					StartArray(name, size, PersistToDB);
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								m_arrayBuilders.back().second->append(var[i]);
							}
						}
						else
						{
							m_arrayBuilders.back().second->append(*var);
						}
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						if(bIsArray)
						{
							for(size_t i=0; i<size; i++)
							{
								var[i] = (unsigned short)m_arrays.back().second.front().Int();
								m_arrays.back().second.pop_front();
							}
						}
						else
						{
							*var = (unsigned short)m_arrays.back().second.front().Int();
							m_arrays.back().second.pop_front();
						}
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = (unsigned short)m_objects.back()[name].Int();
					}
					else
					{
						*var = (unsigned short)m_obj[name].Int();
					}
				}
				if(bIsArray)
				{
					EndArray();
				}
			}

			virtual void serialize(unsigned char* var, const size_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(*var);
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					std::string str;
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						str = m_arrays.back().second.front().String();
						m_arrays.back().second.pop_front();
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						str = m_objects.back()[name].String();
					}
					else
					{
						str = m_obj[name].String();
					}
					strcpy((char*)var, str.c_str());
				}
			}
			virtual void serialize(std::string* var, const size_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				if(!PersistToDB || m_disableDepth)
				{
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(*var);
					}
					else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_objectBuilders.back().second->append(name, *var);
					}
					else
					{
						if(m_builder->hasField(name))
						{
							throw ex_duplicate_key_error(name);
						}
						m_builder->append(name, *var);
					}
				}
				else
				{
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						*var = m_arrays.back().second.front().String();
						m_arrays.back().second.pop_front();
					}
					else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						*var = m_objects.back()[name].String();
					}
					else
					{
						*var = m_obj[name].String();
					}
				}
			}

			virtual size_t StartObject(const std::string& str, bool PersistToDB = true) override
			{
				if(m_disableDepth || !PersistToDB)
				{
					m_disableDepth++;
					return 0;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					//Array fields don't care about name.
					if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					else if(m_arrayBuilders.empty() || m_stateTracker.empty() || m_stateTracker.back() != State::Array)
					{
						if(m_builder->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					m_objectBuilders.push_back(std::make_pair(str, new mongo::BSONObjBuilder()));
					m_stateTracker.push_back(State::Object);
					return 0; //doesn't matter.
				}
				else
				{
					if(!m_arrays.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array && m_arrays.back().first == str)
					{
						mongo::BSONObj o = m_arrays.back().second.front().Obj();
						m_arrays.back().second.pop_front();
						m_objects.push_back(o);
					}
					else if(!m_objects.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
					{
						mongo::BSONObj o = m_objects.back();
						m_objects.push_back(o[str].Obj());
					}
					else
					{
						m_objects.push_back(m_obj[str].Obj());
					}
					m_stateTracker.push_back(State::Object);
					std::list<mongo::BSONElement> elements;
					m_objects.back().elems(elements);
					m_elementList.push_back(elements);
					return m_objects.back().nFields();
				}
			}

			virtual void EndObject() override
			{
				if(m_disableDepth)
				{
					m_disableDepth--;
					return;
				}
				m_stateTracker.pop_back();
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					std::string key = m_objectBuilders.back().first;
					mongo::BSONObj o = m_objectBuilders.back().second->obj();
					delete m_objectBuilders.back().second;
					m_objectBuilders.pop_back();
					if(!m_arrayBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(o);
					}
					else if(!m_objectBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
					{
						m_objectBuilders.back().second->append(key, o);
					}
					else
					{
						m_builder->append(key, o);
					}
				}
				else
				{
					m_objects.pop_back();
					m_elementList.pop_back();
				}
			}

			virtual void StartArray(const std::string& str, size_t& size, bool PersistToDB = true) override
			{
				if(m_disableDepth || !PersistToDB)
				{
					m_disableDepth++;
					return;
				}
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					//Array fields don't care about name.
					if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
					{
						if(m_objectBuilders.back().second->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					else if(m_arrayBuilders.empty() || m_stateTracker.empty() || m_stateTracker.back() != State::Array)
					{
						if(m_builder->hasField(str))
						{
							throw ex_duplicate_key_error(str);
						}
					}
					m_arrayBuilders.push_back(std::make_pair(str, new mongo::BSONArrayBuilder()));
				}
				else
				{
					if(!m_arrays.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array && m_arrays.back().first == str)
					{
						std::vector<mongo::BSONElement> elements = m_arrays.back().second.front().Array();
						m_arrays.back().second.pop_front();
						m_arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
						size = elements.size();
					}
					else if(!m_objects.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
					{
						mongo::BSONObj o = m_objects.back();
						std::vector<mongo::BSONElement> elements = o[str].Array();
						m_arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
						size = elements.size();
					}
					else
					{
						std::vector<mongo::BSONElement> elements = m_obj[str].Array();
						m_arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
						size = elements.size();
					}
				}
				m_stateTracker.push_back(State::Array);
			}

			virtual void EndArray() override
			{
				if(m_disableDepth)
				{
					m_disableDepth--;
					return;
				}
				m_stateTracker.pop_back();
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					std::string key = m_arrayBuilders.back().first;
					mongo::BSONArray arr = m_arrayBuilders.back().second->arr();
					delete m_arrayBuilders.back().second;
					m_arrayBuilders.pop_back();
					if(!m_arrayBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array)
					{
						m_arrayBuilders.back().second->append(arr);
					}
					else if(!m_objectBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
					{
						m_objectBuilders.back().second->append(key, arr);
					}
					else
					{
						m_builder->append(key, arr);
					}
				}
				else
				{
					m_arrays.pop_back();
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
					if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
					{
						return "";
					}
				}
					/*else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
					{
						return m_objects.back().firstElementFieldName();
					}
					else
					{
						throw ex_serializer_overflow();
					}
				}
				else
				{
					if(m_obj.nFields() == 0)
					{
						throw ex_serializer_overflow();
					}
					return m_obj.firstElementFieldName();
				}*/
				if(!m_elementList.empty())
				{
					std::string ret = m_elementList.back().front().fieldName();
					m_elementList.back().pop_front();
					return ret;
				}
				return "";
			}

			MongoSerializerBase()
				: SerializerBase()
				, m_disableDepth(0)
				, m_obj()
				, m_builder(new mongo::BSONObjBuilder())
				, m_arrayBuilders()
				, m_objectBuilders()
				, m_arrays()
				, m_objects()
				, m_stateTracker()
				, m_version(0)
				, m_bIsValid(true)
				, m_bWithMetadata(true)
				, m_elementList()
			{}
			virtual ~MongoSerializerBase()
			{
				for(auto arraybuilder : m_arrayBuilders)
				{
					delete arraybuilder.second;
				}
				m_arrayBuilders.clear();
				for(auto objbuilder : m_objectBuilders)
				{
					delete objbuilder.second;
				}
				delete m_builder;
			}

		protected:
			int m_disableDepth;
			enum class State { None, Array, Object };
			mongo::BSONObj m_obj;
			mongo::BSONObjBuilder* m_builder;
			std::deque<std::pair<std::string, mongo::BSONArrayBuilder*>> m_arrayBuilders;
			std::deque<std::pair<std::string, mongo::BSONObjBuilder*>> m_objectBuilders;
			std::deque<std::pair<std::string, std::deque<mongo::BSONElement>>> m_arrays;
			std::deque<mongo::BSONObj> m_objects;
			std::deque<State> m_stateTracker;
			std::deque<std::list<mongo::BSONElement>> m_elementList;

			//Copied and pasted to avoid indirection with virtual inheritance
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bWithMetadata;
		private:
			MongoSerializerBase(const SerializerBase&);
			MongoSerializerBase& operator=(const SerializerBase&);
		};

		class MongoSerializer : public MongoSerializerBase, public Serializer
		{
		public:
			using Serializer::operator%;
			using Serializer::IsLoading;

			using MongoSerializerBase::serialize;
			using MongoSerializerBase::IsBinary;
			using MongoSerializerBase::IsMongoStream;
			using MongoSerializerBase::IsReplicable;
			using MongoSerializerBase::IsValid;
			using MongoSerializerBase::Str;
			using MongoSerializerBase::Data;
			using MongoSerializerBase::GetVersion;
			using MongoSerializerBase::SetVersion;
			using MongoSerializerBase::Reset;
			using MongoSerializerBase::Size;

			using MongoSerializerBase::StartObject;
			using MongoSerializerBase::EndObject;
			using MongoSerializerBase::StartArray;
			using MongoSerializerBase::EndArray;
			using MongoSerializerBase::StartMap;
			using MongoSerializerBase::EndMap;
			using MongoSerializerBase::GetNextKey;
			using MongoSerializerBase::GetDeletedKeys;

			virtual SerializerBase& operator%(SerializationData<Serializer>&& var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase& operator%(SerializationData<MongoSerializer>&& var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			//Reserve the first sizeof(int32_t)*3 bytes of space to hold metadata (size, version, and checksum).
			MongoSerializer()
			{
				m_bIsValid = true;
			}
			MongoSerializer(bool)
			{
				m_bIsValid = true;
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
			virtual void Reset() override { MongoSerializerBase::Reset(); Data(m_dataStr); }
			using Deserializer::operator%;
			using Deserializer::IsLoading;

			using MongoSerializerBase::serialize;
			using MongoSerializerBase::IsBinary;
			using MongoSerializerBase::IsMongoStream;
			using MongoSerializerBase::IsReplicable;
			using MongoSerializerBase::IsValid;
			using MongoSerializerBase::Str;
			using MongoSerializerBase::Data;
			using MongoSerializerBase::GetVersion;
			using MongoSerializerBase::SetVersion;
			using MongoSerializerBase::Size;

			using MongoSerializerBase::StartObject;
			using MongoSerializerBase::EndObject;
			using MongoSerializerBase::StartArray;
			using MongoSerializerBase::EndArray;
			using MongoSerializerBase::StartMap;
			using MongoSerializerBase::EndMap;
			using MongoSerializerBase::GetNextKey;
			using MongoSerializerBase::GetDeletedKeys;

			virtual SerializerBase& operator%(SerializationData<Deserializer>&& var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase& operator%(SerializationData<MongoDeserializer>&& var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}

			void Data(const mongo::BSONObj& o)
			{
				m_obj = o;
				m_bIsValid = true;
			}

			virtual void Data(const std::string& str) override
			{
				if(str[0] == '{')
				{
					m_obj = mongo::fromjson( str.c_str() );
				}
				else
				{
					m_obj = mongo::BSONObj(str.c_str()).copy();
				}
				m_bIsValid = true;
			}

			virtual void Data(const char* data, size_t /*length*/) override
			{
				if(data[0] == '{')
				{
					m_obj = mongo::fromjson(data);
				}
				else
				{
					m_obj = mongo::BSONObj(data).copy();
				}
				m_bIsValid = true;
			}

			MongoDeserializer(const std::string& data)
			{
				Data(data);
				m_version = m_obj["DataVersion"].Int();
			}

			MongoDeserializer(const std::string& data, bool)
			{
				Data(data);
				m_bWithMetadata = false;
			}

			MongoDeserializer(const char* data, size_t length)
			{
				Data(data, length);
				m_version = m_obj["DataVersion"].Int();
			}

			MongoDeserializer(const char* data, size_t length, bool)
			{
				Data(data, length);
				m_bWithMetadata = false;
			}

			MongoDeserializer(const mongo::BSONObj& o)
			{
				Data(o);
				m_version = m_obj["DataVersion"].Int();
			}

			MongoDeserializer(const mongo::BSONObj& o, bool)
			{
				Data(o);
				m_bWithMetadata = false;
			}

			MongoDeserializer()
			{
				m_bWithMetadata = false;
				m_bIsValid = false;
			}

			MongoDeserializer(bool)
			{
				m_bWithMetadata = false;
				m_bIsValid = false;
			}

			virtual ~MongoDeserializer(){}
		private:
			std::string m_dataStr;
		protected:
			//Anything binary (including BSON) doesn't work here, it makes Mongo freak out if an object is embedded as a key like this. So we'll embed JSON instead.
			virtual SerializerBase* GetAnother(const std::string& data) override { return new JSONDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { throw std::exception(); }
		};

		inline SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::OID>&& var)
		{
			if(s.IsMongoStream())
			{
				s.serialize(&var.val, var.name, var.PersistToDB);
			}
			else
			{
				if(s.IsLoading())
				{
					std::string str;
					s % prepare_data(str, var.name, var.PersistToDB);
					var.val.init(str);
				}
				else
				{
					s % prepare_data(var.val.str(), var.name, var.PersistToDB);
				}
			}
			return s;
		}

		inline SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::BSONObj>&& var)
		{
			if(s.IsMongoStream())
			{
				s.serialize(&var.val, var.name, var.PersistToDB);
			}
			else
			{
				if(s.IsLoading())
				{
					std::string str;
					s % prepare_data(str, var.name, var.PersistToDB);
					if(s.IsBinary())
					{
						var.val = mongo::BSONObj(str.c_str()).copy();
					}
					else
					{
						var.val = mongo::fromjson(str.c_str());
					}
				}
				else
				{
					if(s.IsBinary())
					{
						s % prepare_data(std::string(var.val.objdata(), var.val.objsize()), var.name, var.PersistToDB);
					}
					else
					{
						s % prepare_data(var.val.jsonString(), var.name, var.PersistToDB);
					}
				}
			}
			return s;
		}

		inline SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::Date_t>&& var)
		{
			if(s.IsMongoStream())
			{
				s.serialize(&var.val, var.name, var.PersistToDB);
			}
			else
			{
				if(s.IsLoading())
				{
					int64_t val;
					s % prepare_data(val, var.name, var.PersistToDB);
					var.val = mongo::Date_t(val);
				}
				else
				{
					s % prepare_data(var.val.millis, var.name, var.PersistToDB);
				}
			}
			return s;
		}

	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
