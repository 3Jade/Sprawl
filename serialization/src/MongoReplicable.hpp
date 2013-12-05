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

#include "MongoSerializer.hpp"
#include "Replicable.hpp"

namespace sprawl
{
	namespace serialization
	{
		class MongoReplicableSerializer : public ReplicableSerializer<MongoSerializer>
		{
		public:
			std::pair<mongo::BSONObj, mongo::BSONObj> generateUpdateQuery()
			{
				mongo::BSONObjBuilder b;
				
				mongo::BSONObjBuilder changed;
				bool changed_something = false;

				std::unordered_set<std::string> arraysWithRemovedItems;
				auto get_string_key = [this, &arraysWithRemovedItems](const std::vector<int16_t>& v, bool removal)
				{
					std::stringstream str;
					bool array = false;
					bool printed = false;
					for(size_t i = 0; i < v.size(); ++i)
					{
						bool isCounter = ((i % 2) != 0);
						if(isCounter && !array)
						{
							continue;
						}
						
						if(isCounter)
						{
							array = false;
							if(removal && i == (v.size() - 1))
							{
								arraysWithRemovedItems.insert(str.str());
							}

							if(printed)
							{
								str << ".";
							}
							str << (v[i] - 1);
						}
						else
						{
							if(printed)
							{
								str << ".";
							}
							std::string& s = this->m_reverse_name_index[v[i]];
							if(s == "__array__")
							{
								array = true;
								++i;
								continue;
							}
							str << s;
							printed = true;
						}
					}
					return std::move(str.str());
				};
				
				//Adds and changes
				for( auto& kvp : this->m_data )
				{
					auto it = this->m_marked_data.find(kvp.first);
					if( it == this->m_marked_data.end() || kvp.second != it->second )
					{
						changed_something = true;
						changed.appendAs(this->m_objs[kvp.first][this->m_reverse_name_index[kvp.first[kvp.first.size()-2]]], get_string_key(kvp.first, false));
					}
				}
				if(changed_something)
				{
					b.append("$set", changed.obj());
				}

				//Removes get their own special treatment
				mongo::BSONObjBuilder removed;
				bool removed_something = false;
				for( auto& kvp : this->m_marked_data )
				{
					auto it = this->m_data.find(kvp.first);
					if( it == this->m_data.end() )
					{
						removed.append(get_string_key(kvp.first, true), "");
						removed_something = true;
					}
				}
				if(removed_something)
				{
					b.append("$unset", removed.obj());
				}
				mongo::BSONObjBuilder b2;
				if(!arraysWithRemovedItems.empty())
				{
					mongo::BSONObjBuilder pulled;
					for(auto& key : arraysWithRemovedItems)
					{
						pulled << key << mongo::BSONNULL;
					}
					b2.append("$pull", pulled.obj());
				}
				
				return std::make_pair(b.obj(), b2.obj());
			}

			mongo::BSONObj getBaselineObj()
			{
				return baseline->Obj();
			}

			mongo::BSONObj getBaselineTempObj()
			{
				return baseline->tempObj();
			}
		protected:
			template<typename T2>
			void serialize_impl( T2 *var, const std::string &name, bool PersistToDB)
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				(*m_serializer) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
				m_data[m_current_key] = m_serializer->Str();
				m_objs[m_current_key] = m_serializer->Obj();
				if(!marked)
				{
					(*baseline) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(mongo::OID* var, const std::string& name, bool PersistToDB) override
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				m_serializer->serialize(var, name, PersistToDB);
				m_data[m_current_key] = m_serializer->Str();
				m_objs[m_current_key] = m_serializer->Obj();
				if(!marked)
				{
					baseline->serialize(var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long long int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB)  override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(short int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(char *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(float *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(double *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long double *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(bool *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned long int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned long long int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned short int *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned char *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}
			virtual void serialize(std::string *var, const size_t /*bytes*/, const std::string &name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void PushKey(const std::string& name) override
			{
				if(!this->m_serializer->IsBinary() || (!this->m_current_map_key.empty() && this->m_current_key == this->m_current_map_key.back()))
				{
					if( this->m_name_index.count(name) == 0 )
					{
						this->m_reverse_name_index[this->highest_name] = name;
						this->m_name_index[name] = this->highest_name++;
					}
					this->m_current_key.push_back(this->m_name_index[name]);
				}
				else
				{
					this->m_current_key.push_back(-1);
				}
				int16_t& depth = this->m_depth_tracker[this->m_current_key];
				depth++;
				this->m_current_key.push_back( depth );
			}

			std::unordered_map<std::vector<int16_t>, mongo::BSONObj, container_hash<int16_t>> m_objs;
			std::unordered_map<int16_t, std::string> m_reverse_name_index;
		};

		class MongoReplicableDeserializer : public ReplicableDeserializer<MongoDeserializer>
		{
			virtual void serialize(mongo::OID* var, const std::string& name, bool PersistToDB) override
			{
				m_serializer->Reset();
				PushKey(name);

				MongoSerializer serializer;
				serializer % m_current_key;

				//Do nothing if this element hasn't changed.
				auto it = m_diffs.find(m_current_key);
				if(it != m_diffs.end())
				{
					m_serializer->Data(it->second);
					m_serializer->serialize(var, name, PersistToDB);
					m_diffs.erase(it);
				}
				PopKey();
			}
		};
			
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
