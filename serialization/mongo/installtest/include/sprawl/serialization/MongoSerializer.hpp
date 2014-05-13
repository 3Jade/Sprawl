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
	#pragma warning( disable: 4250; disable: 4996 )
#endif
//Required for mongo apparently.
#include <sstream>
using std::stringstream;
using std::hex;

#include <mongo/client/redef_macros.h>

#include <mongo/util/log.h>

#include <mongo/bson/bsonelement.h>
#include <mongo/bson/bsonobj.h>
#include <mongo/bson/oid.h>
#include <mongo/bson/util/misc.h>
#include <mongo/bson/bsonobjbuilder.h>
#include <mongo/bson/bsonobjiterator.h>
#include <mongo/bson/bson-inl.h>
#include <mongo/bson/bson_db.h>

#include <mongo/db/json.h>

#include <mongo/client/undef_macros.h>

#include "Serializer.hpp"
#include "JSONSerializer.hpp"
#include <deque>
#include "../memory/StlWrapper.hpp"

namespace sprawl
{
	namespace serialization
	{
		class MongoSerializerBase : virtual public SerializerBase
		{
		public:
			typedef class MongoSerializer serializer_type;
			typedef class MongoDeserializer deserializer_type;
			virtual uint32_t GetVersion() override { return m_version; }
			virtual bool IsValid() override { return m_bIsValid; }
			virtual bool Error() override { return m_bError; }
			void ClearError(){ m_bError = false; }
			virtual bool IsMongoStream() override { return true; }
			virtual void SetVersion(uint32_t i) override
			{
				m_version = i;
			}
			virtual void Reset() override;

			using SerializerBase::Data;
			using SerializerBase::operator%;

			virtual const char* Data() override
			{
				EnsureVersion();
				return m_builder->asTempObj().objdata();
			}

			virtual sprawl::String Str() override
			{
				if(IsSaving())
				{
					EnsureVersion();
					return sprawl::String(m_builder->asTempObj().objdata(), m_builder->asTempObj().objsize());
				}
				else
				{
					return m_obj.toString();
				}
			}

			sprawl::String JSONStr()
			{
				if(IsSaving())
				{
					EnsureVersion();
					return m_builder->asTempObj().jsonString();
				}
				else
				{
					return m_obj.jsonString();
				}

			}

			virtual size_t Size() override
			{
				if(IsSaving())
				{
					return m_builder->asTempObj().objsize();
				}
				else
				{
					return m_obj.objsize();
				}
			}
			bool More() { return true; }
			mongo::BSONObj tempObj()
			{
				if(IsSaving())
				{
					EnsureVersion();
					return m_builder->asTempObj();
				}
				else
				{
					return m_obj;
				}
			}
			mongo::BSONObj Obj()
			{
				if(IsSaving())
				{
					EnsureVersion();
					m_bIsValid = false;
					return m_builder->obj();
				}
				else
				{
					return m_obj;
				}
			}
			void EnsureVersion();

			virtual SerializerBase& operator%(BinaryData&& var) override;

		protected:
			template<typename T>
			friend class ReplicableBase;
			using SerializerBase::serialize;

			friend class MongoReplicableDeserializer;
			friend class MongoReplicableSerializer;
		public:
			virtual void serialize(mongo::OID* var, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(mongo::BSONObj* var, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(mongo::Date_t* var, const sprawl::String& name, bool PersistToDB) override;


			virtual void serialize(int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(long int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(long long int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB)  override;

			virtual void serialize(short int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(char* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(float* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(double* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(long double* /*var*/, const uint32_t /*bytes*/, const sprawl::String& /*name*/, bool /*PersistToDB*/) override
			{
				SPRAWL_ABORT_MSG("Mongo does not support objects of type long double. If you can accept the data loss, cast down to double.");
			}

			virtual void serialize(bool* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(unsigned int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(unsigned long int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(unsigned long long int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(unsigned short int* var, const uint32_t bytes, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(unsigned char* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(std::string* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override;

			virtual void serialize(sprawl::String* var, const uint32_t /*bytes*/, const sprawl::String& name, bool PersistToDB) override;

			virtual uint32_t StartObject(const sprawl::String& str, bool PersistToDB = true) override;

			virtual void EndObject() override;

			virtual void StartArray(const sprawl::String& str, uint32_t& size, bool PersistToDB = true) override;

			virtual void EndArray() override;


			sprawl::String GetNextKey();

		protected:

			MongoSerializerBase()
				: SerializerBase()
				, m_disableDepth(0)
				, m_obj()
				, m_builder(new mongo::BSONObjBuilder())
				, m_arrayBuilders()
				, m_objectBuilders()
				, m_arrays()
				, m_objects()
				, m_stateTracker()
				, m_elementList()
				, m_version(0)
				, m_bIsValid(true)
				, m_bWithMetadata(true)
				, m_bError(false)
			{}
			virtual ~MongoSerializerBase();

			int m_disableDepth;
			enum class State { None, Array, Object };
			mongo::BSONObj m_obj;
			mongo::BSONObjBuilder* m_builder;

			std::deque<std::pair<sprawl::String, mongo::BSONArrayBuilder*>, sprawl::memory::StlWrapper<std::pair<sprawl::String, mongo::BSONArrayBuilder*>>> m_arrayBuilders;
			std::deque<std::pair<sprawl::String, mongo::BSONObjBuilder*>, sprawl::memory::StlWrapper<std::pair<sprawl::String, mongo::BSONObjBuilder*>>> m_objectBuilders;
			std::deque<std::pair<sprawl::String, std::deque<mongo::BSONElement>>, sprawl::memory::StlWrapper<std::pair<sprawl::String, std::deque<mongo::BSONElement>>>> m_arrays;
			std::deque<mongo::BSONObj, sprawl::memory::StlWrapper<mongo::BSONObj>> m_objects;
			std::deque<State, sprawl::memory::StlWrapper<State>> m_stateTracker;
			std::deque<std::list<mongo::BSONElement>, sprawl::memory::StlWrapper<mongo::BSONElement>> m_elementList;

			//Copied and pasted to avoid indirection with virtual inheritance
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bWithMetadata;
			bool m_bError;
		private:
			MongoSerializerBase(const SerializerBase&);
			MongoSerializerBase& operator=(const SerializerBase&);
		};

		class MongoSerializer : public MongoSerializerBase, public Serializer
		{
		public:
			using Serializer::operator%;
			using Serializer::IsLoading;

			using MongoSerializerBase::serialize;
			using MongoSerializerBase::IsBinary;
			using MongoSerializerBase::IsMongoStream;
			using MongoSerializerBase::IsReplicable;
			using MongoSerializerBase::IsValid;
			using MongoSerializerBase::Str;
			using MongoSerializerBase::Data;
			using MongoSerializerBase::GetVersion;
			using MongoSerializerBase::SetVersion;
			using MongoSerializerBase::Reset;
			using MongoSerializerBase::Size;

			using MongoSerializerBase::StartObject;
			using MongoSerializerBase::EndObject;
			using MongoSerializerBase::StartArray;
			using MongoSerializerBase::EndArray;
			using MongoSerializerBase::StartMap;
			using MongoSerializerBase::EndMap;
			using MongoSerializerBase::GetNextKey;
			using MongoSerializerBase::GetDeletedKeys;

			virtual SerializerBase& operator%(SerializationData<Serializer>&& var) override
			{
				sprawl::String str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase& operator%(SerializationData<MongoSerializer>&& var) override
			{
				sprawl::String str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			//Reserve the first sizeof(int32_t)*3 bytes of space to hold metadata (size, version, and checksum).
			MongoSerializer()
			{
				m_bIsValid = true;
			}
			MongoSerializer(bool)
			{
				m_bIsValid = true;
				m_bWithMetadata = false;
			}

			virtual ~MongoSerializer() {}
		protected:
			virtual SerializerBase* GetAnother(const sprawl::String& /*data*/) override { SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return nullptr; }
			//Anything binary (including BSON) doesn't work here, it makes Mongo freak out if an object is embedded as a key like this. So we'll embed JSON instead.
			virtual SerializerBase* GetAnother() override { return new JSONSerializer(false); }
		};

		class MongoDeserializer : public MongoSerializerBase, public Deserializer
		{
		public:
			//Reset everything to original state.
			virtual void Reset() override { MongoSerializerBase::Reset(); Data(m_dataStr); }
			using Deserializer::operator%;
			using Deserializer::IsLoading;

			using MongoSerializerBase::serialize;
			using MongoSerializerBase::IsBinary;
			using MongoSerializerBase::IsMongoStream;
			using MongoSerializerBase::IsReplicable;
			using MongoSerializerBase::IsValid;
			using MongoSerializerBase::Str;
			using MongoSerializerBase::Data;
			using MongoSerializerBase::GetVersion;
			using MongoSerializerBase::SetVersion;
			using MongoSerializerBase::Size;

			using MongoSerializerBase::StartObject;
			using MongoSerializerBase::EndObject;
			using MongoSerializerBase::StartArray;
			using MongoSerializerBase::EndArray;
			using MongoSerializerBase::StartMap;
			using MongoSerializerBase::EndMap;
			using MongoSerializerBase::GetNextKey;
			using MongoSerializerBase::GetDeletedKeys;

			virtual SerializerBase& operator%(SerializationData<Deserializer>&& var) override
			{
				sprawl::String str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase& operator%(SerializationData<MongoDeserializer>&& var) override
			{
				sprawl::String str;
				*this % str;
				var.val.Data(str);
				return *this;
			}

			void Data(const mongo::BSONObj& o)
			{
				m_obj = o;
				m_bIsValid = true;
			}

			virtual void Data(const sprawl::String& str) override
			{
				if(str[0] == '{')
				{
					m_obj = mongo::fromjson( str.c_str() );
				}
				else
				{
					m_obj = mongo::BSONObj(str.c_str()).copy();
				}
				m_bIsValid = true;
			}

			virtual void Data(const char* data, size_t /*length*/) override
			{
				if(data[0] == '{')
				{
					m_obj = mongo::fromjson(data);
				}
				else
				{
					m_obj = mongo::BSONObj(data).copy();
				}
				m_bIsValid = true;
			}

			MongoDeserializer(const sprawl::String& data)
			{
				Data(data);
				m_version = m_obj["DataVersion"].Int();
			}

			MongoDeserializer(const sprawl::String& data, bool)
			{
				Data(data);
				m_bWithMetadata = false;
			}

			MongoDeserializer(const char* data, size_t length)
			{
				Data(data, length);
				m_version = m_obj["DataVersion"].Int();
			}

			MongoDeserializer(const char* data, size_t length, bool)
			{
				Data(data, length);
				m_bWithMetadata = false;
			}

			MongoDeserializer(const mongo::BSONObj& o)
			{
				Data(o);
				m_version = m_obj["DataVersion"].Int();
			}

			MongoDeserializer(const mongo::BSONObj& o, bool)
			{
				Data(o);
				m_bWithMetadata = false;
			}

			MongoDeserializer()
			{
				m_bWithMetadata = false;
				m_bIsValid = false;
			}

			MongoDeserializer(bool)
			{
				m_bWithMetadata = false;
				m_bIsValid = false;
			}

			virtual ~MongoDeserializer(){}
		private:
			sprawl::String m_dataStr;
		protected:
			//Anything binary (including BSON) doesn't work here, it makes Mongo freak out if an object is embedded as a key like this. So we'll embed JSON instead.
			virtual SerializerBase* GetAnother(const sprawl::String& data) override { return new JSONDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return nullptr; }
		};

		SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::OID>&& var);

		SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::BSONObj>&& var);

		SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::Date_t>&& var);

	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
