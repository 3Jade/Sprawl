#include "Replicable.hpp"
#include "JSONSerializer.hpp"
#include "BinarySerializer.hpp"

namespace sprawl
{
	namespace serialization
	{
		ReplicationKey::ReplicationKey(sprawl::serialization::ReplicationKey const& other)
			: m_size(other.m_size)
		{
			memcpy( m_data, other.m_data, sizeof(int32_t) * other.m_size );
		}

		bool ReplicationKey::operator==(ReplicationKey const& other) const
		{
			if( m_size != other.m_size )
			{
				return false;
			}
			return (memcmp( m_data, other.m_data, sizeof(int32_t) * m_size ) == 0);
		}

		bool ReplicationKey::operator<(ReplicationKey const& other) const
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

		bool StartsWith(ReplicationKey const& x, ReplicationKey const& y)
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

		template class ReplicableSerializer<JSONSerializer>;
		template class ReplicableDeserializer<JSONDeserializer>;
		template class ReplicableSerializer<BinarySerializer>;
		template class ReplicableDeserializer<BinaryDeserializer>;
	}
}
