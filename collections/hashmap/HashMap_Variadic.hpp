#pragma once

#include "Hash.hpp"
#include "../iterator/MapIterator.hpp"
#include "../accessor/Accessors.hpp"
#include "../accessor/AccessorGroup_HashMap.hpp"
#include "../../memory/PoolAllocator.hpp"
#include "../../common/specialized.hpp"

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template< typename ValueType, typename mapped_type, size_t Idx, typename... AdditionalAccessors >
			class HashMap_Impl;

			template< typename ValueType, typename mapped_type, size_t Idx >
			class HashMap_Impl<ValueType, mapped_type, Idx>
			{
			public:
				typedef ValueType value_type;

				typedef MapIterator<ValueType, mapped_type> iterator;
				typedef MapIterator<ValueType, mapped_type> const const_iterator;
				typedef sprawl::memory::PoolAllocator<sizeof(mapped_type)> allocator;

				template<typename RequestedKeyType>
				inline void Get(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void GetOrInsert(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void find(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template< typename RequestedKeyType>
				inline void find(RequestedKeyType const&, Specialized<Idx>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void cfind(RequestedKeyType const&, Specialized<Idx>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template< typename RequestedKeyType>
				inline void Erase(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				inline iterator begin()
				{
					return iterator(m_first);
				}

				inline const_iterator begin() const
				{
					return cbegin();
				}

				inline const_iterator cbegin() const
				{
					return const_iterator(m_first);
				}

				inline iterator end()
				{
					return iterator(nullptr);
				}

				inline const_iterator end() const
				{
					return cend();
				}

				inline const_iterator cend() const
				{
					return const_iterator(nullptr);
				}

				template<typename RequestedKeyType>
				inline void Has(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				inline size_t Size()
				{
					return m_size;
				}

				inline size_t BucketCount() const
				{
					return m_bucketCount;
				}

				inline size_t BucketSize(int, Specialized<Idx>) const
				{
					return ValueType::error_invalid_index();
				}

				inline bool Empty()
				{
					return m_size == 0;
				}

				void Clear()
				{
					mapped_type* ptr_next = nullptr;
					for (mapped_type* ptr = m_first; ptr != nullptr; ptr = ptr_next)
					{
						ptr_next = ptr->next;
						ptr->~mapped_type();
						allocator::free(ptr);
					}

					m_first = nullptr;
					m_last = nullptr;
					m_size = 0;
				}

				void Rehash()
				{
					for (mapped_type* ptr = m_first; ptr != nullptr; ptr = ptr->next)
					{
						reinsert_(ptr);
					}
				}

				virtual ~HashMap_Impl()
				{
					Clear();
				}

			protected:

				inline bool checkAndInsert_(mapped_type* newItem)
				{
					insert_(newItem);
					return true;
				}

				inline void reinsert_(mapped_type*)
				{
					//
				}

				inline void reserve_(size_t newBucketCount)
				{
					m_bucketCount = newBucketCount;
				}

				inline void nullout_(mapped_type*)
				{
					//
				}

				inline void insert_(mapped_type* newItem)
				{
					newItem->prev = m_last;
					if (m_last != nullptr)
					{
						m_last->next = newItem;
					}
					if (m_first == nullptr)
					{
						m_first = newItem;
					}
					m_last = newItem;
					++m_size;
				}

				void erase_(mapped_type* item)
				{
					if(item == m_first)
					{
						m_first = item->next;
					}
					if(item == m_last)
					{
						m_last = item->prev;
					}
					if(item->prev)
					{
						item->prev->next = item->next;
					}
					if(item->next)
					{
						item->next->prev = item->prev;
					}
					--m_size;
					allocator::free(item);
				}

				HashMap_Impl(size_t startingBucketCount)
					: m_first(nullptr)
					, m_last(nullptr)
					, m_size(0)
					, m_bucketCount(startingBucketCount)
				{
					//
				}

				HashMap_Impl(HashMap_Impl const& other)
					: m_first(nullptr)
					, m_last(nullptr)
					, m_size(other.m_size)
					, m_bucketCount(other.m_bucketCount)
				{
					//
				}

				HashMap_Impl(HashMap_Impl&& other)
					: m_first(other.m_first)
					, m_last(other.m_last)
					, m_size(other.m_size)
					, m_bucketCount(other.m_bucketCount)
				{
					other.m_first = nullptr;
					other.m_last = nullptr;
					other.m_size = 0;
				}

				mapped_type* m_first;
				mapped_type* m_last;

				size_t m_size;
				size_t m_bucketCount;
			};

			template< typename ValueType, typename mapped_type, size_t Idx, typename Accessor, typename... AdditionalAccessors >
			class HashMap_Impl<ValueType, mapped_type, Idx, Accessor, AdditionalAccessors...> : public HashMap_Impl< ValueType, mapped_type, Idx + 1, AdditionalAccessors... >
			{
			public:
				typedef HashMap_Impl<ValueType, mapped_type, Idx + 1, AdditionalAccessors...> Base;

				typedef ValueType value_type;

				typedef MapIterator<ValueType, mapped_type> iterator;
				typedef MapIterator<ValueType, mapped_type> const const_iterator;
				typedef sprawl::memory::PoolAllocator<sizeof(mapped_type)> allocator;

				using Base::Get;
				inline ValueType& Get(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					return get_(key)->Value();
				}
				inline ValueType const& Get(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					return get_(key)->Value();
				}

				using Base::GetOrInsert;
				inline ValueType& GetOrInsert(typename Accessor::key_type const& key, ValueType const& defaultValue, Specialized<Idx> = Specialized<Idx>())
				{
					auto it = find(key, spec);
					if(it.Valid())
					{
						return it.Value();
					}
					return this->Insert(key, defaultValue).Value();
				}

				inline ValueType& GetOrInsert(typename Accessor::key_type const& key, ValueType&& defaultValue, Specialized<Idx> = Specialized<Idx>())
				{
					auto it = find(key, spec);
					if(it.Valid())
					{
						return it.Value();
					}
					return this->Insert(key, std::move(defaultValue)).Value();
				}

				inline ValueType& GetOrInsert(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					auto it = find(key, spec);
					if(it.Valid())
					{
						return it.Value();
					}
					return this->Insert(key, ValueType()).Value();
				}

				inline ValueType const& GetOrInsert(typename Accessor::key_type const& key, ValueType const& defaultValue, Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_cast<HashMap_Impl*>(this)->GetOrInsert(key, defaultValue, spec);
				}

				inline ValueType const& GetOrInsert(typename Accessor::key_type const& key, ValueType&& defaultValue, Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_cast<HashMap_Impl*>(this)->GetOrInsert(key, std::move(defaultValue), spec);
				}

				inline ValueType const& GetOrInsert(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_cast<HashMap_Impl*>(this)->GetOrInsert(key, spec);
				}

				using Base::find;
				using Base::cfind;
				inline iterator find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					mapped_type* ret = get_(key);
					return iterator(ret);
				}

				inline const_iterator find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					return cfind(key);
				}

				inline const_iterator cfind(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					mapped_type* ret = const_cast<mapped_type*>(get_(key));
					return const_iterator(ret);
				}

				using Base::Has;
				inline bool Has(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					return get_(key) != nullptr;
				}

				template<int index>
				inline int BucketSize(int i, Specialized<Idx> spec = Specialized<Idx>()) const
				{
					int ret = 0;
					mapped_type *item = m_thisKeyTable[i];
					while(item)
					{
						++ret;
						item = item->Next(spec);
					}
					return ret;
				}

				inline void Clear()
				{
					for (size_t i = 0; i < this->m_bucketCount; ++i)
					{
						m_thisKeyTable[i] = nullptr;
					}
					Base::Clear();
				}

				template<typename... Params>
				inline iterator Insert(Params&&... keysAndValue)
				{
					mapped_type* newItem = (mapped_type*)allocator::alloc();
					::new((void*)newItem) mapped_type(std::forward<Params>(keysAndValue)...);

					if(this->m_size > (this->m_bucketCount*0.5))
					{
						Reserve(this->m_bucketCount * 2 + 1);
					}

					if(!checkAndInsert_(newItem))
					{
						newItem->~mapped_type();
						allocator::free(newItem);
						return iterator(nullptr);
					}

					return iterator(newItem);
				}

				inline void Reserve(size_t newBucketCount)
				{
					reserve_(newBucketCount);

					for (mapped_type* ptr = this->m_first; ptr != nullptr; ptr = ptr->next)
					{
						nullout_(ptr);
						reinsert_(ptr);
					}
				}

				inline void Rehash()
				{
					rehash_();
					Base::Rehash();
				}

				using Base::Erase;
				inline void Erase(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					erase_(get_(key));
				}

				virtual ~HashMap_Impl()
				{
					if(m_thisKeyTable)
					{
						allocator::free(m_thisKeyTable);
					}
				}

			protected:
				HashMap_Impl(size_t startingBucketCount)
					: Base(startingBucketCount)
					, m_thisKeyTable(nullptr)
				{
					//
				}

				HashMap_Impl(HashMap_Impl const& other)
					: Base(other)
					, m_thisKeyTable(nullptr)
				{
					//
				}

				HashMap_Impl(HashMap_Impl&& other)
					: Base(std::move(other))
					, m_thisKeyTable(other.m_thisKeyTable)
				{
					other.m_thisKeyTable = nullptr;
				}

				void reserve_(size_t newBucketCount)
				{
					if(m_thisKeyTable)
					{
						allocator::free(m_thisKeyTable);
					}
					m_thisKeyTable = (mapped_type**)allocator::alloc(newBucketCount);
					for (size_t i = 0; i < newBucketCount; ++i)
					{
						m_thisKeyTable[i] = nullptr;
					}
					Base::reserve_(newBucketCount);
				}

				inline void nullout_(mapped_type* item)
				{
					item->SetNext(spec, nullptr);
					item->SetPrev(spec, nullptr);
					Base::nullout_(item);
				}

				inline void reinsert_(mapped_type* newItem)
				{
					insertHere_(newItem);
					Base::reinsert_(newItem);
				}

				inline void insert_(mapped_type* newItem)
				{
					insertHere_(newItem);
					Base::insert_(newItem);
				}

				bool checkAndInsert_(mapped_type* newItem)
				{
					typename Accessor::key_type const& key = newItem->Accessor(spec).Key();

					size_t hash = hash_(key);
					newItem->SetHash(spec, hash);
					size_t index = hash % this->m_bucketCount;

					mapped_type* hashMatch = m_thisKeyTable[index];
					while (hashMatch)
					{
						if(hashMatch->Accessor(spec).Key() == key)
							return hashMatch;
						hashMatch = hashMatch->Next(spec);
					}

					if(hashMatch)
					{
						return false;
					}

					if(!Base::checkAndInsert_(newItem))
					{
						return false;
					}

					insertHere_(newItem, index);
					return true;
				}

				void erase_(mapped_type* item)
				{
					mapped_type* prev = item->Prev(spec);
					mapped_type* next = item->Next(spec);

					if(prev)
					{
						prev->SetNext(spec, next);
					}
					else
					{
						m_thisKeyTable[item->Idx(spec)] = next;
					}

					if (next)
					{
						next->SetPrev(spec, prev);
					}
					Base::erase_(item);
				}


			private:
				inline void insertHere_(mapped_type* newItem)
				{
					size_t idx = newItem->GetHash(spec) % this->m_bucketCount;
					insertHere_(newItem, idx);
				}

				inline void insertHere_(mapped_type* newItem, size_t idx)
				{
					mapped_type* next = m_thisKeyTable[idx];
					newItem->SetNext(spec, next);
					if(next != nullptr)
					{
						next->SetPrev(spec, newItem);
					}
					m_thisKeyTable[idx] = newItem;
					newItem->SetIndex(spec, idx);
				}

				inline mapped_type* get_(typename Accessor::key_type const& key)
				{
					size_t hash = hash_(key);
					size_t idx = hash % this->m_bucketCount;
					mapped_type* hashMatch = m_thisKeyTable[idx];
					while(hashMatch)
					{
						if(hashMatch->GetHash(spec) == hash && hashMatch->Accessor(spec).Key() == key)
							return hashMatch;
						hashMatch = hashMatch->Next(spec);
					}
					return nullptr;
				}

				inline mapped_type const* get_(typename Accessor::key_type const& key) const
				{
					size_t hash = hash_(key);
					size_t idx = hash % this->m_bucketCount;
					mapped_type* hashMatch = m_thisKeyTable[idx];
					while(hashMatch)
					{
						if(hashMatch->GetHash(spec) == hash && hashMatch->Accessor(spec).Key() == key)
							return hashMatch;
						hashMatch = hashMatch->Next(spec);
					}
					return nullptr;
				}

				inline size_t hash_(typename Accessor::key_type const& val) const
				{
					return sprawl::Hash<typename Accessor::key_type>::Compute(val);
				}

				void rehash_()
				{
					for (size_t i = 0; i < this->m_bucketCount; ++i)
					{
						mapped_type* item = m_thisKeyTable[i];
						while (item)
						{
							mapped_type* item_next = item->Next(spec);
							item->SetNext(spec, nullptr);
							item->SetPrev(spec, nullptr);
							item = item_next;
						}
						m_thisKeyTable[i] = nullptr;
					}
				}

				mapped_type** m_thisKeyTable;
				static Specialized<Idx> spec;
			};

			template< typename ValueType, typename mapped_type, size_t Idx, typename Accessor, typename... AdditionalAccessors >
			/*static*/ Specialized<Idx> HashMap_Impl<ValueType, mapped_type, Idx, Accessor, AdditionalAccessors...>::spec;
		}

		template< typename ValueType, typename... Accessors >
		class HashMap : public detail::HashMap_Impl<ValueType, detail::MapAccessorGroup<ValueType, Accessors...>, 0, Accessors...>
		{
		public:
			typedef detail::HashMap_Impl<ValueType, detail::MapAccessorGroup<ValueType, Accessors...>, 0, Accessors...> Base;
			typedef detail::MapAccessorGroup<ValueType, Accessors...> mapped_type;

			using Base::Get;
			template<int i, typename T2>
			inline ValueType& Get(T2 const& key)
			{
				return Get(key, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& Get(T2 const& key) const
			{
				return Get(key, Specialized<i>());
			}

			using Base::GetOrInsert;
			template<int i, typename T2>
			inline ValueType& GetOrInsert(T2 const& key, ValueType const& defaultValue)
			{
				return GetOrInsert(key, defaultValue, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& GetOrInsert(T2 const& key, ValueType const& defaultValue) const
			{
				return GetOrInsert(key, defaultValue, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType& GetOrInsert(T2 const& key, ValueType&& defaultValue)
			{
				return GetOrInsert(key, std::move(defaultValue), Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& GetOrInsert(T2 const& key, ValueType&& defaultValue) const
			{
				return GetOrInsert(key, std::move(defaultValue), Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType& GetOrInsert(T2 const& key)
			{
				return GetOrInsert(key, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& GetOrInsert(T2 const& key) const
			{
				return GetOrInsert(key, Specialized<i>());
			}

			template<typename T2>
			inline ValueType& operator[](T2 const& key)
			{
				return GetOrInsert(key);
			}

			template<typename T2>
			inline ValueType const& operator[](T2 const& key) const
			{
				return GetOrInsert(key);
			}
			
			using Base::find;
			template<int i, typename T2>
			inline typename Base::iterator find(T2 const& val)
			{
				return find(val, Specialized<i>());
			}

			template<int i, typename T2>
			inline typename Base::iterator find(T2 const& val) const
			{
				return find(val, Specialized<i>());
			}
			
			using Base::cfind;
			template<int i, typename T2>
			inline typename Base::iterator cfind(T2 const& val) const
			{
				return cfind(val, Specialized<i>());
			}
			
			using Base::Has;
			template<int i, typename T2>
			inline bool Has(T2 const& val)
			{
				return Has(val, Specialized<i>());
			}
			
			using Base::Erase;
			template<int i, typename T2>
			inline void Erase(T2 const& val)
			{
				return Erase(val, Specialized<i>());
			}

			HashMap(size_t startingBucketCount = 256)
				: Base(startingBucketCount)
			{
				Base::Reserve(startingBucketCount);
			}

			HashMap(HashMap const& other)
				: Base(other)
			{
				Base::Reserve(this->m_bucketCount);
				for (mapped_type* ptr = other.m_first; ptr; ptr = ptr->next)
				{
					mapped_type* newPtr = (mapped_type*)Base::allocator::alloc();
					::new((void*)newPtr) mapped_type(*ptr);
					Base::insert_(newPtr);
				}
			}

			HashMap(HashMap&& other)
				: Base(std::move(other))
			{
				other.Reserve(other.m_bucketCount);
			}

			HashMap& operator=(HashMap const& other)
			{
				Base::Clear();
				this->m_bucketCount = other.m_bucketCount;
				this->m_size = other.m_size;
				Base::Reserve(this->m_bucketCount);
				for (mapped_type* ptr = other.m_first; ptr; ptr = ptr->next)
				{
					mapped_type* newPtr = (mapped_type*)Base::allocator::alloc();
					::new((void*)newPtr) mapped_type(*ptr);
					Base::insert_(newPtr);
				}
				return *this;
			}
		};
	}
}
