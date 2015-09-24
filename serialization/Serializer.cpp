#include "Serializer.hpp"

namespace sprawl
{
	namespace serialization
	{
		SerializerBase& SerializerBase::operator%(SerializationData<unsigned int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<unsigned long int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<unsigned char>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<unsigned char* >&& var)
		{
			uint32_t len = (uint32_t)strlen(reinterpret_cast<char*>(var.val));
			if(IsBinary())
			{
				*this % prepare_data(len, var.name, false);
			}
			serialize(var.val, len, var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<bool>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<std::vector<bool>::reference>&& var)
		{
			bool val = var.val;
			serialize(val, sizeof(bool), var.name, var.PersistToDB);
			return *this;
		}

		SerializerBase& SerializerBase::operator%(SerializationData<int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<long int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<long long int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<unsigned long long int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<unsigned short int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<long double>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<short int>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<float>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<double>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<char>&& var)
		{
			serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
			return *this;
		}
		SerializerBase& SerializerBase::operator%(SerializationData<char* >&& var)
		{
			uint32_t len = (uint32_t)strlen(var.val);
			if(IsBinary())
			{
				*this % prepare_data(len, var.name, false);
			}
			serialize(var.val, len, var.name, var.PersistToDB);
			return *this;
		}

		/*virtual*/ SerializerBase& SerializerBase::operator%(BinaryData&& var)
		{
			uint32_t len = var.size;
			serialize(var.val, len, var.name, var.PersistToDB);
			return *this;
		}

		sprawl::serialization::SerializerBase& SerializerBase::operator%(SerializationData<std::string>&& var)
		{
			uint32_t len = (uint32_t)var.val.length();
			if(IsBinary())
			{
				*this % prepare_data(len, var.name, false);
			}
			serialize(&var.val, len, var.name, var.PersistToDB);
			return *this;
		}

		sprawl::serialization::SerializerBase& SerializerBase::operator%(SerializationData<sprawl::String>&& var)
		{
			uint32_t len = (uint32_t)var.val.length();
			if(IsBinary())
			{
				*this % prepare_data(len, var.name, false);
			}
			serialize(&var.val, len, var.name, var.PersistToDB);
			return *this;
		}

		SerializerBase& SerializerBase::operator%(SerializationData<class Serializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class Deserializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class BinarySerializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class BinaryDeserializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class JSONSerializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class JSONDeserializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class YAMLSerializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class YAMLDeserializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class MongoSerializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		SerializerBase& SerializerBase::operator%(SerializationData<class MongoDeserializer> &&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return *this; }
		void SerializerBase::StartArray(sprawl::String const& , uint32_t&, bool){}
		void SerializerBase::EndArray(){}
		uint32_t SerializerBase::StartObject(sprawl::String const& , bool){ return 0; }
		void SerializerBase::EndObject(){}
		uint32_t SerializerBase::StartMap(sprawl::String const& s, bool b){ return StartObject(s, b); }
		void SerializerBase::EndMap(){ EndObject(); }
		sprawl::String SerializerBase::GetNextKey(){ return ""; }

		SerializerBase::StringSet SerializerBase::GetDeletedKeys(sprawl::String const&){ return StringSet(); }

		SerializerBase* SerializerBase::GetAnother(sprawl::String const&){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return nullptr; }

		SerializerBase* SerializerBase::GetAnother(){ SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD; return nullptr; }

		Serializer::~Serializer(){}

		bool Serializer::IsLoading() { return false; }

		Serializer::Serializer()
		{
		}

		Deserializer::~Deserializer(){}

		bool Deserializer::IsLoading() { return true; }

		Deserializer::Deserializer()
		{
		}


	}
}
