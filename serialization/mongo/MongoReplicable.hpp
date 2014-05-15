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
			virtual void StartArray(const sprawl::String &name, uint32_t &size, bool b) override;

			virtual uint32_t StartObject(const sprawl::String &name, bool b) override;

			virtual uint32_t StartMap(const sprawl::String &name, bool b) override;

			mongo::BSONObj getBaselineObj()
			{
				return m_baseline->Obj();
			}

			mongo::BSONObj getBaselineTempObj()
			{
				return m_baseline->tempObj();
			}

			virtual void Mark();

			virtual void Reset();

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
			virtual void serialize(mongo::OID* var, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(mongo::BSONObj* var, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(mongo::Date_t* var, const sprawl::String& name, bool PersistToDB) override;

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
			virtual void PushKey(const sprawl::String& name, bool forArray = false) override;

			virtual void PopKey() override;

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

			std::pair<mongo::BSONObj, mongo::BSONObj> BuildDelta(const BuildDeltaParams& params);

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

			virtual void serialize(mongo::OID* var, const sprawl::String& name, bool PersistToDB) override;

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
			
		extern template class ReplicableSerializer<MongoSerializer>;
		extern template class ReplicableDeserializer<MongoDeserializer>;
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
