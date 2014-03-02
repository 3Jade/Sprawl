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
			virtual void StartArray(const std::string &name, uint32_t &size, bool b) override
			{
				ReplicableSerializer<MongoSerializer>::StartArray(name, size, b);
				m_allArrays.insert(m_current_key);
			}

			virtual uint32_t StartObject(const std::string &name, bool b)
			{
				uint32_t ret = ReplicableSerializer<MongoSerializer>::StartObject(name, b);
				m_allObjs.insert(m_current_key);
				return ret;
			}

			virtual uint32_t StartMap(const std::string &name, bool b)
			{
				uint32_t ret = ReplicableSerializer<MongoSerializer>::StartMap(name, b);
				m_allObjs.insert(m_current_key);
				return ret;
			}

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
					int currentArray = 0;
					for(size_t i = 0; i < v.size(); ++i)
					{
						bool isCounter = ((i % 2) != 0);
						if(isCounter && !array)
						{
							continue;
						}

						auto handleArrays = [
							&allArrayIndexes,
							&allArrayIndexesByKey,
							&currentArrayIndex,
							&rootIndexes
						](const std::string& key, const std::string& parentKey)
						{
							ArrayIndexInfo* parentIndex = currentArrayIndex;
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
						};
						
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

							handleArrays(str.str(), parentKey);

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
							int key = v[i];
							if(key < 0)
							{
								key = -key;
								if(currentArray != key)
								{
									if(printed)
									{
										str << ".";
										printed = false;
									}
									currentArray = key;
									std::string& s = this->m_reverse_name_index[key];
									str << s;

									printed = true;
									++i;
								}
								array = true;
								continue;
							}

							std::string parentKey = str.str();

							if(printed)
							{
								str << ".";
								printed = false;
							}

							if(currentArray != 0)
							{
								++i;
								str << (v[i] - 1);
								currentArray = 0;

								handleArrays(str.str(), parentKey);

								if(removal)
								{
									currentArrayIndex->SetRemovedChildren();
								}
								else
								{
									currentArrayIndex->SetValidChildren();
								}
								array = false;
							}
							else
							{
								std::string& s = this->m_reverse_name_index[key];
								str << s;
							}
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

				for( auto& key : this->m_allArrays )
				{
					if(!this->m_markedArrays.count(key))
					{
						changed_something = true;
						changed.append( get_string_key(key, false), mongo::BSONArray() );
					}
				}

				for( auto& key : this->m_allObjs )
				{
					if(!this->m_markedObjs.count(key))
					{
						changed_something = true;
						changed.append( get_string_key(key, false), mongo::BSONObj() );
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

				for( auto& key : this->m_allArrays )
				{
					//Not what this says it does, but it will implicitly mark children valid.
					get_string_key(key, false);
				}
				for( auto& key : this->m_allObjs )
				{
					//Not what this says it does, but it will implicitly mark children valid.
					get_string_key(key, false);
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

				for( auto& key : this->m_markedArrays )
				{
					if(!this->m_allArrays.count(key))
					{
						std::string keyStr = get_string_key(key, true);
						if(currentArrayIndex)
						{
							currentArrayIndex->removeQuery.append(keyStr, "");
							removed_from_array = true;
						}
						else
						{
							removed.append(keyStr, "");
						}
						removed_something = true;
					}
				}

				for( auto& key : this->m_markedObjs )
				{
					if(!this->m_allObjs.count(key))
					{
						std::string keyStr = get_string_key(key, true);
						if(currentArrayIndex)
						{
							currentArrayIndex->removeQuery.append(keyStr, "");
							removed_from_array = true;
						}
						else
						{
							removed.append(keyStr, "");
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
								index->removeQuery.appendElementsUnique((*this)(child));
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
						removed.appendElementsUnique(getRemoveQuery(index));
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
								removedArrayIndexes.appendElementsUnique((*this)(child));
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
							pulled.appendElementsUnique(getCorrectRemoveIndex(index));
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

			virtual void Mark()
			{
				ReplicableSerializer<MongoSerializer>::Mark();
				m_markedArrays = std::move( m_allArrays );
				m_markedObjs = std::move( m_allObjs );
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

			virtual void PushKey(const std::string& name, bool forArray = false) override
			{
				bool parentIsArray = false;
				if(!m_current_key.empty() && m_current_key[m_current_key.size() - 2] < 0)
				{
					parentIsArray = true;
				}
				if(!this->m_serializer->IsBinary() || (!this->m_current_map_key.empty() && this->m_current_key == this->m_current_map_key.back()))
				{
					if( this->m_name_index.count(name) == 0 )
					{
						this->m_reverse_name_index[this->m_highest_name] = name;
						this->m_name_index[name] = this->m_highest_name++;
					}
					int key = this->m_name_index[name];
					if(forArray)
					{
						//Use sign bit to encode this as an array member.
						key = -key;
					}
					this->m_current_key.push_back(key);
				}
				else
				{
					this->m_current_key.push_back(-1);
				}
				if(parentIsArray)
				{
					int16_t& item = m_array_tracker.back();
					++item;
					m_current_key.push_back( item );
				}
				else
				{
					int16_t& depth = this->m_depth_tracker[this->m_current_key];
					depth++;
					this->m_current_key.push_back( depth );
				}
				if(forArray)
				{
					this->m_array_tracker.push_back(0);
				}
			}

			virtual void PopKey() override
			{
				if( m_current_key[m_current_key.size() - 2] < 0 )
				{
					this->m_array_tracker.pop_back();
				}
				ReplicableSerializer<MongoSerializer>::PopKey();
			}

			std::unordered_map<std::vector<int16_t>, mongo::BSONObj, container_hash<int16_t>> m_objs;
			std::unordered_map<int16_t, std::string> m_reverse_name_index;
			std::unordered_set<std::vector<int16_t>, container_hash<int16_t>> m_allArrays;
			std::unordered_set<std::vector<int16_t>, container_hash<int16_t>> m_allObjs;
			std::unordered_set<std::vector<int16_t>, container_hash<int16_t>> m_markedArrays;
			std::unordered_set<std::vector<int16_t>, container_hash<int16_t>> m_markedObjs;
			std::vector<int16_t> m_array_tracker;
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
