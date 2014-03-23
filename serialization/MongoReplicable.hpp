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
			typedef std::unordered_set<ReplicationKey, RKeyHash, std::equal_to<ReplicationKey>, sprawl::memory::StlWrapper<ReplicationKey>> ReplicationSet;
			typedef std::unordered_map<ReplicationKey, mongo::BSONObj, RKeyHash, std::equal_to<ReplicationKey>, sprawl::memory::StlWrapper<std::pair<ReplicationKey, mongo::BSONObj>>> ReplicationBSONMap;
			virtual void StartArray(const sprawl::String &name, uint32_t &size, bool b) override
			{
				ReplicableSerializer<MongoSerializer>::StartArray(name, size, b);
				m_allArrays.insert(m_current_key);
			}

			virtual uint32_t StartObject(const sprawl::String &name, bool b) override
			{
				uint32_t ret = ReplicableSerializer<MongoSerializer>::StartObject(name, b);
				m_allObjs.insert(m_current_key);
				return ret;
			}

			virtual uint32_t StartMap(const sprawl::String &name, bool b) override
			{
				uint32_t ret = ReplicableSerializer<MongoSerializer>::StartMap(name, b);
				m_allObjs.insert(m_current_key);
				return ret;
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
				m_markedObjData = std::move( m_objData );
				m_array_tracker.clear();
			}

			virtual void Reset()
			{
				ReplicableSerializer<MongoSerializer>::Reset();
				m_allArrays.clear();
				m_markedArrays.clear();
				m_allObjs.clear();
				m_markedObjs.clear();
				m_objData.clear();
				m_markedObjData.clear();
				m_array_tracker.clear();
			}

			std::pair<mongo::BSONObj, mongo::BSONObj> generateUpdateQuery()
			{
				BuildDeltaParams params = { m_objData, m_data, m_marked_data, m_allArrays, m_markedArrays, m_allObjs, m_markedObjs };
				return this->BuildDelta( params );
			}

			std::pair<mongo::BSONObj, mongo::BSONObj> generateUndoQuery()
			{
				BuildDeltaParams params = {m_markedObjData, m_marked_data, m_data, m_markedArrays, m_allArrays, m_markedObjs, m_allObjs};
				return this->BuildDelta( params );
			}

		protected:
			template<typename T2>
			void serialize_impl( T2* var, const sprawl::String& name, bool PersistToDB)
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				(*m_serializer) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
				m_data.insert(std::make_pair(m_current_key, m_serializer->Str()));
				m_objData.insert(std::make_pair(m_current_key, m_serializer->Obj()));
				if(!m_marked)
				{
					(*m_baseline) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
				}

				this->PopKey();
			}

		public:
			virtual void serialize(mongo::OID* var, const sprawl::String& name, bool PersistToDB) override
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				m_serializer->serialize(var, name, PersistToDB);
				m_data.insert(std::make_pair(m_current_key, m_serializer->Str()));
				m_objData.insert(std::make_pair(m_current_key, m_serializer->Obj()));
				if(!m_marked)
				{
					m_baseline->serialize(var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(mongo::BSONObj* var, const sprawl::String& name, bool PersistToDB) override
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				m_serializer->serialize(var, name, PersistToDB);
				m_data.insert(std::make_pair(m_current_key, m_serializer->Str()));
				m_objData.insert(std::make_pair(m_current_key, m_serializer->Obj()));
				if(!m_marked)
				{
					m_baseline->serialize(var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(mongo::Date_t* var, const sprawl::String& name, bool PersistToDB) override
			{
				this->m_serializer->Reset();
				this->PushKey(name);

				m_serializer->serialize(var, name, PersistToDB);
				m_data.insert(std::make_pair(m_current_key, m_serializer->Str()));
				m_objData.insert(std::make_pair(m_current_key, m_serializer->Obj()));
				if(!m_marked)
				{
					m_baseline->serialize(var, name, PersistToDB);
				}

				this->PopKey();
			}

			virtual void serialize(int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long long int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB)  override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(short int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(char* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(float* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(double* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(long double* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(bool* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned long int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned long long int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned short int* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

			virtual void serialize(unsigned char* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}
			virtual void serialize(std::string* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}
			virtual void serialize(sprawl::String* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override
			{
				serialize_impl(var, name, PersistToDB);
			}

		private:
			virtual void PushKey(const sprawl::String& name, bool forArray = false) override
			{
				bool parentIsArray = false;
				if(!m_current_key.empty() && m_current_key[m_current_key.size() - 2] < 0)
				{
					parentIsArray = true;
				}

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

				if(parentIsArray)
				{
					int32_t& item = m_array_tracker.back();
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

			struct BuildDeltaParams
			{
				const ReplicationBSONMap& objs;
				const ReplicationMap& data;
				const ReplicationMap& markedData;
				const ReplicationSet& allArrays;
				const ReplicationSet& markedArrays;
				const ReplicationSet& allObjs;
				const ReplicationSet& markedObjs;
			};

			std::pair<mongo::BSONObj, mongo::BSONObj> BuildDelta(const BuildDeltaParams& params)
			{
				const ReplicationBSONMap& objs = params.objs;
				const ReplicationMap& data = params.data;
				const ReplicationMap& markedData = params.markedData;
				const ReplicationSet& allArrays = params.allArrays;
				const ReplicationSet& markedArrays = params.markedArrays;
				const ReplicationSet& allObjs = params.allObjs;
				const ReplicationSet& markedObjs = params.markedObjs;

				mongo::BSONObjBuilder b;

				struct ObjectData
				{
					ObjectData()
						: parentObject(nullptr)
						, children()
						, data()
						, shortKey()
						, fullKey()
						, intKey()
						, isArray(false)
						, isNew(true)
						, hasValidChildren(false)
						, hasRemovedChildren(false)
						, removeQuery()
					{
						//
					}

					void MarkNotNew()
					{
						isNew = false;
						if(parentObject)
						{
							parentObject->MarkNotNew();
						}
					}

					void SetRemovedChildren()
					{
						hasRemovedChildren = true;
						if(parentObject)
						{
							parentObject->SetRemovedChildren();
						}
					}

					void SetValidChildren()
					{
						hasValidChildren = true;
						if(parentObject)
						{
							parentObject->SetValidChildren();
						}
					}

					mongo::BSONElement BuildData()
					{
						if(children.empty())
						{
							return data;
						}

						if(isArray)
						{
							mongo::BSONArrayBuilder arrayBuilder;
							for(auto& kvp : children)
							{
								arrayBuilder.append(kvp.second->BuildData());
							}
							mongo::BSONObjBuilder objBuilder;
							objBuilder.append("firstElem", arrayBuilder.arr());
							objData = objBuilder.obj();
						}
						else
						{
							mongo::BSONObjBuilder childObjsBuilder;
							for(auto& kvp : children)
							{
								childObjsBuilder.appendAs(kvp.second->BuildData(), kvp.second->shortKey.c_str());
							}
							mongo::BSONObjBuilder objBuilder;
							objBuilder.append("firstElem", childObjsBuilder.obj());
							objData = objBuilder.obj();
						}
						return objData.firstElement();
					}

					ObjectData* parentObject;
					std::map<ReplicationKey, ObjectData*, std::less<ReplicationKey>, sprawl::memory::StlWrapper<std::pair<ReplicationKey, ObjectData*>>> children;
					mongo::BSONElement data;
					mongo::BSONObj objData;
					sprawl::String shortKey;

					///TODO: sprawl::String here
					std::string fullKey;
					ReplicationKey intKey;
					bool isArray;
					bool isNew;
					bool hasValidChildren;
					bool hasRemovedChildren;
					mongo::BSONObjBuilder removeQuery;
				};

				std::list<ObjectData, sprawl::memory::StlWrapper<ObjectData>> allObjects;
				std::map<ReplicationKey, ObjectData*, std::less<ReplicationKey>, sprawl::memory::StlWrapper<std::pair<ReplicationKey, ObjectData*>>> allKeys;
				ObjectData* currentObject;
				std::set<ObjectData*, std::less<ObjectData*>, sprawl::memory::StlWrapper<ObjectData*>> rootObjects;

				auto buildObjectData = [
					this,
					&allKeys,
					&allObjects,
					&currentObject,
					&rootObjects
				] (
					const ReplicationKey& keyToBuild,
					bool newObj,
					bool removal
				)

				{
					ReplicationKey currentKey;
					currentObject = nullptr;
					uint32_t size = keyToBuild.size();
					const int32_t* cArray = &keyToBuild[0];

					for(uint32_t i = 1; i < size; i += 2)
					{
						currentKey.push_back(cArray[i - 1]);
						currentKey.push_back(cArray[i]);

						ObjectData* parentObject = currentObject;
						auto it = allKeys.find(currentKey);
						if(it != allKeys.end())
						{
							currentObject = it->second;
						}
						else
						{
							allObjects.emplace_back();
							currentObject = &allObjects.back();

							int key = currentKey[currentKey.size() - 2];
							if(key < 0)
							{
								key = -key;
								currentObject->isArray = true;
							}

							if(parentObject)
							{
								if(parentObject->isArray)
								{
									char buf[16];
#ifdef _WIN32
									_snprintf( buf, 16, "%d", currentKey.back() - 1 );
#else
									snprintf( buf, 16, "%d", currentKey.back() - 1 );
#endif
									currentObject->shortKey = buf;
								}
								else
								{
									currentObject->shortKey = this->m_reverse_name_index[key];
								}

								currentObject->fullKey = parentObject->fullKey + "." + currentObject->shortKey.toStdString();

								parentObject->children.insert(std::make_pair(currentKey, currentObject));
							}
							else
							{
								currentObject->shortKey = this->m_reverse_name_index[key];
								currentObject->fullKey = currentObject->shortKey.toStdString();
								rootObjects.insert(currentObject);
							}

							currentObject->parentObject = parentObject;
							currentObject->intKey = currentKey;

							allKeys.insert(std::make_pair(currentKey, currentObject));
						}

						if(!newObj)
						{
							currentObject->isNew = false;
						}

						if(removal)
						{
							currentObject->SetRemovedChildren();
						}
						else
						{
							currentObject->SetValidChildren();
						}

					}
				};

				bool changed_something = false;

				//Adds and changes
				for( auto& kvp : data )
				{
					auto it = markedData.find(kvp.first);
					bool isNew = ( it == markedData.end() );
					if( isNew || kvp.second != it->second )
					{
						changed_something = true;
						mongo::BSONElement obj = objs.at(kvp.first)[this->m_reverse_name_index[kvp.first[kvp.first.size()-2]].c_str()];
						buildObjectData(kvp.first, isNew, false);
						if(currentObject)
						{
							currentObject->data = obj;
						}
					}
				}

				for( auto& key : allArrays )
				{
					if(!markedArrays.count(key))
					{
						if(!allKeys.count(key))
						{
							buildObjectData(key, true, false);
							if(currentObject)
							{
								mongo::BSONObjBuilder b;
								b.append("firstElement", mongo::BSONArray());
								currentObject->objData = b.obj();
								currentObject->data = currentObject->objData.firstElement();
							}
							changed_something = true;
						}
					}
					else
					{
						auto kvp = allKeys.find(key);
						if(kvp != allKeys.end())
						{
							kvp->second->MarkNotNew();
						}
					}
				}

				for( auto& key : allObjs )
				{
					if(!markedObjs.count(key))
					{
						if(!allKeys.count(key))
						{
							changed_something = true;
							buildObjectData(key, true, false);
							if(currentObject)
							{
								mongo::BSONObjBuilder b;
								b.append("firstElement", mongo::BSONObj());
								currentObject->objData = b.obj();
								currentObject->data = currentObject->objData.firstElement();
							}
						}
					}
					else
					{
						auto kvp = allKeys.find(key);
						if(kvp != allKeys.end())
						{
							kvp->second->MarkNotNew();
						}
					}
				}

				struct
				{
					void operator()(mongo::BSONObjBuilder& b, ObjectData* obj)
					{
						if(obj->isNew || obj->children.empty())
						{
							b.appendAs(obj->BuildData(), obj->fullKey);
						}
						else
						{
							for(auto& kvp : obj->children)
							{
								(*this)(b, kvp.second);
							}
						}
					}
				} getAddQuery;

				if(changed_something)
				{
					mongo::BSONObjBuilder changed;
					for(auto obj : rootObjects)
					{
						getAddQuery(changed, obj);
					}
					b.append("$set", changed.obj());
				}

				//Removes get their own special treatment
				mongo::BSONObjBuilder removed;
				bool removed_something = false;

				for( auto& key : allArrays )
				{
					if(!allKeys.count(key))
					{
						buildObjectData(key, false, false);
					}
				}
				for( auto& key : allObjs )
				{
					if(!allKeys.count(key))
					{
						buildObjectData(key, false, false);
					}
				}

				for( auto& kvp : markedData )
				{
					auto it = data.find(kvp.first);
					if( it == data.end() )
					{
						buildObjectData(kvp.first, false, true);
						if(currentObject)
						{
							currentObject->removeQuery.append(currentObject->fullKey, "");
						}
						removed_something = true;
					}
				}

				for( auto& key : markedArrays )
				{
					if(!allArrays.count(key))
					{
						buildObjectData(key, false, true);
						if(currentObject)
						{
							currentObject->removeQuery.append(currentObject->fullKey, "");
						}
						removed_something = true;
					}
				}

				for( auto& key : markedObjs )
				{
					if(!allObjs.count(key))
					{
						buildObjectData(key, false, true);
						if(currentObject)
						{
							currentObject->removeQuery.append(currentObject->fullKey, "");
						}
						removed_something = true;
					}
				}

				struct
				{
					mongo::BSONObj operator()(ObjectData* obj)
					{
						if(obj->hasValidChildren)
						{
							for(auto& kvp : obj->children)
							{
								obj->removeQuery.appendElementsUnique((*this)(kvp.second));
							}
							return obj->removeQuery.obj();
						}
						else
						{
							return BSON(obj->fullKey << "");
						}
					}
				} getRemoveQuery;

				if(removed_something)
				{
					for(ObjectData* obj : rootObjects)
					{
						if(obj->hasRemovedChildren)
						{
							removed.appendElementsUnique(getRemoveQuery(obj));
						}
					}

					b.append("$unset", removed.obj());
				}

				mongo::BSONObjBuilder b2;

				struct
				{
					mongo::BSONObj operator()(ObjectData* obj, bool& pulled_something)
					{
						if(obj->hasValidChildren)
						{
							mongo::BSONObjBuilder removedArrayIndexes;
							for(auto& kvp : obj->children)
							{
								removedArrayIndexes.appendElementsUnique((*this)(kvp.second, pulled_something));
							}
							return removedArrayIndexes.obj();
						}
						else
						{
							if(obj->parentObject && obj->parentObject->isArray)
							{
								pulled_something = true;
								return BSON(obj->parentObject->fullKey << mongo::BSONNULL);
							}
							else
							{
								return mongo::BSONObj();
							}
						}
					}
				} getCorrectRemoveIndex;

				if(removed_something)
				{
					bool pulled_something = false;
					mongo::BSONObjBuilder pulled;
					for(ObjectData* obj : rootObjects)
					{
						if(obj->hasRemovedChildren)
						{
							pulled.appendElementsUnique(getCorrectRemoveIndex(obj, pulled_something));
						}
					}
					if(pulled_something)
					{
						b2.append("$pull", pulled.obj());
					}
				}

				return std::make_pair(b.obj(), b2.obj());
			}

			ReplicationBSONMap m_objData;
			ReplicationBSONMap m_markedObjData;
			KeyToStringMap m_reverse_name_index;
			ReplicationSet m_allArrays;
			ReplicationSet m_allObjs;
			ReplicationSet m_markedArrays;
			ReplicationSet m_markedObjs;
			ReplicationKey m_array_tracker;
		};

		class MongoReplicableDeserializer : public ReplicableDeserializer<MongoDeserializer>
		{
		public:
			using ReplicableDeserializer<MongoDeserializer>::serialize;

			virtual void serialize(mongo::OID* var, const sprawl::String& name, bool PersistToDB) override
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

			MongoReplicableDeserializer(const sprawl::String& data)
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
