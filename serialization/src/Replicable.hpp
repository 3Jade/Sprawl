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
#include <map>
#include <boost/functional/hash.hpp>

namespace sprawl
{
	namespace serialization
	{
		template <typename Container> // we can make this generic for any container [1]
		struct container_hash
		{
			std::size_t operator()(const std::vector<Container> & c) const
			{
				return boost::hash_range(c.begin(), c.end());
			}
		};

		template <typename Container>
		struct container_comp
		{
			bool operator()(const std::vector<Container> & x, const std::vector<Container> & y) const
			{
				int xsize = x.size();
				int ysize = y.size();
				for(int i = 0; i < std::min(xsize, ysize); i++)
				{
					if(x[i] < y[i])
					{
						return true;
					}
					if(y[i] < x[i])
					{
						return false;
					}
				}

				if(xsize < ysize)
				{
					return true;
				}

				return false;
			}
		};

		template <typename Container>
		bool StartsWith(const std::vector<Container> & x, const std::vector<Container> & y)
		{
			int xsize = x.size();
			int ysize = y.size();
			if(xsize < ysize)
				return false;
			for(int i = 0; i < ysize; i++)
			{
				if(x[i] != y[i])
					return false;
			}
			return true;
		}

		template<typename T>
		class ReplicableBase : virtual public SerializerBase
		{
		protected:
			ReplicableBase()
				: m_data()
				, m_diffs()
				, m_depth_tracker()
				, m_current_key()
				, m_serializer(nullptr)
				, m_name_index()
				, highest_name(0)
				, m_current_map_key()
				, m_keyindex()
			{
				this->m_bIsReplicable = true;
			}

			virtual ~ReplicableBase()
			{
				if(m_serializer)
					delete m_serializer;
			}

		public:
			virtual void SetVersion(uint32_t i) override
			{
				m_version = i;
				m_serializer->SetVersion(i);
			}

			virtual void Reset() override
			{
				this->m_data.clear();
				this->m_depth_tracker.clear();
				this->m_diffs.clear();
				this->m_removed.clear();
				this->m_name_index.clear();
				this->m_current_map_key.clear();
				this->highest_name = 0;
				this->m_current_key.clear();
				this->m_keyindex.clear();
			}

			void StartArray( const std::string& name, size_t& size, bool ) override
			{
				PushKey(name);
				if(IsLoading())
				{
					//A little less pleasant than would be ideal...
					//Go through the list of unconsumed keys to determine the size of the array post-merge.
					std::set<std::vector<int16_t>>::iterator it;
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
						m_current_key.push_back(i+1);

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
					std::map<std::vector<int16_t>, std::string>::iterator it2;
					while(true)
					{
						if(!m_serializer->IsBinary())
						{
							m_current_key.push_back(m_name_index[name]);
						}
						else
						{
							m_current_key.push_back(-1);
						}
						m_current_key.push_back(size+1);

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
			}

			void EndArray() override
			{
				PopKey();
			}

			int StartObject( const std::string& name, bool ) override
			{
				PushKey(name);
				return 0;
			}

			int StartMap( const std::string& name, bool ) override
			{
				PushKey(name);
				m_current_map_key.push_back(m_current_key);
				if(IsLoading())
				{
					std::set<std::string> unique_subkeys;
					for(auto &kvp : m_diffs)
					{
						int size = m_current_key.size();
						if(kvp.first.size() != size + 2) continue;
						if(!StartsWith(kvp.first, m_current_key)) continue;
						if(kvp.first[size] == -1) continue;

						unique_subkeys.insert(m_keyindex[kvp.first[size]]);
					}
					return unique_subkeys.size();
				}
				else
				{
					return 0;
				}
			}

			void EndMap()
			{
				m_current_map_key.pop_back();
				EndObject();
			}

			void EndObject() override
			{
				PopKey();
			}

			std::string GetNextKey() override
			{
				//Get the next key that belongs to our current element.
				//TODO: Have GetMap() just return a list of keys to grab that the calling function can iterate so we can avoid these string compares.
				for(auto &kvp : m_diffs)
				{
					int size = m_current_key.size();
					if(kvp.first.size() != size + 2) continue;
					if(!StartsWith(kvp.first, m_current_key)) continue;
					if(kvp.first[size] == -1) continue;

					return m_keyindex[kvp.first[size]];
				}
				return "";
			}

			std::unordered_set<std::string> GetDeletedKeys(const std::string& name) override
			{
				std::unordered_set<std::string> deleted_keys;
				PushKey(name);
				//Go through our removed keys and figure out which ones belong to this map.
				for(auto it = m_removed.begin(); it != m_removed.end(); )
				{
					auto delete_it = it++;

					int size = m_current_key.size();
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

		protected:
			template<typename T2>
			void serialize_impl( T2 *var, const std::string &name, bool PersistToDB)
			{
				m_serializer->Reset();
				PushKey(name);
				if(IsLoading())
				{
					typename T::serializer_type serializer;
					serializer % m_current_key;

					//Do nothing if this element hasn't changed.
					auto it = m_diffs.find(m_current_key);
					if(it != m_diffs.end())
					{
						m_serializer->Data(it->second);
						(*m_serializer) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
						m_diffs.erase(it);
					}
				}
				else
				{
					(*m_serializer) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
					m_data[m_current_key] = m_serializer->Str();
				}
				PopKey();
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
		protected:
			void PushKey(const std::string& name)
			{
				if(!m_serializer->IsBinary() || (!m_current_map_key.empty() && m_current_key == m_current_map_key.back()))
				{
					if( m_name_index.count(name) == 0 )
					{
						m_name_index[name] = highest_name++;
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

			void PopKey()
			{
				m_current_key.pop_back();
				m_current_key.pop_back();
			}

			void DecrementCurrentKeyCounter()
			{
				m_current_key.pop_back();
				m_depth_tracker[m_current_key]--;
				m_current_key.push_back(-1);
			}

			std::map<std::vector<int16_t>, std::string, container_comp<int16_t>> m_data;
			std::map<std::vector<int16_t>, std::string, container_comp<int16_t>> m_diffs;
			std::set<std::vector<int16_t>, container_comp<int16_t>> m_removed;
			std::unordered_map<std::vector<int16_t>, int16_t, container_hash<int16_t>> m_depth_tracker;
			std::vector<int16_t> m_current_key;
			SerializerBase* m_serializer;
			std::unordered_map<std::string, int> m_name_index;
			int highest_name;
			std::vector<std::vector<int16_t>> m_current_map_key;
			std::unordered_map<int16_t, std::string> m_keyindex;
		};

		template<typename T>
		class ReplicableSerializer : public ReplicableBase<T>, public Serializer
		{
		public:
			ReplicableSerializer()
				: ReplicableBase<T>()
				, Serializer()
				, m_marked_data()
			{
				this->m_serializer = new T(false);
			}

			virtual void Reset() override
			{
				ReplicableBase<T>::Reset();
				m_marked_data.clear();
			}

			void Mark()
			{
				//Save what we just serialized to reference the next time we serialize, and return the serializer to a blank state.
				this->m_marked_data = std::move( this->m_data );
				this->m_depth_tracker.clear();
				this->m_diffs.clear();
				this->m_removed.clear();
				this->m_keyindex.clear();
			}
			const char* Data() override
			{
				return Str().c_str();
			}

			std::string Str() override
			{
				//Str is a combination of Mark() and diff().
				//Doing this enables the replicable serializer to follow the exact same API as a regular serializer, so they can be changed out easily.
				//But it does sacrifice some control over when these two things happen - most people will probably not care.
				std::string ret = diff();
				Mark();
				return std::move(ret);
			}

			std::string diff()
			{
				//Meat and potatoes of this replication system: Go through all of our keys and determine if they've changed since the last time we serialized data.
				//Ignore what hasn't, return what has.
				std::unordered_set<int16_t> used_indices;

				//Adds and changes
				for( auto& kvp : this->m_data )
				{
					auto it = this->m_marked_data.find(kvp.first);
					if( it == this->m_marked_data.end() || kvp.second != it->second )
					{
						this->m_diffs.insert(kvp);
						for(int idx = 0; idx < kvp.first.size(); idx += 2)
						{
							if(kvp.first[idx] != -1)
							{
								used_indices.insert(kvp.first[idx]);
							}
						}
					}
				}

				//Removes get their own special treatment
				for( auto& kvp : this->m_marked_data )
				{
					auto it = this->m_data.find(kvp.first);
					if( it == this->m_data.end() )
					{
						this->m_removed.insert(kvp.first);
						for(int idx = 0; idx < kvp.first.size(); idx += 2)
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
		private:
			std::map<std::vector<int16_t>, std::string, container_comp<int16_t>> m_marked_data;
		};

		template<typename T>
		class ReplicableDeserializer : public ReplicableBase<T>, public Deserializer
		{
		public:
			ReplicableDeserializer(const std::string& data)
				: ReplicableBase<T>()
				, Deserializer()
			{
				this->m_serializer = new T("", false);
				T serializer(data);

				serializer % sprawl::serialization::prepare_data(this->m_keyindex, "keyindex");
				serializer % sprawl::serialization::prepare_data(this->m_diffs, "diffs");
				serializer % sprawl::serialization::prepare_data(this->m_removed, "removed");

				for(auto& kvp : this->m_keyindex)
				{
					this->m_name_index[kvp.second] = kvp.first;
				}
			}
			const char* Data() override
			{
				return Str().c_str();
			}

			std::string Str() override
			{
				return "";
			}

			void Data(const std::string& data)
			{
				this->m_diffs.clear();
				this->m_data.clear();
				this->m_depth_tracker.clear();
				T serializer(data);
				serializer % sprawl::serialization::prepare_data(this->m_diffs, "diffs");
			}
		protected:
			virtual SerializerBase* GetAnother(const std::string& data) override { return new T(data, false); }
			virtual SerializerBase* GetAnother() override { throw std::exception(); }
		};
	}
}
