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

				struct ArrayIndexInfo
				{
					ArrayIndexInfo()
						: parentIndex(nullptr)
						, key()
						, parentKey()
						, hasValidChildren(false)
						, hasRemovedChildren(false)
						, removeQuery()
						, children()
					{
						//
					}

					void SetRemovedChildren()
					{
						hasRemovedChildren = true;
						if(parentIndex)
						{
							parentIndex->SetRemovedChildren();
						}
					}

					void SetValidChildren()
					{
						hasValidChildren = true;
						if(parentIndex)
						{
							parentIndex->SetValidChildren();
						}
					}

					ArrayIndexInfo* parentIndex;
					std::string key;
					std::string parentKey;
					bool hasValidChildren;
					bool hasRemovedChildren;
					mongo::BSONObjBuilder removeQuery;
					std::unordered_set<ArrayIndexInfo*> children;
				};

				ArrayIndexInfo* currentArrayIndex = nullptr;
				std::list<ArrayIndexInfo> allArrayIndexes;
				std::unordered_map<std::string, ArrayIndexInfo*> allArrayIndexesByKey;
				std::unordered_set<ArrayIndexInfo*> rootIndexes;

				auto get_string_key = [
					this,
					&currentArrayIndex,
					&allArrayIndexes,
					&allArrayIndexesByKey,
					&rootIndexes
				] (
					const std::vector<int16_t>& v,
					bool removal
				)

				{
					currentArrayIndex = nullptr;
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

							std::string parentKey = str.str();
							if(printed)
							{
								str << ".";
								printed = false;
							}
							str << (v[i] - 1);

							ArrayIndexInfo* parentIndex = currentArrayIndex;
							std::string key = str.str();
							auto it = allArrayIndexesByKey.find(key);
							if(it != allArrayIndexesByKey.end())
							{
								currentArrayIndex = it->second;
							}
							else
							{
								allArrayIndexes.emplace_back();
								currentArrayIndex = &allArrayIndexes.back();
								currentArrayIndex->key = key;
								currentArrayIndex->parentKey = parentKey;
								allArrayIndexesByKey.insert(std::make_pair(key, currentArrayIndex));
							}

							if(!parentIndex)
							{
								rootIndexes.insert(currentArrayIndex);
							}
							else
							{
								currentArrayIndex->parentIndex = parentIndex;
								parentIndex->children.insert(currentArrayIndex);
							}

							if(removal)
							{
								currentArrayIndex->SetRemovedChildren();
							}
							else
							{
								currentArrayIndex->SetValidChildren();
							}

							printed = true;
						}
						else
						{
							if(printed)
							{
								str << ".";
								printed = false;
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

				mongo::BSONObjBuilder changed;
				bool changed_something = false;

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
				bool removed_from_array = false;

				bool removed_something = false;
				for( auto& kvp : this->m_marked_data )
				{
					auto it = this->m_data.find(kvp.first);
					if( it != this->m_data.end() )
					{
						//Not what this says it does, but it will implicitly mark children valid.
						get_string_key(kvp.first, false).c_str();
					}
				}
				for( auto& kvp : this->m_marked_data )
				{
					auto it = this->m_data.find(kvp.first);
					if( it == this->m_data.end() )
					{
						std::string key = get_string_key(kvp.first, true);
						if(currentArrayIndex)
						{
							currentArrayIndex->removeQuery.append(key, "");
							removed_from_array = true;
						}
						else
						{
							removed.append(key, "");
						}
						removed_something = true;
					}
				}

				struct
				{
					mongo::BSONObj operator()(ArrayIndexInfo* index)
					{
						if(index->hasValidChildren)
						{
							for(ArrayIndexInfo* child : index->children)
							{
								index->removeQuery.appendElements((*this)(child));
							}
							return index->removeQuery.obj();
						}
						else
						{
							return BSON(index->key << "");
						}
					}
				} getRemoveQuery;

				for(ArrayIndexInfo* index : rootIndexes)
				{
					if(index->hasRemovedChildren)
					{
						removed.appendElements(getRemoveQuery(index));
					}
				}

				if(removed_something)
				{
					b.append("$unset", removed.obj());
				}

				mongo::BSONObjBuilder b2;

				struct
				{
					mongo::BSONObj operator()(ArrayIndexInfo* index)
					{
						if(index->hasValidChildren)
						{
							mongo::BSONObjBuilder removedArrayIndexes;
							for(ArrayIndexInfo* child : index->children)
							{
								removedArrayIndexes.appendElements((*this)(child));
							}
							return removedArrayIndexes.obj();
						}
						else
						{
							return BSON(index->parentKey << mongo::BSONNULL);
						}
					}
				} getCorrectRemoveIndex;

				if(removed_from_array)
				{
					mongo::BSONObjBuilder pulled;
					for(ArrayIndexInfo* index : rootIndexes)
					{
						if(index->hasRemovedChildren)
						{
							pulled.appendElements(getCorrectRemoveIndex(index));
						}
					}
					b2.append("$pull", pulled.obj());
				}
				
				return std::make_pair(b.obj(), b2.obj());
			}

			mongo::BSONObj getBaselineObj()
			{
				return m_baseline->Obj();
			}

			mongo::BSONObj getBaselineTempObj()
			{
				return m_baseline->tempObj();
			}
		protected:
			template<typename T2>
			void serialize_impl( T2* var, const std::string& name, bool PersistToDB)
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				(*m_serializer) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
				m_data[m_current_key] = m_serializer->Str();
				m_objs[m_current_key] = m_serializer->Obj();
				if(!m_marked)
				{
					(*m_baseline) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
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
				if(!m_marked)
				{
					m_baseline->serialize(var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(mongo::BSONObj* var, const std::string& name, bool PersistToDB) override
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				m_serializer->serialize(var, name, PersistToDB);
				m_data[m_current_key] = m_serializer->Str();
				m_objs[m_current_key] = m_serializer->Obj();
				if(!m_marked)
				{
					m_baseline->serialize(var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(mongo::Date_t* var, const std::string& name, bool PersistToDB) override
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				m_serializer->serialize(var, name, PersistToDB);
				m_data[m_current_key] = m_serializer->Str();
				m_objs[m_current_key] = m_serializer->Obj();
				if(!m_marked)
				{
					m_baseline->serialize(var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long long int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB)  override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(short int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(char* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(float* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(double* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long double* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(bool* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned long int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned long long int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned short int* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned char* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}
			virtual void serialize(std::string* var, const uint32_t /*bytes*/, const std::string& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void PushKey(const std::string& name) override
			{
				if(!this->m_serializer->IsBinary() || (!this->m_current_map_key.empty() && this->m_current_key == this->m_current_map_key.back()))
				{
					if( this->m_name_index.count(name) == 0 )
					{
						this->m_reverse_name_index[this->m_highest_name] = name;
						this->m_name_index[name] = this->m_highest_name++;
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

			MongoReplicableDeserializer(const std::string& data)
				: ReplicableDeserializer<MongoDeserializer>(data)
			{
				//
			}

			MongoReplicableDeserializer(const char* data, size_t length)
				: ReplicableDeserializer<MongoDeserializer>(data, length)
			{
				//
			}
		};
			
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
