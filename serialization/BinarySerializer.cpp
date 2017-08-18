#include "BinarySerializer.hpp"

namespace sprawl
{
	namespace serialization
	{
		uint32_t BinarySerializerBase::ComputeChecksum()
		{
			if(m_bWithMetadata)
				return compute_checksum( m_data + ms_headerSize, m_size-ms_headerSize);
			else
				return compute_checksum( m_data, m_size );
		}

		void BinarySerializerBase::SetVersion(uint32_t i)
		{
			m_version = i;
			if(IsSaving() && m_bWithMetadata)
			{
				//Update version metadata
				memcpy(m_data + sizeof(int32_t), &i, sizeof(i));
			}
		}

		ErrorState<void> BinarySerializerBase::Reset()
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
			return ErrorState<void>();
		}

		const char*BinarySerializerBase::Data()
		{
			if(m_checksum_stale && m_bWithMetadata)
			{
				m_checksum = compute_checksum( m_data + ms_headerSize, m_size-ms_headerSize);
				memcpy( m_data + sizeof(int32_t)*2, &m_checksum, sizeof(int32_t) );
				m_checksum_stale = false;
			}
			return m_data;
		}

		String BinarySerializerBase::Str()
		{
			if(m_checksum_stale && m_bWithMetadata)
			{
				m_checksum = compute_checksum( m_data + ms_headerSize, m_size-ms_headerSize);
				memcpy( m_data + sizeof(int32_t)*2, &m_checksum, sizeof(int32_t) );
				m_checksum_stale = false;
			}
			return sprawl::String(m_data, m_size); }

		uint32_t BinarySerializerBase::compute_checksum(const char* data, int len)
		{
			uint32_t hash = 0;

			for(int i=0; i<len; i++)
			{
				hash = ((hash << 5) + hash) ^ data[i];
			}

			return hash;
		}

		void BinarySerializerBase::serialize_impl(void* var, const uint32_t bytes, bool)
		{
			if(!m_bIsValid)
			{
				m_bError = true;
			}
			if(m_bError)
			{
				return;
			}
			if(IsSaving())
			{
				if(m_pos + bytes >= m_capacity*0.75)
				{
					int oldcap = m_capacity;
					uint32_t newPos = m_pos + bytes;
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
					m_bError = true;
				}
				memcpy( var, m_data + m_pos, bytes );
			}

			m_pos += (uint32_t)bytes;
		}

		BinarySerializerBase::BinarySerializerBase()
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
			, m_bError(false)
		{}

		BinarySerializerBase::~BinarySerializerBase()
		{
			if(m_data != NULL)
			{
				delete[] m_data;
				m_data = NULL;
			}
		}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<void> BinarySerializerBase::serialize(String* var, const uint32_t bytes, String const&, bool PersistToDB)
		{
			if(IsLoading())
			{
				char* data = new char[bytes];
				serialize_impl(data, bytes, PersistToDB);
				*var = sprawl::String(data, bytes);
				delete[] data;
			}
			else
			{
				serialize_impl(const_cast<char*>(var->c_str()), bytes, PersistToDB);
			}
			return ErrorState<void>();
		}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<void> BinarySerializerBase::serialize(std::string* var, const uint32_t bytes, String const&, bool PersistToDB)
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
			return ErrorState<void>();
		}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> BinarySerializer::operator%(SerializationData<Serializer>&& var)
		{
			sprawl::String str = var.val.Str();
			SPRAWL_RETHROW(*this % prepare_data(str, var.name, var.PersistToDB));
			return *this;
		}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> BinarySerializer::operator%(SerializationData<BinarySerializer>&& var)
		{
			sprawl::String str = var.val.Str();
			SPRAWL_RETHROW(*this % prepare_data(str, var.name, var.PersistToDB));
			return *this;
		}

		BinarySerializer::BinarySerializer(bool)
			: BinarySerializerBase()
			, Serializer()
		{
			m_capacity = 256;
			m_bWithMetadata = false;
			m_data = new char[m_capacity];
			m_bInitialized = true;
		}

		BinarySerializer::~BinarySerializer() {}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> BinarySerializer::GetAnother() { return new BinarySerializer(false); }

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> BinarySerializer::GetAnother(String const&) { SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return nullptr; }

		BinarySerializer::BinarySerializer()
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

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> BinaryDeserializer::operator%(SerializationData<Deserializer>&& var)
		{
			sprawl::String str;
			SPRAWL_RETHROW(*this % str);
			SPRAWL_RETHROW(var.val.Data(str));
			return *this;
		}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase&> BinaryDeserializer::operator%(SerializationData<BinaryDeserializer>&& var)
		{
			sprawl::String str;
			SPRAWL_RETHROW(*this % str);
			SPRAWL_RETHROW(var.val.Data(str));
			return *this;
		}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<void> BinaryDeserializer::Data(const char* data, size_t length)
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
				SPRAWL_RETHROW(serialize(m_size, sizeof(int32_t), "", false));
				if(length < m_size)
				{
					m_bIsValid = false;
				}
				else
				{
					SPRAWL_RETHROW(serialize(m_version, sizeof(int32_t), "", false));
					SPRAWL_RETHROW(serialize(m_checksum, sizeof(int32_t), "", false));
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
			return ErrorState<void>();
		}

		BinaryDeserializer::BinaryDeserializer(bool)
			: BinarySerializerBase()
			, Deserializer()
		{
			m_bWithMetadata = false;
			m_bIsValid = false;
		}

		BinaryDeserializer::~BinaryDeserializer(){}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> BinaryDeserializer::GetAnother() { SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return nullptr; }

		SPRAWL_WARN_UNUSED_RESULT ErrorState<SerializerBase*> BinaryDeserializer::GetAnother(String const& data) { return new BinaryDeserializer(data, false); }

		BinaryDeserializer::BinaryDeserializer()
			: BinarySerializerBase()
			, Deserializer()
		{
			m_bIsValid = false;
		}

		BinaryDeserializer::BinaryDeserializer(const char* data, size_t length, bool)
			: BinarySerializerBase()
			, Deserializer()
		{
			m_bWithMetadata = false;
			SPRAWL_ACTION_ON_ERROR(Data(data, length), m_bIsValid = false);
		}

		BinaryDeserializer::BinaryDeserializer(String const& data, bool)
			: BinarySerializerBase()
			, Deserializer()
		{
			m_bWithMetadata = false;
			SPRAWL_ACTION_ON_ERROR(Data(data), m_bIsValid = false);
		}

		BinaryDeserializer::BinaryDeserializer(const char* data, size_t length)
			: BinarySerializerBase()
			, Deserializer()
		{
			SPRAWL_ACTION_ON_ERROR(Data(data, length), m_bIsValid = false);
		}

		BinaryDeserializer::BinaryDeserializer(String const& data)
			: BinarySerializerBase()
			, Deserializer()
		{
			SPRAWL_ACTION_ON_ERROR(Data(data), m_bIsValid = false);
		}

		SPRAWL_WARN_UNUSED_RESULT ErrorState<void> BinaryDeserializer::Data(String const& str)
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
				SPRAWL_RETHROW(serialize(m_size, sizeof(int32_t), "", false));
				SPRAWL_RETHROW(serialize(m_version, sizeof(int32_t), "", false));
				SPRAWL_RETHROW(serialize(m_checksum, sizeof(int32_t), "", false));
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
				m_size = (uint32_t)str.length();
				m_pos = 0;
				m_data = new char[m_size];
				memcpy(m_data, str.c_str(), m_size);
			}
			m_bInitialized = true;
			return ErrorState<void>();
		}




	}
}
