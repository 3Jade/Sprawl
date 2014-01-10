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
			uint32_t ComputeChecksum()
			{
				if(m_bWithMetadata)
					return compute_checksum( m_data + ms_headerSize, m_size-ms_headerSize);
				else
					return compute_checksum( m_data, m_size );
			}
			uint32_t GetChecksum() { return m_checksum; }
			virtual uint32_t GetVersion() override { return m_version; }
			virtual bool IsValid() override { return m_bIsValid; }
			virtual void SetVersion(uint32_t i) override
			{
				m_version = i;
				if(IsSaving() && m_bWithMetadata)
				{
					//Update version metadata
					memcpy(m_data + sizeof(int32_t), &i, sizeof(i));
				}
			}
			virtual void Reset() override
			{
				if(IsSaving())
				{
					m_capacity = 256;
					m_data = new char[m_capacity];
					if(m_bWithMetadata)
					{
						m_pos = ms_headerSize;
						m_size = m_pos;
						//Zero out the entire contents
						memset(m_data, 0, m_capacity);

						m_checksum = 0;
						m_checksum_stale = false;

						//Size, first int
						memcpy( m_data, &m_size, sizeof(int32_t) );
						//Version, second int
						memcpy(m_data + sizeof(int32_t), &m_version, sizeof(m_version));
						//Checksum, third int
						memcpy( m_data + sizeof(int32_t)*2, &m_checksum, sizeof(int32_t) );
					}
					else
					{
						m_pos = 0;
						m_size = 0;
						memset(m_data, 0, m_capacity);
					}
				}
				else
				{
					if(m_bWithMetadata)
						m_pos = ms_headerSize;
					else
						m_pos = 0;
				}
			}
			using SerializerBase::Data;
			virtual const char* Data() override
			{
				if(m_checksum_stale && m_bWithMetadata)
				{
					m_checksum = compute_checksum( m_data + ms_headerSize, m_size-ms_headerSize);
					memcpy( m_data + sizeof(int32_t)*2, &m_checksum, sizeof(int32_t) );
					m_checksum_stale = false;
				}
				return m_data;
			}
			virtual std::string Str() override
			{
				if(m_checksum_stale && m_bWithMetadata)
				{
					m_checksum = compute_checksum( m_data + ms_headerSize, m_size-ms_headerSize);
					memcpy( m_data + sizeof(int32_t)*2, &m_checksum, sizeof(int32_t) );
					m_checksum_stale = false;
				}
				return std::string(m_data, m_size); }
			bool More(){ return m_pos < m_size; }
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

			uint32_t compute_checksum(const char* data, int len)
			{
				uint32_t hash = 0;

				for(int i=0; i<len; i++)
				{
					hash = ((hash << 5) + hash) ^ data[i];
				}

				return hash;
			}

			using SerializerBase::serialize;
			void serialize_impl(void* var, const size_t bytes, bool /*PersistToDB*/)
			{
				if(!m_bIsValid)
				{
					throw ex_invalid_data();
				}
				if(IsSaving())
				{
					if(m_pos + bytes >= m_capacity*0.75)
					{
						int oldcap = m_capacity;
						int newPos = m_pos + bytes;
						while(newPos >= m_capacity*0.75)
						{
							m_capacity = m_capacity * 2 + 1;
						}
						char* newdata = new char[m_capacity];
						memcpy(newdata, m_data, oldcap);
						delete[] m_data;
						m_data = newdata;
					}
					memcpy( m_data + m_pos, var, bytes );

					m_size += (uint32_t)bytes;
					if(m_bWithMetadata)
					{
						memcpy( m_data, &m_size, sizeof(int32_t) );
						m_checksum_stale = true;
					}
				}
				else
				{
					if(m_bInitialized && m_pos + bytes > m_size)
					{
						throw ex_serializer_overflow();
					}
					memcpy( var, m_data + m_pos, bytes );
				}

				m_pos += (uint32_t)bytes;
			}
			virtual void serialize(int* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(long int* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(long long int* var, const size_t bytes, const std::string&, bool PersistToDB)  override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(short int* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(char* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(float* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(double* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(long double* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(bool* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(unsigned int* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(unsigned long int* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(unsigned long long int* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(unsigned short int* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(unsigned char* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				serialize_impl(reinterpret_cast<void*>(var), bytes, PersistToDB);
			}

			virtual void serialize(std::string* var, const size_t bytes, const std::string&, bool PersistToDB) override
			{
				if(IsLoading())
				{
					char* data = new char[bytes];
					serialize_impl(data, bytes, PersistToDB);
					*var = std::string(data, bytes);
					delete[] data;
				}
				else
				{
					serialize_impl(const_cast<char*>(var->c_str()), bytes, PersistToDB);
				}
			}

			BinarySerializerBase()
				: SerializerBase()
				, m_data(NULL)
				, m_pos(0)
				, m_capacity(0)
				, m_checksum(0)
				, m_checksum_stale(false)
				, m_bInitialized(false)
				, m_size(0)
				, m_version(0)
				, m_bIsValid(true)
				, m_bWithMetadata(true)
			{}
			virtual ~BinarySerializerBase()
			{
				if(m_data != NULL)
				{
					delete[] m_data;
					m_data = NULL;
				}
			}
		private:
			BinarySerializerBase(const SerializerBase&);
			BinarySerializerBase& operator=(const SerializerBase&);
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

			virtual SerializerBase& operator%(SerializationData<Serializer>&& var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			virtual SerializerBase& operator%(SerializationData<BinarySerializer>&& var) override
			{
				std::string str = var.val.Str();
				*this % prepare_data(str, var.name, var.PersistToDB);
				return *this;
			}

			//Reserve the first sizeof(int32_t)*3 bytes of space to hold metadata (size, version, and checksum).
			BinarySerializer()
				: BinarySerializerBase()
				, Serializer()
			{
				m_capacity = 256;
				m_data = new char[m_capacity];
				if(m_bWithMetadata)
				{
					m_pos = ms_headerSize;
					m_size = m_pos;
					//Make sure metadata info is empty.
					memset(m_data, 0, ms_headerSize);
					m_checksum = 0;

					//Size, first int
					memcpy( m_data, &m_size, sizeof(int32_t) );
					//Version, second int
					memcpy(m_data + sizeof(int32_t), &m_version, sizeof(m_version));
					//Checksum, third int
					memcpy( m_data + sizeof(int32_t)*2, &m_checksum, sizeof(int32_t) );
				}
				m_bInitialized = true;
			}

			BinarySerializer(bool)
				: BinarySerializerBase()
				, Serializer()
			{
				m_capacity = 256;
				m_bWithMetadata = false;
				m_data = new char[m_capacity];
				m_bInitialized = true;
			}

			virtual ~BinarySerializer() {};
		protected:
			virtual SerializerBase* GetAnother(const std::string& /*data*/) override { throw std::exception(); }
			virtual SerializerBase* GetAnother() override { return new BinarySerializer(false); }
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

			virtual SerializerBase& operator%(SerializationData<Deserializer>&& var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}
			virtual SerializerBase& operator%(SerializationData<BinaryDeserializer>&& var) override
			{
				std::string str;
				*this % str;
				var.val.Data(str);
				return *this;
			}

			virtual void Data(const std::string& str) override
			{
				if(m_data != NULL)
				{
					delete[] m_data;
				}
				m_bIsValid = true;
				if(m_bWithMetadata)
				{
					m_data = new char[ms_headerSize];
					m_pos = 0;
					memcpy(m_data, str.c_str(), ms_headerSize);
					serialize(m_size, sizeof(int32_t), "", false);
					serialize(m_version, sizeof(int32_t), "", false);
					serialize(m_checksum, sizeof(int32_t), "", false);
					delete[] m_data;
					m_pos = ms_headerSize;
					m_data = new char[m_size];
					memcpy(m_data, str.c_str(), m_size);
					unsigned int computed_checksum = compute_checksum(m_data + ms_headerSize, m_size-ms_headerSize);
					if(computed_checksum != m_checksum)
					{
						m_bIsValid = false;
					}
				}
				else
				{
					m_size = (uint32_t)str.size();
					m_pos = 0;
					m_data = new char[m_size];
					memcpy(m_data, str.c_str(), m_size);
				}
				m_bInitialized = true;
			}

			virtual void Data(const char* data, size_t length) override
			{
				if(m_data != NULL)
				{
					delete[] m_data;
				}
				m_bIsValid = true;
				if(m_bWithMetadata)
				{
					m_data = new char[ms_headerSize];
					m_pos = 0;
					memcpy(m_data, data, ms_headerSize);
					serialize(m_size, sizeof(int32_t), "", false);
					if(length < m_size)
					{
						m_bIsValid = false;
					}
					else
					{
						serialize(m_version, sizeof(int32_t), "", false);
						serialize(m_checksum, sizeof(int32_t), "", false);
						delete[] m_data;
						m_pos = ms_headerSize;
						m_data = new char[m_size];
						memcpy(m_data, data, m_size);
						unsigned int computed_checksum = compute_checksum(m_data + ms_headerSize, m_size-ms_headerSize);
						if(computed_checksum != m_checksum)
						{
							m_bIsValid = false;
						}
					}
				}
				else
				{
					m_size = (uint32_t)length;
					m_pos = 0;
					m_data = new char[m_size];
					memcpy(m_data, data, m_size);
				}
				m_bInitialized = true;
			}

			BinaryDeserializer(const std::string& data)
				: BinarySerializerBase()
				, Deserializer()
			{
				Data(data);
			}

			BinaryDeserializer(const char* data, size_t length)
				: BinarySerializerBase()
				, Deserializer()
			{
				Data(data, length);
			}

			BinaryDeserializer(const std::string& data, bool)
				: BinarySerializerBase()
				, Deserializer()
			{
				m_bWithMetadata = false;
				Data(data);
			}

			BinaryDeserializer(const char* data, size_t length, bool)
				: BinarySerializerBase()
				, Deserializer()
			{
				m_bWithMetadata = false;
				Data(data, length);
			}

			BinaryDeserializer()
				: BinarySerializerBase()
				, Deserializer()
			{
				m_bIsValid = false;
			}

			BinaryDeserializer(bool)
				: BinarySerializerBase()
				, Deserializer()
			{
				m_bWithMetadata = false;
				m_bIsValid = false;
			}

			virtual ~BinaryDeserializer(){}

			virtual SerializerBase* GetAnother(const std::string& data) override { return new BinaryDeserializer(data, false); }
			virtual SerializerBase* GetAnother() override { throw std::exception(); }
		private:
		};
	}
}

#ifdef _WIN32
	#pragma warning( pop )
#endif
