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

#ifndef SPRAWL_REPLICABLE_MAX_DEPTH
#		define SPRAWL_REPLICABLE_MAX_DEPTH 64
#endif

#include "Serializer.hpp"
#include "BinarySerializer.hpp"
#include "JSONSerializer.hpp"
#include <map>

namespace sprawl
{
	namespace serialization
	{
		class ReplicationKey
		{
		public:
			ReplicationKey()
				: m_size(0)
				, m_data()
			{
				//
			}

			ReplicationKey( const ReplicationKey& other );

			uint32_t size() const
			{
				return m_size;
			}

			int32_t& operator[](const uint32_t index)
			{
				return m_data[index];
			}

			const int32_t& operator[](const uint32_t index) const
			{
				return m_data[index];
			}

			void push_back(const int32_t val)
			{
				if(m_size == SPRAWL_REPLICABLE_MAX_DEPTH)
				{
					SPRAWL_ABORT_MSG("Replicable key depth (%d) exceeded. Redefine SPRAWL_REPLICABLE_MAX_DEPTH", SPRAWL_REPLICABLE_MAX_DEPTH);
				}
				m_data[m_size++] = val;
			}

			void pop_back()
			{
				--m_size;
			}

			int32_t& front()
			{
				return m_data[0];
			}

			int32_t& back()
			{
				return m_data[m_size - 1];
			}

			const int32_t& front() const
			{
				return m_data[0];
			}

			const int32_t& back() const
			{
				return m_data[m_size - 1];
			}

			bool empty() const
			{
				return (m_size == 0);
			}

			bool operator==( const ReplicationKey& other ) const;

			bool operator<( const ReplicationKey& other ) const;

			void clear()
			{
				m_size = 0;
			}

			void Serialize(SerializerBase& s);

		protected:
			friend struct RKeyHash;
			uint32_t m_size;
			int32_t m_data[SPRAWL_REPLICABLE_MAX_DEPTH];
		};

		struct RKeyHash
		{
			std::size_t operator()(const ReplicationKey& key) const
			{
				return sprawl::murmur3::Hash(key.m_data, key.m_size * sizeof(int32_t));
			}
		};

		bool StartsWith(const ReplicationKey& x, const ReplicationKey& y);

		template<typename T>
		class ReplicableBase : virtual public SerializerBase
		{
		protected:
			typedef sprawl::memory::StlWrapper<std::pair<ReplicationKey, sprawl::String>> ReplicationMapAllocator;
			typedef sprawl::memory::StlWrapper<std::pair<sprawl::String, int32_t>> StringToKeyAllocator;
			typedef sprawl::memory::StlWrapper<std::pair<int32_t, sprawl::String>> KeyToStringAllocator;
			typedef std::map<ReplicationKey, sprawl::String, std::less<ReplicationKey>, ReplicationMapAllocator> ReplicationMap;
			typedef std::unordered_map<sprawl::String, int32_t, std::hash<sprawl::String>, std::equal_to<sprawl::String>, StringToKeyAllocator> StringToKeyMap;
			typedef std::unordered_map<int32_t, sprawl::String, std::hash<int32_t>, std::equal_to<int32_t>, KeyToStringAllocator> KeyToStringMap;

			ReplicableBase();

			virtual ~ReplicableBase();

		public:
			virtual uint32_t GetVersion() override { return m_serializer->GetVersion(); }
			virtual bool IsValid() override { return true; }
			virtual bool Error() override { return false; }
			virtual size_t Size() override { return 0; }
			virtual void SetVersion(uint32_t i) override
			{
				m_serializer->SetVersion(i);
			}

			virtual bool IsMongoStream() override { return m_serializer->IsMongoStream(); }
			virtual void Reset() override
			{
				this->m_data.clear();
				this->m_depth_tracker.clear();
				this->m_diffs.clear();
				this->m_removed.clear();
				this->m_name_index.clear();
				this->m_current_map_key.clear();
				this->m_highest_name = 1;
				this->m_current_key.clear();
				this->m_keyindex.clear();
			}

			virtual bool IsReplicable() override { return true; }

			void StartArray( const sprawl::String& name, uint32_t& size, bool b ) override;

			void EndArray() override;

			virtual uint32_t StartObject( const sprawl::String& name, bool b) override;

			virtual uint32_t StartMap( const sprawl::String& name, bool b) override;

			void EndMap();

			void EndObject() override;

			sprawl::String GetNextKey() override;

			StringSet GetDeletedKeys(const sprawl::String& name) override;

		protected:
			template<typename T2>
			void serialize_impl( T2* var, const sprawl::String& name, bool PersistToDB)
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
					m_data.insert(std::make_pair(m_current_key, m_serializer->Str()));
					if(!m_marked)
					{
						(*m_baseline) % sprawl::serialization::prepare_data(*var, name, PersistToDB);
					}
				}
				PopKey();
			}

		public:
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

		protected:
			virtual void PushKey(const sprawl::String& name, bool /*forArray*/ = false);

			virtual void PopKey();

			void DecrementCurrentKeyCounter();

			typename ReplicableBase<T>::ReplicationMap m_data;
			typename ReplicableBase<T>::ReplicationMap m_diffs;
			std::set<ReplicationKey, std::less<ReplicationKey>, sprawl::memory::StlWrapper<ReplicationKey>> m_removed;
			std::unordered_map<ReplicationKey, int16_t, RKeyHash, std::equal_to<ReplicationKey>, sprawl::memory::StlWrapper<ReplicationKey>> m_depth_tracker;
			ReplicationKey m_current_key;
			StringToKeyMap m_name_index;
			int32_t m_highest_name;
			std::vector<ReplicationKey, sprawl::memory::StlWrapper<ReplicationKey>> m_current_map_key;
			KeyToStringMap m_keyindex;
			T* m_serializer;
			T* m_baseline;
			bool m_marked;
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
				this->m_baseline = new T();
			}

			using Serializer::operator%;
			using Serializer::IsLoading;

			using ReplicableBase<T>::serialize;
			using ReplicableBase<T>::IsBinary;
			using ReplicableBase<T>::IsMongoStream;
			using ReplicableBase<T>::IsReplicable;
			using ReplicableBase<T>::IsValid;
			using ReplicableBase<T>::GetVersion;
			using ReplicableBase<T>::SetVersion;
			using ReplicableBase<T>::Size;

			using ReplicableBase<T>::StartObject;
			using ReplicableBase<T>::EndObject;
			using ReplicableBase<T>::StartArray;
			using ReplicableBase<T>::EndArray;
			using ReplicableBase<T>::StartMap;
			using ReplicableBase<T>::EndMap;
			using ReplicableBase<T>::GetNextKey;
			using ReplicableBase<T>::GetDeletedKeys;

			virtual void Reset() override
			{
				ReplicableBase<T>::Reset();
				m_marked_data.clear();
				this->m_marked = false;
			}

			void Mark()
			{
				//Save what we just serialized to reference the next time we serialize, and return the serializer to a blank state.
				this->m_marked_data = std::move( this->m_data );
				this->m_depth_tracker.clear();
				this->m_diffs.clear();
				this->m_removed.clear();
				this->m_keyindex.clear();
				this->m_marked = true;
			}
			const char* Data() override
			{
				return Str().c_str();
			}

			sprawl::String Str() override
			{
				//Str is a combination of Mark() and diff().
				//Doing this enables the replicable serializer to follow the exact same API as a regular serializer, so they can be changed out easily.
				//But it does sacrifice some control over when these two things happen - most people will probably not care.
				sprawl::String ret = diff();
				Mark();
				return std::move(ret);
			}

			sprawl::String diff()
			{
				return GetDiff( this->m_data, this->m_marked_data );
			}

			sprawl::String rdiff()
			{
				return GetDiff( this->m_marked_data, this->m_data );
			}

			sprawl::String getBaselineStr()
			{
				return this->m_baseline->Str();
			}

		protected:
			sprawl::String GetDiff(
				const typename ReplicableBase<T>::ReplicationMap& data,
				const typename ReplicableBase<T>::ReplicationMap& markedData
			);

			typename ReplicableBase<T>::ReplicationMap m_marked_data;
		};

		template<typename T>
		class ReplicableDeserializer : public ReplicableBase<T>, public Deserializer
		{
		public:
			ReplicableDeserializer(const sprawl::String& data)
				: ReplicableBase<T>()
				, Deserializer()
			{
				this->m_serializer = new T("", 0, false);
				T serializer(data);

				serializer % sprawl::serialization::prepare_data(this->m_keyindex, "keyindex");
				serializer % sprawl::serialization::prepare_data(this->m_diffs, "diffs");
				serializer % sprawl::serialization::prepare_data(this->m_removed, "removed");

				for(auto& kvp : this->m_keyindex)
				{
					this->m_name_index[kvp.second] = kvp.first;
				}
			}

			ReplicableDeserializer(const char* data, size_t length)
				: ReplicableBase<T>()
				, Deserializer()
			{
				this->m_serializer = new T("", 0, false);
				T serializer(data, length);

				serializer % sprawl::serialization::prepare_data(this->m_keyindex, "keyindex");
				serializer % sprawl::serialization::prepare_data(this->m_diffs, "diffs");
				serializer % sprawl::serialization::prepare_data(this->m_removed, "removed");

				for(auto& kvp : this->m_keyindex)
				{
					this->m_name_index[kvp.second] = kvp.first;
				}
			}

			using Deserializer::operator%;
			using Deserializer::IsLoading;

			using ReplicableBase<T>::serialize;
			using ReplicableBase<T>::IsBinary;
			using ReplicableBase<T>::IsMongoStream;
			using ReplicableBase<T>::IsReplicable;
			using ReplicableBase<T>::IsValid;
			using ReplicableBase<T>::GetVersion;
			using ReplicableBase<T>::SetVersion;
			using ReplicableBase<T>::Size;
			using ReplicableBase<T>::Reset;

			using ReplicableBase<T>::StartObject;
			using ReplicableBase<T>::EndObject;
			using ReplicableBase<T>::StartArray;
			using ReplicableBase<T>::EndArray;
			using ReplicableBase<T>::StartMap;
			using ReplicableBase<T>::EndMap;
			using ReplicableBase<T>::GetNextKey;
			using ReplicableBase<T>::GetDeletedKeys;

			const char* Data() override
			{
				return Str().c_str();
			}

			sprawl::String Str() override
			{
				return "";
			}

			void Data(const sprawl::String& data)
			{
				this->m_diffs.clear();
				this->m_data.clear();
				this->m_depth_tracker.clear();
				T serializer(data);
				serializer % sprawl::serialization::prepare_data(this->m_diffs, "diffs");
			}

			void Data(const char* data, size_t length)
			{
				this->m_diffs.clear();
				this->m_data.clear();
				this->m_depth_tracker.clear();
				T serializer(data, length);
				serializer % sprawl::serialization::prepare_data(this->m_diffs, "diffs");
			}
		protected:
			virtual SerializerBase* GetAnother(const sprawl::String& data) override { return new T(data, false); }
			virtual SerializerBase* GetAnother() override { SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return nullptr; }
		};

		class JSONSerializer;
		class BinarySerializer;
		class YAMLSerializer;

		extern template class ReplicableSerializer<JSONSerializer>;
		extern template class ReplicableSerializer<BinarySerializer>;
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
