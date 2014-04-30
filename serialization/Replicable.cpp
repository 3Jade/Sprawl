#include "Replicable.hpp"
#include "JSONSerializer.hpp"
#include "BinarySerializer.hpp"

namespace sprawl
{
	namespace serialization
	{
		ReplicationKey::ReplicationKey(const sprawl::serialization::ReplicationKey& other)
			: m_size(other.m_size)
		{
			memcpy( m_data, other.m_data, sizeof(int32_t) * other.m_size );
		}

		bool ReplicationKey::operator==(const ReplicationKey& other) const
		{
			if( m_size != other.m_size )
			{
				return false;
			}
			return (memcmp( m_data, other.m_data, sizeof(int32_t) * m_size ) == 0);
		}

		bool ReplicationKey::operator<(const ReplicationKey& other) const
		{
			uint32_t size = (m_size < other.m_size ? m_size : other.m_size);

			for(uint32_t i = 0; i < size; ++i)
			{
				if( m_data[i] == other.m_data[i] )
					continue;

				return abs(m_data[i]) < abs(other.m_data[i]);

			}

			return m_size < other.m_size;
		}

		void ReplicationKey::Serialize(SerializerBase& s)
		{
			if(s.IsBinary())
			{
				s % prepare_data(m_size, "size", true);
			}
			s.StartArray("ReplicationKey", m_size, true);
			if(s.IsBinary())
			{
				s.serialize(m_data, m_size * sizeof(int32_t), "data", true);
			}
			else
			{
				for(uint32_t i=0; i<m_size; ++i)
				{
					s % prepare_data(m_data[i], "data");
				}
			}
			s.EndArray();
		}

		bool StartsWith(const ReplicationKey& x, const ReplicationKey& y)
		{
			uint32_t xsize = x.size();
			uint32_t ysize = y.size();
			if(xsize < ysize)
				return false;
			const int32_t* xArr = &x[0];
			const int32_t* yArr = &y[0];
			for(uint32_t i = 0; i < ysize; i++)
			{
				if(xArr[i] != yArr[i])
					return false;
			}
			return true;
		}

		template<typename T>
		ReplicableBase<T>::ReplicableBase()
			: m_data()
			, m_diffs()
			, m_depth_tracker()
			, m_current_key()
			, m_name_index()
			, m_highest_name(1)
			, m_current_map_key()
			, m_keyindex()
			, m_serializer(nullptr)
			, m_baseline(nullptr)
			, m_marked(false)
		{
		}

		template<typename T>
		ReplicableBase<T>::~ReplicableBase()
		{
			if(m_serializer)
				delete m_serializer;
			if(m_baseline)
				delete m_baseline;
		}

		template<typename T>
		void ReplicableBase<T>::StartArray(const String& name, uint32_t& size, bool b)
		{
			PushKey(name, true);
			if(IsLoading())
			{
				//A little less pleasant than would be ideal...
				//Go through the list of unconsumed keys to determine the size of the array post-merge.
				std::set<ReplicationKey, std::less<ReplicationKey>, sprawl::memory::StlWrapper<ReplicationKey>>::iterator it;
				bool shrunk = false;
				for(size_t i=0; i<size; i++)
				{
					if(!m_serializer->IsBinary())
					{
						m_current_key.push_back(m_name_index[name]);
					}
					else
					{
						m_current_key.push_back(-1);
					}
					m_current_key.push_back((short)(i+1));

					it = m_removed.find(m_current_key);
					if(it != m_removed.end())
					{
						if(!shrunk)
						{
							//Elements deleted, size reduced.
							size = i;
							shrunk = true;
						}
						if(shrunk)
						{
							m_removed.erase(it);
						}
					}
					m_current_key.pop_back();
					m_current_key.pop_back();
				}
				typename ReplicableBase<T>::ReplicationMap::iterator it2;
				for(;;)
				{
					if(!m_serializer->IsBinary())
					{
						m_current_key.push_back(m_name_index[name]);
					}
					else
					{
						m_current_key.push_back(-1);
					}
					m_current_key.push_back((short)(size+1));

					it2 = m_diffs.find(m_current_key);
					if(it2 == m_diffs.end())
					{
						m_current_key.pop_back();
						m_current_key.pop_back();
						break;
					}
					else
					{
						if(shrunk)
						{
							m_diffs.erase(it2);
						}
						else
						{
							size++;
						}
					}
					m_current_key.pop_back();
					m_current_key.pop_back();
				}
			} // if(IsLoading())
			else if(!m_marked)
			{
				m_baseline->StartArray(name, size, b);
			}
		}

		template<typename T>
		void ReplicableBase<T>::EndArray()
		{
			PopKey();
			if(!m_marked)
			{
				m_baseline->EndArray();
			}
		}

		template<typename T>
		uint32_t ReplicableBase<T>::StartObject(const String& name, bool b)
		{
			PushKey(name);
			if(!m_marked)
			{
				m_baseline->StartObject(name, b);
			}
			return 0;
		}

		template<typename T>
		uint32_t ReplicableBase<T>::StartMap(const String& name, bool b)
		{
			PushKey(name);
			m_current_map_key.push_back(m_current_key);
			if(IsLoading())
			{
				std::set<sprawl::String, std::less<sprawl::String>, sprawl::memory::StlWrapper<sprawl::String>> unique_subkeys;
				for(auto& kvp : m_diffs)
				{
					size_t size = m_current_key.size();
					if(kvp.first.size() != size + 2) continue;
					if(!StartsWith(kvp.first, m_current_key)) continue;
					if(kvp.first[size] == -1) continue;

					unique_subkeys.insert(m_keyindex[kvp.first[size]]);
				}
				return unique_subkeys.size();
			}
			else if(!m_marked)
			{
				m_baseline->StartMap(name, b);
			}
			return 0;
		}

		template<typename T>
		void ReplicableBase<T>::EndMap()
		{
			m_current_map_key.pop_back();
			PopKey();
			if(!m_marked)
			{
				m_baseline->EndMap();
			}
		}

		template<typename T>
		void ReplicableBase<T>::EndObject()
		{
			PopKey();
			if(!m_marked)
			{
				m_baseline->EndObject();
			}
		}

		template<typename T>
		String ReplicableBase<T>::GetNextKey()
		{
			//Get the next key that belongs to our current element.
			//TODO: Have GetMap() just return a list of keys to grab that the calling function can iterate so we can avoid these string compares.
			for(auto& kvp : m_diffs)
			{
				size_t size = m_current_key.size();
				if(kvp.first.size() != size + 2) continue;
				if(!StartsWith(kvp.first, m_current_key)) continue;
				if(kvp.first[size] == -1) continue;

				return m_keyindex[kvp.first[size]];
			}
			return "";
		}

		template<typename T>
		SerializerBase::StringSet ReplicableBase<T>::GetDeletedKeys(const String& name)
		{
			StringSet deleted_keys;
			PushKey(name);
			//Go through our removed keys and figure out which ones belong to this map.
			for(auto it = m_removed.begin(); it != m_removed.end(); )
			{
				auto delete_it = it++;

				size_t size = m_current_key.size();
				if(delete_it->size() != size + 2) continue;
				if(!StartsWith(*delete_it, m_current_key)) continue;
				if((*delete_it)[size] == -1) continue;

				deleted_keys.insert(m_keyindex[(*delete_it)[size]]);
				m_removed.erase(delete_it);
			}
			DecrementCurrentKeyCounter();
			PopKey();
			return std::move(deleted_keys);
		}

		template<typename T>
		void ReplicableBase<T>::PushKey(const String& name, bool)
		{
			if(!m_serializer->IsBinary() || (!m_current_map_key.empty() && m_current_key == m_current_map_key.back()))
			{
				if( m_name_index.count(name) == 0 )
				{
					m_name_index[name] = m_highest_name++;
				}
				m_current_key.push_back(m_name_index[name]);
			}
			else
			{
				m_current_key.push_back(-1);
			}
			int16_t& depth = m_depth_tracker[m_current_key];
			depth++;
			m_current_key.push_back( depth );
		}

		template<typename T>
		void ReplicableBase<T>::PopKey()
		{
			m_current_key.pop_back();
			m_current_key.pop_back();
		}

		template<typename T>
		void ReplicableBase<T>::DecrementCurrentKeyCounter()
		{
			m_current_key.pop_back();
			m_depth_tracker[m_current_key]--;
			m_current_key.push_back(-1);
		}

		template<typename T>
		String ReplicableSerializer<T>::GetDiff(const typename ReplicableBase<T>::ReplicationMap& data, const typename ReplicableBase<T>::ReplicationMap& markedData)
		{
			//Meat and potatoes of this replication system: Go through all of our keys and determine if they've changed since the last time we serialized data.
			//Ignore what hasn't, return what has.
			std::unordered_set<int32_t> used_indices;

			this->m_diffs.clear();
			this->m_removed.clear();
			this->m_keyindex.clear();

			//Adds and changes
			for( auto& kvp : data )
			{
				auto it = markedData.find(kvp.first);
				if( it == markedData.end() || kvp.second != it->second )
				{
					this->m_diffs.insert(kvp);
					for(size_t idx = 0; idx < kvp.first.size(); idx += 2)
					{
						if(kvp.first[idx] != -1)
						{
							used_indices.insert(kvp.first[idx]);
						}
					}
				}
			}

			//Removes get their own special treatment
			for( auto& kvp : markedData )
			{
				auto it = data.find(kvp.first);
				if( it == data.end() )
				{
					this->m_removed.insert(kvp.first);
					for(size_t idx = 0; idx < kvp.first.size(); idx += 2)
					{
						if(kvp.first[idx] != -1)
						{
							used_indices.insert(kvp.first[idx]);
						}
					}
				}
			}
			T serializer;

			for( auto& kvp : this->m_name_index )
			{
				if(used_indices.count(kvp.second) == 0) continue;
				this->m_keyindex[kvp.second] = kvp.first;
			}

			serializer % sprawl::serialization::prepare_data(this->m_keyindex, "keyindex");
			serializer % sprawl::serialization::prepare_data(this->m_diffs, "diffs");
			serializer % sprawl::serialization::prepare_data(this->m_removed, "removed");

			return serializer.Str();
		}

		template class ReplicableSerializer<JSONSerializer>;
		template class ReplicableSerializer<BinarySerializer>;
	}
}
