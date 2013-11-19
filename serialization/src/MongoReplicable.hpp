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
 
#include "MongoSerializer.hpp"
#include "Replicable.hpp"

namespace sprawl
{
	namespace serialization
	{
		template<typename T>
		class MongoReplicableBase : virtual public ReplicableBase<T>
		{
		protected:
			template<typename T2>
			void serialize_impl( T2 *var, const std::string &name, bool PersistToDB)
			{
				this->m_serializer->Reset();
				this->PushKey(name);
				if(this->IsLoading())
				{
					typename T::serializer_type serializer;
					serializer % this->m_current_key;

					//Do nothing if this element hasn't changed.
					auto it = this->m_diffs.find(this->m_current_key);
					if(it != this->m_diffs.end())
					{
						this->m_serializer->Data(it->second);
						(*this->m_serializer) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
						this->m_diffs.erase(it);
					}
				}
				else
				{
					(*this->m_serializer) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
					this->m_data[this->m_current_key] = this->m_serializer->Str();
					this->m_objs[this->m_current_key] = this->m_serializer->Obj();
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
		
		class MongoReplicableSerializer : public ReplicableSerializer<MongoSerializer>, public MongoReplicableBase<MongoSerializer>
		{
		public:
			mongo::BSONObj generateUpdateQuery()
			{
				mongo::BSONObjBuilder b;
				
				auto get_string_key = [this](const std::vector<int16_t>& v)
				{
					std::string str;
					for(int i = 2; i < v.size(); i += 2)
					{
						if(i != 2)
						{
							str += ".";
						}
						str += this->m_reverse_name_index[v[i]];
					}
					return std::move(str);
				};
				
				//Adds and changes
				for( auto& kvp : this->m_data )
				{
					auto it = this->m_marked_data.find(kvp.first);
					if( it == this->m_marked_data.end() || kvp.second != it->second )
					{
						b.appendAs(this->m_objs[kvp.first][this->m_reverse_name_index[kvp.first[kvp.first.size()-2]]], get_string_key(kvp.first));
					}
				}

				//Removes get their own special treatment
				mongo::BSONObjBuilder removed;
				bool removed_something = false;
				for( auto& kvp : this->m_marked_data )
				{
					auto it = this->m_data.find(kvp.first);
					if( it == this->m_data.end() )
					{
						removed.append(get_string_key(kvp.first), "");
						removed_something = true;
					}
				}
				if(removed_something)
				{
					b.append("$unset", removed.obj());
				}
				
				return b.obj();
			}
		};
		
		typedef ReplicableDeserializer<MongoDeserializer> MongoReplicableDeserializer;
			
	}
}
