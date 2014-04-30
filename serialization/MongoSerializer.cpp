#include "MongoSerializer.hpp"

namespace sprawl
{
	namespace serialization
	{

		void MongoSerializerBase::Reset()
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

		void MongoSerializerBase::EnsureVersion()
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

		SerializerBase& MongoSerializerBase::operator%(BinaryData&& var)
		{
			if(!var.PersistToDB || m_disableDepth)
			{
				return *this;
			}
			if(!m_bIsValid)
			{
				throw ex_invalid_data();
			}
			if(IsSaving())
			{
				if(!m_arrayBuilders.empty() && m_stateTracker.back() == State::Array)
				{
					mongo::BSONObjBuilder builder;
					builder.appendBinData("binaryData", var.size, mongo::BinDataGeneral, var.val);
					m_arrayBuilders.back().second->append(builder.obj());
				}
				else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
				{
					if(m_objectBuilders.back().second->hasField(var.name.c_str()))
					{
						throw ex_duplicate_key_error(var.name);
					}
					m_objectBuilders.back().second->appendBinData(var.name.c_str(), var.size, mongo::BinDataGeneral, var.val);
				}
				else
				{
					if(m_builder->hasField(var.name.c_str()))
					{
						throw ex_duplicate_key_error(var.name.c_str());
					}
					m_builder->appendBinData(var.name.c_str(), var.size, mongo::BinDataGeneral, var.val);
				}
			}
			else
			{
				const char* str;
				int size = 0;
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					str = m_arrays.back().second.front().Obj()["binaryData"].binData(size);
					m_arrays.back().second.pop_front();
				}
				else if(!m_objects.empty() && m_stateTracker.back() == State::Object)
				{
					str = m_objects.back()[var.name.c_str()].binData(size);
				}
				else
				{
					str = m_obj[var.name.c_str()].binData(size);
				}
				if(var.size != (uint32_t)size)
				{
					throw ex_serializer_overflow();
				}
				memcpy(var.val, str, size);
			}
			return *this;
		}

		uint32_t MongoSerializerBase::StartObject(const String& str, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(str.c_str()))
					{
						throw ex_duplicate_key_error(str);
					}
				}
				else if(m_arrayBuilders.empty() || m_stateTracker.empty() || m_stateTracker.back() != State::Array)
				{
					if(m_builder->hasField(str.c_str()))
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
					m_objects.push_back(o[str.c_str()].Obj());
				}
				else
				{
					m_objects.push_back(m_obj[str.c_str()].Obj());
				}
				m_stateTracker.push_back(State::Object);
				std::list<mongo::BSONElement> elements;
				m_objects.back().elems(elements);
				m_elementList.push_back(elements);
				return m_objects.back().nFields();
			}
		}

		void MongoSerializerBase::EndObject()
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
				sprawl::String key = m_objectBuilders.back().first;
				mongo::BSONObj o = m_objectBuilders.back().second->obj();
				delete m_objectBuilders.back().second;
				m_objectBuilders.pop_back();
				if(!m_arrayBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array)
				{
					m_arrayBuilders.back().second->append(o);
				}
				else if(!m_objectBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
				{
					m_objectBuilders.back().second->append(key.c_str(), o);
				}
				else
				{
					m_builder->append(key.c_str(), o);
				}
			}
			else
			{
				m_objects.pop_back();
				m_elementList.pop_back();
			}
		}

		void MongoSerializerBase::StartArray(const String& str, uint32_t& size, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(str.c_str()))
					{
						throw ex_duplicate_key_error(str);
					}
				}
				else if(m_arrayBuilders.empty() || m_stateTracker.empty() || m_stateTracker.back() != State::Array)
				{
					if(m_builder->hasField(str.c_str()))
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
					std::vector<mongo::BSONElement> elements = o[str.c_str()].Array();
					m_arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
					size = elements.size();
				}
				else
				{
					std::vector<mongo::BSONElement> elements = m_obj[str.c_str()].Array();
					m_arrays.push_back(std::make_pair(str, std::deque<mongo::BSONElement>(elements.begin(), elements.end())));
					size = elements.size();
				}
			}
			m_stateTracker.push_back(State::Array);
		}

		void MongoSerializerBase::EndArray()
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
				sprawl::String key = m_arrayBuilders.back().first;
				mongo::BSONArray arr = m_arrayBuilders.back().second->arr();
				delete m_arrayBuilders.back().second;
				m_arrayBuilders.pop_back();
				if(!m_arrayBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Array)
				{
					m_arrayBuilders.back().second->append(arr);
				}
				else if(!m_objectBuilders.empty() && !m_stateTracker.empty() && m_stateTracker.back() == State::Object)
				{
					m_objectBuilders.back().second->append(key.c_str(), arr);
				}
				else
				{
					m_builder->append(key.c_str(), arr);
				}
			}
			else
			{
				m_arrays.pop_back();
			}
		}

		String MongoSerializerBase::GetNextKey()
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
				sprawl::String ret(m_elementList.back().front().fieldName());
				m_elementList.back().pop_front();
				return ret;
			}
			return "";
		}

		MongoSerializerBase::~MongoSerializerBase()
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

		void MongoSerializerBase::serialize(String* var, const uint32_t, const String& name, bool PersistToDB)
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
					m_arrayBuilders.back().second->append(var->toStdString());
				}
				else if(!m_objectBuilders.empty() && m_stateTracker.back() == State::Object)
				{
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), var->toStdString());
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), var->toStdString());
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
					*var = m_objects.back()[name.c_str()].String();
				}
				else
				{
					*var = m_obj[name.c_str()].String();
				}
			}
		}

		void MongoSerializerBase::serialize(std::string* var, const uint32_t, const String& name, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
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
					*var = m_objects.back()[name.c_str()].String();
				}
				else
				{
					*var = m_obj[name.c_str()].String();
				}
			}
		}

		void MongoSerializerBase::serialize(unsigned char* var, const uint32_t, const String& name, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
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
					str = m_objects.back()[name.c_str()].String();
				}
				else
				{
					str = m_obj[name.c_str()].String();
				}
				strcpy((char*)var, str.c_str());
			}
		}

		void MongoSerializerBase::serialize(unsigned short* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = (unsigned short)m_objects.back()[name.c_str()].Int();
				}
				else
				{
					*var = (unsigned short)m_obj[name.c_str()].Int();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(unsigned long long* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), (long long int)*var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), (long long int)*var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = m_objects.back()[name.c_str()].Long();
				}
				else
				{
					*var = m_obj[name.c_str()].Long();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(unsigned long* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), (long long int)*var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), (long long int)*var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = (unsigned long)m_objects.back()[name.c_str()].Long();
				}
				else
				{
					*var = (unsigned long)m_obj[name.c_str()].Long();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(unsigned int* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = m_objects.back()[name.c_str()].Int();
				}
				else
				{
					*var = m_obj[name.c_str()].Int();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(bool* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = m_objects.back()[name.c_str()].Bool();
				}
				else
				{
					*var = m_obj[name.c_str()].Bool();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(double* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = m_objects.back()[name.c_str()].Double();
				}
				else
				{
					*var = m_obj[name.c_str()].Double();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(float* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = (float)m_objects.back()[name.c_str()].Double();
				}
				else
				{
					*var = (float)m_obj[name.c_str()].Double();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(char* var, const uint32_t, const String& name, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
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
					str = m_objects.back()[name.c_str()].String();
				}
				else
				{
					str = m_obj[name.c_str()].String();
				}
				strcpy(var, str.c_str());
			}
		}

		void MongoSerializerBase::serialize(short* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = (short)m_objects.back()[name.c_str()].Int();
				}
				else
				{
					*var = (short)m_obj[name.c_str()].Int();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(long long* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = m_objects.back()[name.c_str()].Long();
				}
				else
				{
					*var = m_obj[name.c_str()].Long();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(long* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), (long long int)*var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), (long long int)*var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = (long)m_objects.back()[name.c_str()].Long();
				}
				else
				{
					*var = (long)m_obj[name.c_str()].Long();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(int* var, const uint32_t bytes, const String& name, bool PersistToDB)
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
			uint32_t size = 0;
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
						for(uint32_t i=0; i<size; i++)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
				}
			}
			else
			{
				if(!m_arrays.empty() && m_stateTracker.back() == State::Array)
				{
					if(bIsArray)
					{
						for(uint32_t i=0; i<size; i++)
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
					*var = m_objects.back()[name.c_str()].Int();
				}
				else
				{
					*var = m_obj[name.c_str()].Int();
				}
			}
			if(bIsArray)
			{
				EndArray();
			}
		}

		void MongoSerializerBase::serialize(mongo::Date_t* var, const String& name, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
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
					*var = m_objects.back()[name.c_str()].Date();
				}
				else
				{
					*var = m_obj[name.c_str()].Date();
				}
			}
		}

		void MongoSerializerBase::serialize(mongo::BSONObj* var, const String& name, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
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
					*var = m_objects.back()[name.c_str()].Obj().copy();
				}
				else
				{
					*var = m_obj[name.c_str()].Obj().copy();
				}
			}
		}

		void MongoSerializerBase::serialize(mongo::OID* var, const String& name, bool PersistToDB)
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
					if(m_objectBuilders.back().second->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_objectBuilders.back().second->append(name.c_str(), *var);
				}
				else
				{
					if(m_builder->hasField(name.c_str()))
					{
						throw ex_duplicate_key_error(name);
					}
					m_builder->append(name.c_str(), *var);
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
					*var = m_objects.back()[name.c_str()].OID();
				}
				else
				{
					*var = m_obj[name.c_str()].OID();
				}
			}
		}

		SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::OID>&& var)
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

		SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::BSONObj>&& var)
		{
			if(s.IsMongoStream())
			{
				s.serialize(&var.val, var.name, var.PersistToDB);
			}
			else
			{
				if(s.IsLoading())
				{
					sprawl::String str;
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
						s % prepare_data(sprawl::String(var.val.objdata(), var.val.objsize()), var.name, var.PersistToDB);
					}
					else
					{
						s % prepare_data(var.val.jsonString(), var.name, var.PersistToDB);
					}
				}
			}
			return s;
		}

		SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::Date_t>&& var)
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
