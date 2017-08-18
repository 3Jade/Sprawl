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

#include "Serializer.hpp"

namespace sprawl
{
	namespace serialization
	{
		class BinarySerializerBase : virtual public SerializerBase
		{
		public:
			static const int ms_headerSize = sizeof(int32_t)*3;

			virtual bool IsBinary() override { return true; }
			virtual size_t Size() override { return m_size; }

			typedef class BinarySerializer serializer_type;
			typedef class BinaryDeserializer deserializer_type;
			uint32_t ComputeChecksum();
			uint32_t GetChecksum() { return m_checksum; }
			virtual uint32_t GetVersion() override { return m_version; }
			virtual bool IsValid() override { return m_bIsValid; }
			virtual void SetVersion(uint32_t i) override;
			virtual ErrorState<void> Reset() override;
			using SerializerBase::Data;
			virtual const char* Data() override;
			virtual sprawl::String Str() override;
			bool More(){ return m_pos < m_size; }
			virtual bool Error() override { return m_bError; }
		protected:
			template<typename T>
			friend class ReplicableBase;
			char* m_data;
			uint32_t m_pos;
			uint32_t m_capacity;
			uint32_t m_checksum;
			bool m_checksum_stale;
			bool m_bInitialized;

			//Copied and pasted to avoid indirection with virtual inheritance
			uint32_t m_size;
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bWithMetadata;
			bool m_bError;

			uint32_t compute_checksum(const char* data, int len);

			using SerializerBase::serialize;
			void serialize_impl(void* var, const uint32_t bytes, bool /*PersistToDB*/);

		public:
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(long int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(long long int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB)  override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(short int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(char* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(float* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(double* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(long double* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(bool* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned long int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned long long int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned short int* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(unsigned char* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
				return ErrorState<void>();
			}

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(std::string* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override;

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> serialize(sprawl::String* var, const uint32_t bytes, sprawl::String const&, bool PersistToDB) override;
		protected:

			BinarySerializerBase();
			virtual ~BinarySerializerBase();
		private:
			BinarySerializerBase(SerializerBase const&);
			BinarySerializerBase& operator=(SerializerBase const&);
		};

		class BinarySerializer : public BinarySerializerBase, public Serializer
		{
		public:
			using Serializer::operator%;
			using Serializer::IsLoading;

			using BinarySerializerBase::serialize;
			using BinarySerializerBase::IsBinary;
			using BinarySerializerBase::IsMongoStream;
			using BinarySerializerBase::IsReplicable;
			using BinarySerializerBase::IsValid;
			using BinarySerializerBase::Str;
			using BinarySerializerBase::Data;
			using BinarySerializerBase::GetVersion;
			using BinarySerializerBase::SetVersion;
			using BinarySerializerBase::Reset;
			using BinarySerializerBase::Size;

			using BinarySerializerBase::StartObject;
			using BinarySerializerBase::EndObject;
			using BinarySerializerBase::StartArray;
			using BinarySerializerBase::EndArray;
			using BinarySerializerBase::StartMap;
			using BinarySerializerBase::EndMap;
			using BinarySerializerBase::GetNextKey;
			using BinarySerializerBase::GetDeletedKeys;

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<Serializer>&& var) override;

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<BinarySerializer>&& var) override;

			//Reserve the first sizeof(int32_t)*3 bytes of space to hold metadata (size, version, and checksum).
			BinarySerializer();

			BinarySerializer(bool);

			virtual ~BinarySerializer();
		protected:
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother(sprawl::String const& /*data*/) override;
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother() override;
		};

		class BinaryDeserializer : public BinarySerializerBase, public Deserializer
		{
		public:
			using Deserializer::operator%;
			using Deserializer::IsLoading;

			using BinarySerializerBase::serialize;
			using BinarySerializerBase::IsBinary;
			using BinarySerializerBase::IsMongoStream;
			using BinarySerializerBase::IsReplicable;
			using BinarySerializerBase::IsValid;
			using BinarySerializerBase::Str;
			using BinarySerializerBase::Data;
			using BinarySerializerBase::GetVersion;
			using BinarySerializerBase::SetVersion;
			using BinarySerializerBase::Reset;
			using BinarySerializerBase::Size;

			using BinarySerializerBase::StartObject;
			using BinarySerializerBase::EndObject;
			using BinarySerializerBase::StartArray;
			using BinarySerializerBase::EndArray;
			using BinarySerializerBase::StartMap;
			using BinarySerializerBase::EndMap;
			using BinarySerializerBase::GetNextKey;
			using BinarySerializerBase::GetDeletedKeys;

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<Deserializer>&& var) override;
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> operator%(SerializationData<BinaryDeserializer>&& var) override;

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> Data(sprawl::String const& str) override;

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<void> Data(const char* data, size_t length) override;

			BinaryDeserializer(sprawl::String const& data);

			BinaryDeserializer(const char* data, size_t length);

			BinaryDeserializer(sprawl::String const& data, bool);

			BinaryDeserializer(const char* data, size_t length, bool);

			BinaryDeserializer();

			BinaryDeserializer(bool);

			virtual ~BinaryDeserializer();

			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother(sprawl::String const& data) override;
			virtual SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> GetAnother() override;
		private:
		};
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
