#include "MongoReplicable.hpp"

namespace sprawl
{
	namespace serialization
	{
		void MongoReplicableSerializer::StartArray(String const& name, uint32_t& size, bool b)
		{
			ReplicableSerializer<MongoSerializer>::StartArray(name, size, b);
			m_allArrays.insert(m_current_key);
		}

		uint32_t MongoReplicableSerializer::StartObject(String const& name, bool b)
		{
			uint32_t ret = ReplicableSerializer<MongoSerializer>::StartObject(name, b);
			m_allObjs.insert(m_current_key);
			return ret;
		}

		uint32_t MongoReplicableSerializer::StartMap(String const& name, bool b)
		{
			uint32_t ret = ReplicableSerializer<MongoSerializer>::StartMap(name, b);
			m_allObjs.insert(m_current_key);
			return ret;
		}

		void MongoReplicableSerializer::Mark()
		{
			ReplicableSerializer<MongoSerializer>::Mark();
			m_markedArrays = std::move( m_allArrays );
			m_markedObjs = std::move( m_allObjs );
			m_markedObjData = std::move( m_objData );
			m_array_tracker.clear();
		}

		void MongoReplicableSerializer::Discard()
		{
			ReplicableSerializer<MongoSerializer>::Discard();
			m_allArrays.clear();
			m_allObjs.clear();
			m_objData.clear();
			m_array_tracker.clear();
		}

		void MongoReplicableSerializer::Reset()
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

		void MongoReplicableSerializer::PushKey(String const& name, bool forArray)
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

		void MongoReplicableSerializer::PopKey()
		{
			if( m_current_key[m_current_key.size() - 2] < 0 )
			{
				this->m_array_tracker.pop_back();
			}
			ReplicableSerializer<MongoSerializer>::PopKey();
		}

		std::vector<mongo::BSONObj> MongoReplicableSerializer::BuildDelta(MongoReplicableSerializer::BuildDeltaParams const& params)
		{
			ReplicationBSONMap const& objs = params.objs;
			ReplicationMap const& data = params.data;
			ReplicationMap const& markedData = params.markedData;
			ReplicationSet const& allArrays = params.allArrays;
			ReplicationSet const& markedArrays = params.markedArrays;
			ReplicationSet const& allObjs = params.allObjs;
			ReplicationSet const& markedObjs = params.markedObjs;

			mongo::BSONObjBuilder updateQuery;

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
				sprawl::String fullKey;
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
			typedef std::set<ObjectData*, std::less<ObjectData*>, sprawl::memory::StlWrapper<ObjectData*>> ObjectSet;
			ObjectSet rootObjects;

			auto buildObjectData = [
					this,
					&allKeys,
					&allObjects,
					&currentObject,
					&rootObjects
					] (
					ReplicationKey const& keyToBuild,
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

							currentObject->fullKey = parentObject->fullKey + "." + currentObject->shortKey;

							parentObject->children.insert(std::make_pair(currentKey, currentObject));
						}
						else
						{
							currentObject->shortKey = this->m_reverse_name_index[key];
							currentObject->fullKey = currentObject->shortKey;
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
						b.appendAs(obj->BuildData(), obj->fullKey.c_str());
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
				updateQuery.append("$set", changed.obj());
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
						currentObject->removeQuery.append(currentObject->fullKey.c_str(), "");
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
						currentObject->removeQuery.append(currentObject->fullKey.c_str(), "");
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
						currentObject->removeQuery.append(currentObject->fullKey.c_str(), "");
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
						return BSON(obj->fullKey.c_str() << "");
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

				updateQuery.append("$unset", removed.obj());
			}

			std::vector<mongo::BSONObj> ret;

			if(changed_something || removed_something)
			{
				ret.push_back(updateQuery.obj());
			}

			//If array members have been removed, we have to generate an arbitrary number of additional queries.
			//This is one of the most frustrating aspects of Mongo: You can't directly remove an element from an array;
			//you have to change it to a value that you know doesn't exist anywhere else in the array, then instruct
			//mongo to remove all elements of that value from the array. Very backward.

			//Worse than that, if you have nested arrays (say, [ [ 1, 2, 3 ], [ 4, 5, 6 ] ]) and you remove an item
			//from the inner array and also remove an item from the outer array (say, changing it to [ [ 1, 2 ] ]
			//mongo won't let you perform both removals at the same time. They have to be executed in separate steps.

			//That means that we have to generate N additional queries to mongo to clean up these shrunk arrays. Blech.

			struct
			{
				bool ArrayAlreadyAccountedFor(ObjectData* obj)
				{
					while(obj)
					{
						if(obj->isArray && objectsAccountedFor.count(obj))
						{
							return true;
						}
						obj = obj->parentObject;
					}
					return false;
				}

				void AddAccountedArray(ObjectData* obj)
				{
					while(obj)
					{
						if(obj->isArray)
						{
							objectsAccountedFor.insert(obj);
						}
						obj = obj->parentObject;
					}
				}

				mongo::BSONObj operator()(ObjectData* obj)
				{
					if(obj->hasValidChildren)
					{
						mongo::BSONObjBuilder removedArrayIndexes;
						for(auto& kvp : obj->children)
						{
							removedArrayIndexes.appendElementsUnique((*this)(kvp.second));
						}
						return removedArrayIndexes.obj();
					}
					else
					{
						if(obj->parentObject && obj->parentObject->isArray)
						{
							if(objectsAlreadyPulled.count(obj->parentObject))
							{
								return mongo::BSONObj();
							}
							if(ArrayAlreadyAccountedFor(obj->parentObject))
							{
								newRootObjects.insert(obj->parentObject);
								return mongo::BSONObj();
							}
							pulled_something = true;
							AddAccountedArray(obj->parentObject);
							objectsAlreadyPulled.insert(obj->parentObject);
							return BSON(obj->parentObject->fullKey.c_str() << mongo::BSONNULL);
						}
						else
						{
							return mongo::BSONObj();
						}
					}
				}

				void Reset()
				{
					newRootObjects.clear();
					objectsAccountedFor.clear();
					pulled_something = false;
				}

				ObjectSet newRootObjects;
				ObjectSet objectsAccountedFor;
				ObjectSet objectsAlreadyPulled;
				bool pulled_something;
			} getCorrectRemoveIndex;

			if(removed_something)
			{
				while(!rootObjects.empty())
				{
					mongo::BSONObjBuilder pullQuery;
					mongo::BSONObjBuilder pulled;
					getCorrectRemoveIndex.Reset();
					for(ObjectData* obj : rootObjects)
					{
						if(obj->hasRemovedChildren)
						{
							pulled.appendElementsUnique(getCorrectRemoveIndex(obj));
						}
					}
					if(getCorrectRemoveIndex.pulled_something)
					{
						pullQuery.append("$pull", pulled.obj());
						ret.push_back(pullQuery.obj());
					}
					rootObjects = std::move(getCorrectRemoveIndex.newRootObjects);
				}
			}

			return std::move(ret);
		}

		void MongoReplicableSerializer::serialize(mongo::Date_t* var, String const& name, bool PersistToDB)
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

		void MongoReplicableSerializer::serialize(mongo::BSONObj* var, String const& name, bool PersistToDB)
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

		void MongoReplicableSerializer::serialize(mongo::OID* var, String const& name, bool PersistToDB)
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

		void MongoReplicableDeserializer::serialize(mongo::OID* var, String const& name, bool PersistToDB)
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

		template class ReplicableSerializer<MongoSerializer>;
		template class ReplicableDeserializer<MongoDeserializer>;
	}
}
