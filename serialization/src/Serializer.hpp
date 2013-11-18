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
			SerializationData(T &a, const std::string &b = "noname", bool c=true) : val(a), name(b), PersistToDB(c)
			{}
			SerializationData(T &&a, const std::string &b = "noname", bool c=true) : val(a), name(b), PersistToDB(c)
			{}
			T& operator*(){ return val; }
			T& val;
			std::string name;
			bool PersistToDB;
		};

		template<typename T>
		SerializationData<T> prepare_data(T &val, const std::string &name = "noname", bool persist=true)
		{
			return SerializationData<T>(val, name, persist);
		}

		template<typename T>
		SerializationData<T> prepare_data(T &&val, const std::string &name = "noname", bool persist=true)
		{
			return SerializationData<T>(val, name, persist);
		}

		#define NAME_PROPERTY(var) sprawl::serialization::prepare_data(var, #var)
		#define TRANSIENT_NAME_PROPERTY(var) sprawl::serialization::prepare_data(var, #var, false)

		class SerializerBase
		{
		public:
			bool IsLoading() { return m_bIsLoading; }
			bool IsSaving() { return !m_bIsLoading; }
			bool IsBinary() { return m_bIsBinary; }
			bool IsReplicable() { return m_bIsReplicable; }

			uint32_t GetVersion() { return m_version; }
			virtual void SetVersion(uint32_t i)
			{
				m_version = i;
			}
			virtual void Reset() { }

			bool IsValid() { return m_bIsValid; }
			virtual int Size() { return m_size; }
			SerializerBase &operator%(SerializationData<unsigned int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}

			SerializerBase &operator%(SerializationData<unsigned long int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<unsigned char> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<unsigned char *> &&var)
			{
				int len = strlen(reinterpret_cast<char*>(var.val));
				if(IsBinary())
				{
					*this % prepare_data(len, var.name+"_length", false);
				}
				serialize(var.val, len, var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<bool> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<std::vector<bool>::reference> &&var)
			{
				bool val = var.val;
				serialize(val, sizeof(bool), var.name, var.PersistToDB);
				return *this;
			}

			SerializerBase &operator%(SerializationData<int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<long int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<long long int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<unsigned long long int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<unsigned short int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<long double> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<short int> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<float> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<double> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<char> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			SerializerBase &operator%(SerializationData<char *> &&var)
			{
				int len = strlen(var.val);
				if(IsBinary())
				{
					*this % prepare_data(len, var.name+"_length", false);
				}
				serialize(var.val, len, var.name, var.PersistToDB);
				return *this;
			}

			template< std::size_t N >
			SerializerBase &operator%(SerializationData<unsigned int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<unsigned long int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<unsigned long long int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<long long int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<unsigned short int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<short int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<long double [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<bool [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<long int [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<float [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}
			template< std::size_t N >
			SerializerBase &operator%(SerializationData<double [N]> &&var)
			{
				serialize(var.val, sizeof(var.val), var.name, var.PersistToDB);
				return *this;
			}

			template< typename T, std::size_t N, std::size_t M>
			SerializerBase &operator%(SerializationData<T [N][M]> &&var)
			{
				size_t size;
				StartArray(var.name, size, var.PersistToDB);
				for(int i = 0; i < N; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
				return *this;
			}

			template< typename T, std::size_t N, std::size_t M, std::size_t O>
			SerializerBase &operator%(SerializationData<T [N][M][O]> &&var)
			{
				size_t size;
				StartArray(var.name, size, var.PersistToDB);
				for(int i = 0; i < N; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
				return *this;
			}

			template< typename T, std::size_t N, std::size_t M, std::size_t O, std::size_t P>
			SerializerBase &operator%(SerializationData<T [N][M][O][P]> &&var)
			{
				size_t size;
				StartArray(var.name, size, var.PersistToDB);
				for(int i = 0; i < N; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
				return *this;
			}

			SerializerBase &operator%(SerializationData<std::string> &&var)
			{
				unsigned int len = var.val.length();
				if(IsBinary())
				{
					*this % prepare_data(len, var.name+"_length", false);
				}
				serialize(&var.val, len, var.name, var.PersistToDB);
				return *this;
			}

		private:
			//Optimized vector implementations for simple types
			template<typename T>
			void VectorSerialize(SerializationData<std::vector<T>> &var, std::true_type)
			{
				size_t size = var.val.size();
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
					for(size_t i=0; i<size; i++)
					{
						*this % prepare_data(var.val[i], var.name, var.PersistToDB);
					}
				}
				EndArray();
			}

			template<typename T>
			void VectorSerialize(SerializationData<std::vector<T>> &var, std::false_type)
			{
				size_t size = var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					var.val.resize(size);
				}
				for(size_t i=0; i<size; i++)
				{
					*this % prepare_data(var.val[i], var.name, var.PersistToDB);
				}
				EndArray();
			}

			//Except that bool has its own weird behaviors... so it has to be treated like the other kind.
			void VectorSerialize(SerializationData<std::vector<bool>> &var, std::true_type)
			{
				VectorSerialize( var, std::false_type());
			}

		public:
			template<typename T>
			SerializerBase &operator%(SerializationData<std::vector<T>> &&var)
			{
				VectorSerialize(var, std::is_integral<T>());
				return *this;
			}

			template<typename key_type, typename val_type, typename comp, typename alloc>
			SerializerBase &operator%(SerializationData<std::map<key_type, val_type, comp, alloc>> &&var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						std::unordered_set<std::string> deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							key_type k;
							this->OneOff(const_cast<std::string&>(key), k);
							var.val.erase(k);
						}
					}
					else
					{
						var.val.clear();
					}
				}
				int size = var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				int calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(int i=0; i<size; i++)
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
							std::string key = GetNextKey();
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
							std::string s;
							key_type k = kvp.first;
							this->OneOff(s, k);
							(*this) % prepare_data(kvp.second, s, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			template<typename key_type, typename val_type, typename comp, typename alloc>
			SerializerBase &operator%(SerializationData<std::unordered_map<key_type, val_type, comp, alloc>> &&var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						std::unordered_set<std::string> deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							key_type k;
							this->OneOff(const_cast<std::string&>(key), k);
							var.val.erase(k);
						}
					}
					else
					{
						var.val.clear();
					}
				}
				int size = var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				int calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(int i=0; i<size; i++)
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
							std::string key = GetNextKey();
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
							std::string s;
							key_type k = kvp.first;
							this->OneOff(s, k);
							(*this) % prepare_data(kvp.second, s, var.PersistToDB);
						}
					}
				}
				EndMap();
				return *this;
			}

			template<typename val_type>
			SerializerBase &operator%(SerializationData<std::map<std::string, val_type>> &&var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						std::unordered_set<std::string> deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							var.val.erase(key);
						}
					}
					else
					{
						var.val.clear();
					}
				}
				int size = var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				int calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(int i=0; i<size; i++)
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
							std::string key = GetNextKey();
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[key] = v;
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
			template<typename val_type>
			SerializerBase &operator%(SerializationData<std::unordered_map<std::string, val_type>> &&var)
			{
				if(IsLoading())
				{
					if(IsReplicable())
					{
						std::unordered_set<std::string> deleted_keys = GetDeletedKeys(var.name);
						for(auto& key : deleted_keys)
						{
							var.val.erase(key);
						}
					}
					else
					{
						var.val.clear();
					}
				}
				int size = var.val.size();
				if(IsBinary())
				{
					(*this) % prepare_data(size, var.name, false);
				}
				int calcedSize = StartMap(var.name, var.PersistToDB);
				if(IsLoading())
				{
					if(!IsBinary())
					{
						size = calcedSize;
					}
					for(int i=0; i<size; i++)
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
							std::string key = GetNextKey();
							(*this) % prepare_data(v, key, var.PersistToDB);
							var.val[key] = v;
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

			template<typename T, typename comp, typename alloc>
			SerializerBase &operator%(SerializationData<std::set<T, comp, alloc>> &&var)
			{
				size_t size = var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					for(size_t i=0; i<size; i++)
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
			SerializerBase &operator%(SerializationData<std::unordered_set<T, comp, alloc>> &&var)
			{
				size_t size = var.val.size();
				if(IsBinary())
				{
					*this % prepare_data(size, var.name, false);
				}
				StartArray(var.name, size, var.PersistToDB);
				if(IsLoading())
				{
					for(size_t i=0; i<size; i++)
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

			virtual SerializerBase &operator%(SerializationData<class Serializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class Deserializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class BinarySerializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class BinaryDeserializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class JSONSerializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class JSONDeserializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class YAMLSerializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class YAMLDeserializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class MongoSerializer> &&){ throw std::exception(); return *this; }
			virtual SerializerBase &operator%(SerializationData<class MongoDeserializer> &&){ throw std::exception(); return *this; }
			virtual void StartArray(const std::string &, size_t&, bool = true){}
			virtual void EndArray(){}
			virtual int StartObject(const std::string &, bool = true){ return 0; }
			virtual void EndObject(){}
			virtual int StartMap(const std::string &s, bool b = true){ return StartObject(s, b); }
			virtual void EndMap(){ EndObject(); }
			virtual std::string GetNextKey(){ return ""; }
			virtual std::unordered_set<std::string> GetDeletedKeys(const std::string&){ return std::unordered_set<std::string>(); }

		protected:
			virtual SerializerBase* GetAnother(const std::string& /*data*/){ throw std::exception(); return this; }
			virtual SerializerBase* GetAnother(){ throw std::exception(); return this; }

			template<typename T>
			std::string to_string( T& val, std::true_type)
			{
				return std::move( std::to_string(val) );
			}

			template<typename T>
			std::string to_string( T& val, std::false_type)
			{
				SerializerBase* s = this->GetAnother();
				*s % prepare_data( val, "key", false );
				std::string data = s->Str();
				delete s;
				return std::move(data);
			}

			template<typename T>
			void get_from_string( const std::string& data, T& val, std::false_type )
			{
				SerializerBase* d = this->GetAnother(data);
				*d % prepare_data( val, "key", false );
				delete d;
			}

			template<typename T>
			void get_from_string( const std::string& data, T& val, std::true_type )
			{
				std::stringstream s(data);
				s >> val;
			}

			template<typename T>
			void OneOff( std::string& data, T& val )
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
			SerializerBase &operator%(T &&var)
			{
				return *this % prepare_data(var, "noname", false);
			}

			template<typename T>
			SerializerBase &operator%(SerializationData<T> &&var)
			{
				StartObject(var.name, var.PersistToDB);
				var.val.Serialize(*this);
				EndObject();
				return *this;
			}

			template<typename T>
			SerializerBase &operator%(SerializationData<T*> &&var)
			{
				StartObject(var.name, var.PersistToDB);
				var.val->Serialize(*this);
				EndObject();
				return *this;
			}

			template<typename T>
			SerializerBase &operator%(SerializationData<std::shared_ptr<T>> &&var)
			{
				if(IsLoading() && !var.val)
				{
					var.val.reset( new T() );
				}
				*this % prepare_data(*var.val, var.name, var.PersistToDB);
				return *this;
			}

			virtual const char *Data() = 0;
			virtual void Data(const std::string&){}
			virtual std::string Str() = 0;
			virtual ~SerializerBase(){}
		protected:
			bool m_bIsLoading;
			uint32_t m_size;
			uint32_t m_version;
			bool m_bIsValid;
			bool m_bInitialized;
			bool m_bIsBinary;
			bool m_bIsReplicable;
			bool m_bWithMetadata;

			template<typename T>
			void serialize(T &var, const size_t bytes, const std::string &name, bool PersistToDB)
			{
				serialize( &var, bytes, name, PersistToDB);
			}

			virtual void serialize(int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(long int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(long long int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(short int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(char *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(float *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(double *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(long double *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(bool *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(unsigned int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(unsigned long int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(unsigned long long int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(unsigned short int *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(unsigned char *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;
			virtual void serialize(std::string *var, const size_t bytes, const std::string &name, bool PersistToDB) = 0;

			SerializerBase() : m_bIsLoading(false), m_size(0), m_version(0), m_bIsValid(true), m_bInitialized(false), m_bIsBinary(false), m_bIsReplicable(false), m_bWithMetadata(true) {}
		private:
			SerializerBase(const SerializerBase&);
			SerializerBase &operator=(const SerializerBase&);
		};

		class Serializer : virtual public SerializerBase
		{
		public:
			using SerializerBase::operator%;
			template <typename T>
			SerializerBase &operator%(SerializationData<const T> &&var)
			{
				T cVar = var.val;
				return *this % prepare_data(cVar, var.name, var.PersistToDB);
			}

			template<typename T>
			SerializerBase &operator%(T &&var)
			{
				return *this % prepare_data(var, "noname", true);
			}

			template<typename T>
			SerializerBase &operator%(SerializationData<T> &&var)
			{
				StartObject(var.name, var.PersistToDB);
				var.val.Serialize(*this);
				EndObject();
				return *this;
			}

			virtual ~Serializer(){}
		protected:
			Serializer()
			{
				m_bIsLoading = false;
			}
		};

		class Deserializer : virtual public SerializerBase
		{
		public:
			using SerializerBase::operator%;
			using SerializerBase::Data;
			virtual void Data(const std::string &str) = 0;
			virtual ~Deserializer(){}
		protected:
			Deserializer()
			{
				m_bIsLoading = true;
				m_bIsValid = false;
			}
		};
	}
}
