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

#include <string>
#include <cstring>
#include <vector>
#include <cstdint>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <memory>
#include <sstream>
#include "../string/String.hpp"
#include "../memory/PoolAllocator.hpp"
#include "../memory/StlWrapper.hpp"
#include "../common/errors.hpp"

namespace mongo {
	class OID;
	class BSONObj;
	struct Date_t;
}

namespace sprawl
{
	namespace serialization
	{
		class ex_invalid_data: public std::exception
		{
		public:
			const char* what() const throw ()
			{
				return "Data invalid";
			}
			~ex_invalid_data() throw ()
			{
			}
		};

		class ex_serializer_overflow: public std::exception
		{
		public:
			const char* what() const throw ()
			{
				return "Serialization overflow";
			}
			~ex_serializer_overflow() throw ()
			{
			}
		};

		template<typename T>
		class SerializationData
		{
		public:
			explicit SerializationData(T& a, sprawl::String const& b = "noname", bool c=true) : val(a), name(b), PersistToDB(c)
			{}
			explicit SerializationData(T&& a, sprawl::String const& b = "noname", bool c=true) : val(a), name(b), PersistToDB(c)
			{}
			T& operator*(){ return val; }
			T& val;
			sprawl::String name;
			bool PersistToDB;
		private:
			SerializationData<T> operator=(SerializationData<T>& other);
		};

		class BinaryData
		{
		public:
			explicit BinaryData(char* data, uint32_t dataLength, sprawl::String const& b = "noname", bool c=true)
				: val(data)
				, name(b)
				, size(dataLength)
				, PersistToDB(c)
			{}
			const char* operator*(){ return val; }
			char* val;
			sprawl::String name;
			uint32_t size;
			bool PersistToDB;
		private:
			BinaryData operator=(BinaryData& other);
		};

		inline BinaryData prepare_data(char* val, uint32_t dataLength, sprawl::String const& name = "noname", bool persist=true)
		{
			return BinaryData(val, dataLength, name, persist);
		}

		template<typename T>
		inline SerializationData<T> prepare_data(T& val, sprawl::String const& name = "noname", bool persist=true, typename std::enable_if<!std::is_enum<T>::value>::type* = 0)
		{
			return SerializationData<T>(val, name, persist);
		}

		template<typename T>
		inline SerializationData<T> prepare_data(T&& val, sprawl::String const& name = "noname", bool persist=true, typename std::enable_if<!std::is_reference<T>::value>::type* = 0, typename std::enable_if<!std::is_enum<T>::value>::type* = 0)
		{
			return SerializationData<T>(val, name, persist);
		}

		template<typename T, bool isEnum = std::is_enum<T>::value>
		struct UnderlyingTypeHelper
		{
			typedef T type;
		};

		template<typename T>
		struct UnderlyingTypeHelper<T, true>
		{
			typedef typename std::underlying_type<T>::type type;
		};

		template<typename T>
		inline SerializationData<typename UnderlyingTypeHelper<T>::type> prepare_data(T& val, sprawl::String const& name = "noname", bool persist=true, typename std::enable_if<std::is_enum<T>::value>::type* = 0)
		{
			return SerializationData<typename std::underlying_type<T>::type>((typename std::underlying_type<T>::type&)(val), name, persist);
		}

		template<typename T>
		inline SerializationData<typename UnderlyingTypeHelper<T>::type> prepare_data(T&& val, sprawl::String const& name = "noname", bool persist=true, typename std::enable_if<!std::is_reference<T>::value>::type* = 0, typename std::enable_if<std::is_enum<T>::value>::type* = 0)
		{
			return SerializationData<typename std::underlying_type<T>::type>((typename std::underlying_type<T>::type&&)(val), name, persist);
		}

		#define NAME_PROPERTY(var) sprawl::serialization::prepare_data(var, sprawl::StringLiteral(#var))
		#define TRANSIENT_NAME_PROPERTY(var) sprawl::serialization::prepare_data(var, sprawl::StringLiteral(#var), false)
		#define BINARY_NAME_PROPERTY(var, length) sprawl::serialization::prepare_data(var, length, sprawl::StringLiteral(#var), true)
		#define TRANSIENT_BINARY_NAME_PROPERTY(var, length) sprawl::serialization::prepare_data(var, length, sprawl::StringLiteral(#var), false)

		class SerializerBase
		{
		public:
			typedef std::unordered_set<sprawl::String, std::hash<sprawl::String>, std::equal_to<sprawl::String>, sprawl::memory::StlWrapper<sprawl::String>> StringSet;
			virtual bool IsLoading() = 0;
			bool IsSaving() { return !IsLoading(); }
			virtual bool IsBinary() { return false; }
			virtual bool IsReplicable() { return false; }
			virtual bool IsMongoStream() { return false; }

			virtual uint32_t GetVersion() = 0;
			virtual void SetVersion(uint32_t i) = 0;
			virtual void Reset() { }

			virtual bool IsValid() = 0;
			virtual bool Error() = 0;
			virtual size_t Size() = 0;

			SerializerBase& operator%(SerializationData<unsigned int>&& var);
			SerializerBase& operator%(SerializationData<unsigned long int>&& var);
			SerializerBase& operator%(SerializationData<unsigned char>&& var);
			SerializerBase& operator%(SerializationData<unsigned char* >&& var);
			SerializerBase& operator%(SerializationData<bool>&& var);
			SerializerBase& operator%(SerializationData<std::vector<bool>::reference>&& var);
			SerializerBase& operator%(SerializationData<int>&& var);
			SerializerBase& operator%(SerializationData<long int>&& var);
			SerializerBase& operator%(SerializationData<long long int>&& var);
			SerializerBase& operator%(SerializationData<unsigned long long int>&& var);
			SerializerBase& operator%(SerializationData<unsigned short int>&& var);
			SerializerBase& operator%(SerializationData<long double>&& var);
			SerializerBase& operator%(SerializationData<short int>&& var);
			SerializerBase& operator%(SerializationData<float>&& var);
			SerializerBase& operator%(SerializationData<double>&& var);
			SerializerBase& operator%(SerializationData<char>&& var);
			SerializerBase& operator%(SerializationData<char* >&& var);
			virtual SerializerBase& operator%(BinaryData&& var);

			template< std::size_t N >
			SerializerBase& operator%(SerializationData<unsigned int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<unsigned long int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<unsigned long long int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<long long int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<unsigned short int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<short int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<long double [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<bool [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<long int [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<float [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase& operator%(SerializationData<double [N]>&& var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}

			template< typename T, std::size_t N, std::size_t M>
			SerializerBase& operator%(SerializationData<T [N][M]>&& var)
			{
				uint32_t size;
				StartArray(var.name, size, var.PersistToDB);
				for(int i = 0; i < N; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
				return *this;
			}

			template< typename T, std::size_t N, std::size_t M, std::size_t O>
			SerializerBase& operator%(SerializationData<T [N][M][O]>&& var)
			{
				uint32_t size;
				StartArray(var.name, size, var.PersistToDB);
				for(int i = 0; i < N; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
				return *this;
			}

			template< typename T, std::size_t N, std::size_t M, std::size_t O, std::size_t P>
			SerializerBase& operator%(SerializationData<T [N][M][O][P]>&& var)
			{
				uint32_t size;
				StartArray(var.name, size, var.PersistToDB);
				for(int i = 0; i < N; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
				return *this;
			}

			SerializerBase& operator%(SerializationData<std::string>&& var);
			SerializerBase& operator%(SerializationData<sprawl::String>&& var);

		private:
			//Optimized vector implementations for simple types
			template<typename T>
			void VectorSerialize(SerializationData<std::vector<T>>& var, std::true_type)
			{
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					var.val.resize(size);
				}
				if(IsBinary())
				{
					serialize(&var.val[0], size * sizeof(T), var.name, var.PersistToDB);
				}
				else
				{
					for(uint32_t i=0; i<size; i++)
					{
						*this % prepare_data(var.val[i], var.name, var.PersistToDB);
					}
				}
				EndArray();
			}

			template<typename T>
			void VectorSerialize(SerializationData<std::vector<T>>& var, std::false_type)
			{
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					var.val.resize(size);
				}
				for(uint32_t i=0; i<size; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
			}

			//Except that bool has its own weird behaviors... so it has to be treated like the other kind.
			void VectorSerialize(SerializationData<std::vector<bool>>& var, std::true_type)
			{
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					var.val.resize(size);
				}
				for(uint32_t i=0; i<size; i++)
				{
					bool b = var.val[i];
					*this % prepare_data(b, var.name, var.PersistToDB);
					if(IsLoading())
					{
						var.val[i] = b;
					}
				}
				EndArray();
			}

		public:
			template<typename T>
			SerializerBase& operator%(SerializationData<std::vector<T>>&& var)
			{
				VectorSerialize(var, std::is_integral<T>());
				return *this;
			}

			template<typename key_type, typename val_type, typename comp, typename alloc>
			SerializerBase& operator%(SerializationData<std::map<key_type, val_type, comp, alloc>>&& var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						StringSet deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							key_type k;
							this->OneOff(const_cast<sprawl::String&>(key), k);
							var.val.erase(k);
						}
					}
					else
					{
						var.val.clear();
					}
				}
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				uint32_t calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(uint32_t i=0; i<size; i++)
					{
						key_type k;
						val_type v;
						if(IsBinary())
						{
							(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
							var.val[k] = v;
						}
						else
						{
							sprawl::String key = GetNextKey();
							this->OneOff(key, k);
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[k] = v;
						}
					}
				}
				else
				{
					for(auto& kvp : var.val)
					{
						if(IsBinary())
						{
							key_type name = kvp.first;
							(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(kvp.second, var.name, var.PersistToDB);
						}
						else
						{
							sprawl::String s;
							key_type k = kvp.first;
							this->OneOff(s, k);
							(*this) % prepare_data(kvp.second, s, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			template<typename key_type, typename val_type, typename hash, typename eq, typename alloc>
			SerializerBase& operator%(SerializationData<std::unordered_map<key_type, val_type, hash, eq, alloc>>&& var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						StringSet deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							key_type k;
							this->OneOff(const_cast<sprawl::String&>(key), k);
							var.val.erase(k);
						}
					}
					else
					{
						var.val.clear();
					}
				}
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				uint32_t calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(uint32_t i=0; i<size; i++)
					{
						key_type k;
						val_type v;
						if(IsBinary())
						{
							(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
							var.val[k] = v;
						}
						else
						{
							sprawl::String key = GetNextKey();
							this->OneOff(key, k);
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[k] = v;
						}
					}
				}
				else
				{
					for(auto& kvp : var.val)
					{
						if(IsBinary())
						{
							key_type name = kvp.first;
							(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(kvp.second, var.name, var.PersistToDB);
						}
						else
						{
							sprawl::String s;
							key_type k = kvp.first;
							this->OneOff(s, k);
							(*this) % prepare_data(kvp.second, s, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			template<typename key_type, typename val_type>
			SerializerBase& operator%(SerializationData<std::pair<key_type, val_type>>&& var)
			{
				if(IsLoading())
				{
					key_type k;
					val_type v;
					if(IsBinary())
					{
						(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
						var.val = std::make_pair(k, v);
					}
					else
					{
						sprawl::String key = GetNextKey();
						this->OneOff(key, k);
						(*this) % prepare_data(v, key, var.PersistToDB);
						var.val = std::make_pair(k, v);
					}
				}
				else
				{
					if(IsBinary())
					{
						key_type name = var.val.first;
						(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(var.val.second, var.name, var.PersistToDB);
					}
					else
					{
						sprawl::String s;
						key_type k = var.val.first;
						this->OneOff(s, k);
						(*this) % prepare_data(var.val.second, s, var.PersistToDB);
					}
				}
				return *this;
			}

			template<typename val_type, typename comp, typename alloc>
			SerializerBase& operator%(SerializationData<std::map<std::string, val_type, comp, alloc>>&& var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						StringSet deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							var.val.erase(key.toStdString());
						}
					}
					else
					{
						var.val.clear();
					}
				}
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				uint32_t calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(uint32_t i=0; i<size; i++)
					{
						std::string k;
						val_type v;
						if(IsBinary())
						{
							(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
							var.val[k] = v;
						}
						else
						{
							sprawl::String key = GetNextKey();
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[key.toStdString()] = v;
						}
					}
				}
				else
				{
					for(auto& kvp : var.val)
					{
						if(IsBinary())
						{
							std::string name = kvp.first;
							(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(kvp.second, var.name, var.PersistToDB);
						}
						else
						{
							(*this) % prepare_data(kvp.second, kvp.first, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			template<typename val_type, typename comp, typename alloc>
			SerializerBase& operator%(SerializationData<std::map<sprawl::String, val_type, comp, alloc>>&& var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						StringSet deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							var.val.erase(key.toStdString());
						}
					}
					else
					{
						var.val.clear();
					}
				}
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				uint32_t calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(uint32_t i=0; i<size; i++)
					{
						sprawl::String k;
						val_type v;
						if(IsBinary())
						{
							(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
							var.val[k] = v;
						}
						else
						{
							sprawl::String key = GetNextKey();
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[key.toStdString()] = v;
						}
					}
				}
				else
				{
					for(auto& kvp : var.val)
					{
						if(IsBinary())
						{
							sprawl::String name = kvp.first;
							(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(kvp.second, var.name, var.PersistToDB);
						}
						else
						{
							(*this) % prepare_data(kvp.second, kvp.first, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			//Specialization for performance.
			template<typename val_type, typename hash, typename eq, typename alloc>
			SerializerBase& operator%(SerializationData<std::unordered_map<std::string, val_type, hash, eq, alloc>>&& var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						StringSet deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							var.val.erase(key.toStdString());
						}
					}
					else
					{
						var.val.clear();
					}
				}
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				uint32_t calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(uint32_t i=0; i<size; i++)
					{
						std::string k;
						val_type v;
						if(IsBinary())
						{
							(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
							var.val[k] = v;
						}
						else
						{
							sprawl::String key = GetNextKey();
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[key.toStdString()] = v;
						}
					}
				}
				else
				{
					for(auto& kvp : var.val)
					{
						if(IsBinary())
						{
							std::string name = kvp.first;
							(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(kvp.second, var.name, var.PersistToDB);
						}
						else
						{
							(*this) % prepare_data(kvp.second, kvp.first, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			//Specialization for performance.
			template<typename val_type, typename hash, typename eq, typename alloc>
			SerializerBase& operator%(SerializationData<std::unordered_map<sprawl::String, val_type, hash, eq, alloc>>&& var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						StringSet deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							var.val.erase(key.toStdString());
						}
					}
					else
					{
						var.val.clear();
					}
				}
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				uint32_t calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(uint32_t i=0; i<size; i++)
					{
						sprawl::String k;
						val_type v;
						if(IsBinary())
						{
							(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
							var.val[k] = v;
						}
						else
						{
							sprawl::String key = GetNextKey();
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[key.toStdString()] = v;
						}
					}
				}
				else
				{
					for(auto& kvp : var.val)
					{
						if(IsBinary())
						{
							sprawl::String name = kvp.first;
							(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(kvp.second, var.name, var.PersistToDB);
						}
						else
						{
							(*this) % prepare_data(kvp.second, kvp.first, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			template<typename val_type>
			SerializerBase& operator%(SerializationData<std::pair<std::string, val_type>>&& var)
			{
				if(IsLoading())
				{
					std::string k;
					val_type v;
					if(IsBinary())
					{
						(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
						var.val = std::make_pair(k, v);
					}
					else
					{
						sprawl::String key = GetNextKey();
						(*this) % prepare_data(v, key, var.PersistToDB);
						var.val = std::make_pair(k, v);
					}
				}
				else
				{
					if(IsBinary())
					{
						std::string name = var.val.first;
						(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(var.val.second, var.name, var.PersistToDB);
					}
					else
					{
						(*this) % prepare_data(var.val.second, var.val.first, var.PersistToDB);
					}
				}
				return *this;
			}

			template<typename val_type>
			SerializerBase& operator%(SerializationData<std::pair<sprawl::String, val_type>>&& var)
			{
				if(IsLoading())
				{
					sprawl::String k;
					val_type v;
					if(IsBinary())
					{
						(*this) % prepare_data(k, var.name, var.PersistToDB) % prepare_data(v, var.name, var.PersistToDB);
						var.val = std::make_pair(k, v);
					}
					else
					{
						sprawl::String key = GetNextKey();
						(*this) % prepare_data(v, key, var.PersistToDB);
						var.val = std::make_pair(k, v);
					}
				}
				else
				{
					if(IsBinary())
					{
						sprawl::String name = var.val.first;
						(*this) % prepare_data(name, var.name, var.PersistToDB) % prepare_data(var.val.second, var.name, var.PersistToDB);
					}
					else
					{
						(*this) % prepare_data(var.val.second, var.val.first, var.PersistToDB);
					}
				}
				return *this;
			}

			template<typename T, typename comp, typename alloc>
			SerializerBase& operator%(SerializationData<std::set<T, comp, alloc>>&& var)
			{
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					for(uint32_t i=0; i<size; i++)
					{
						T val;
						*this % prepare_data(val, var.name, var.PersistToDB);
						var.val.insert(val);
					}
				}
				else
				{
					for(T val : var.val)
					{
						*this % prepare_data(val, var.name, var.PersistToDB);
					}
				}
				EndArray();
				return *this;
			}

			template<typename T, typename comp, typename alloc>
			SerializerBase& operator%(SerializationData<std::unordered_set<T, comp, alloc>>&& var)
			{
				uint32_t size = (uint32_t)var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					for(uint32_t i=0; i<size; i++)
					{
						T val;
						*this % prepare_data(val, var.name, var.PersistToDB);
						var.val.insert(val);
					}
				}
				else
				{
					for(T val : var.val)
					{
						*this % prepare_data(val, var.name, var.PersistToDB);
					}
				}
				EndArray();
				return *this;
			}
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
			virtual SerializerBase& operator%(SerializationData<class Serializer> &&);
			virtual SerializerBase& operator%(SerializationData<class Deserializer> &&);
			virtual SerializerBase& operator%(SerializationData<class BinarySerializer> &&);
			virtual SerializerBase& operator%(SerializationData<class BinaryDeserializer> &&);
			virtual SerializerBase& operator%(SerializationData<class JSONSerializer> &&);
			virtual SerializerBase& operator%(SerializationData<class JSONDeserializer> &&);
			virtual SerializerBase& operator%(SerializationData<class YAMLSerializer> &&);
			virtual SerializerBase& operator%(SerializationData<class YAMLDeserializer> &&);
			virtual SerializerBase& operator%(SerializationData<class MongoSerializer> &&);
			virtual SerializerBase& operator%(SerializationData<class MongoDeserializer> &&);
			virtual void StartArray(sprawl::String const& , uint32_t&, bool = true);
			virtual void EndArray();
			virtual uint32_t StartObject(sprawl::String const& , bool = true);
			virtual void EndObject();
			virtual uint32_t StartMap(sprawl::String const& s, bool b = true);
			virtual void EndMap();
			virtual sprawl::String GetNextKey();

			virtual StringSet GetDeletedKeys(sprawl::String const&);

			virtual void serialize(int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(long long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(short int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(char* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(float* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(double* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(long double* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(bool* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(unsigned int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(unsigned long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(unsigned long long int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(unsigned short int* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(unsigned char* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(std::string* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;
			virtual void serialize(sprawl::String* var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB) = 0;

		protected:
			virtual SerializerBase* GetAnother(sprawl::String const& /*data*/);
			virtual SerializerBase* GetAnother();
#ifdef _WIN32
#pragma warning(pop)
#endif
			template<typename T>
			sprawl::String to_string( T& val, std::true_type)
			{
				sprawl::StringBuilder b;
				b << val;
				return b.Str();
			}

			template<typename T>
			sprawl::String to_string( T& val, std::false_type)
			{
				SerializerBase* s = this->GetAnother();
				*s % prepare_data( val, "key", false );
				sprawl::String data = s->Str();
				delete s;
				return std::move(data);
			}

			template<typename T>
			void get_from_string( sprawl::String const& data, T& val, std::false_type )
			{
				SerializerBase* d = this->GetAnother(data);
				*d % prepare_data( val, "key", false );
				delete d;
			}

			template<typename T>
			void get_from_string( sprawl::String const& data, T& val, std::true_type )
			{
				///TODO: Get rid of std::stringstream here
				std::stringstream s(data.toStdString());
				s >> val;
			}

			template<typename T>
			void OneOff( sprawl::String& data, T& val )
			{
				if(IsLoading())
				{
					if(IsBinary())
					{
						get_from_string(data, val, std::false_type());
					}
					else
					{
						get_from_string(data, val, typename std::is_arithmetic<T>::type());
					}
				}
				else
				{
					if(IsBinary())
					{
						data = to_string(val, std::false_type());
					}
					else
					{
						data = to_string(val, typename std::is_arithmetic<T>::type());
					}
				}
			}

		public:

			template<typename T>
			SerializerBase& operator%(T&& var)
			{
				return *this % prepare_data(var, "noname", false);
			}

			template<typename T>
			SerializerBase& operator%(SerializationData<T>&& var)
			{
				StartObject(var.name, var.PersistToDB);
				var.val.Serialize(*this);
				EndObject();
				return *this;
			}

			template<typename T>
			SerializerBase& operator%(SerializationData<T*>&& var)
			{
				StartObject(var.name, var.PersistToDB);
				var.val->Serialize(*this);
				EndObject();
				return *this;
			}

			template<typename T>
			SerializerBase& operator%(SerializationData<std::shared_ptr<T>>&& var)
			{
				bool hasValue = (var.val != nullptr);
				*this % prepare_data(hasValue, var.name+"_exists", var.PersistToDB);
				if(hasValue)
				{
					if(IsLoading() && !var.val)
					{
						var.val.reset( new T() );
					}
					*this % prepare_data(*var.val, var.name, var.PersistToDB);
				}
				else if(IsLoading())
				{
					var.val.reset();
				}
				return *this;
			}

			virtual const char* Data() = 0;
			virtual void Data(sprawl::String const&){}
			virtual sprawl::String Str() = 0;
			virtual ~SerializerBase(){}
		protected:

			template<typename T>
			void serialize(T& var, const uint32_t bytes, sprawl::String const& name, bool PersistToDB)
			{
				serialize(&var, bytes, name, PersistToDB);
			}

			//Usually does nothing, but has to be here for mongo serializer to work properly with OIDs.
			friend SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::OID>&& var);
			friend SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::BSONObj>&& var);
			friend SerializerBase& operator%(SerializerBase& s, SerializationData<mongo::Date_t>&& var);
			virtual void serialize(mongo::OID* /*var*/, sprawl::String const& /*name*/, bool /*PersistToDB*/) {}
			virtual void serialize(mongo::BSONObj* /*var*/, sprawl::String const& /*name*/, bool /*PersistToDB*/) {}
			virtual void serialize(mongo::Date_t* /*var*/, sprawl::String const& /*name*/, bool /*PersistToDB*/) {}

			SerializerBase() {}
		private:
			SerializerBase(SerializerBase const&);
			SerializerBase& operator=(SerializerBase const&);
		};

		class Serializer : virtual public SerializerBase
		{
		public:
			using SerializerBase::operator%;
			template <typename T>
			SerializerBase& operator%(SerializationData<T const>&& var)
			{
				T cVar = var.val;
				return *this % prepare_data(cVar, var.name, var.PersistToDB);
			}

#ifndef _WIN32
			template<typename T>
			SerializerBase& operator%(T&& var)
			{
			  return *this % prepare_data(var, "noname", true);
			}

			template<typename T>
			SerializerBase& operator%(SerializationData<T>&& var)
			{
			  StartObject(var.name, var.PersistToDB);
			  var.val.Serialize(*this);
			  EndObject();
			  return *this;
			}
#endif

			virtual ~Serializer();
			virtual bool IsLoading() override;
		protected:
			Serializer();
		};

		class Deserializer : virtual public SerializerBase
		{
		public:
			using SerializerBase::operator%;
			using SerializerBase::Data;
			virtual void Data(sprawl::String const& str) = 0;
			virtual void Data(const char* data, size_t length) = 0;
			virtual ~Deserializer();
			virtual bool IsLoading() override;
		protected:
			Deserializer();
		};
	}
}
