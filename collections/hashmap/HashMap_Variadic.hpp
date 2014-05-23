#pragma once

#include "Hash.hpp"
#include "../iterator/LLIterator.hpp"
#include "../accessor/Accessors.hpp"
#include "../accessor/AccessorGroup_Variadic.hpp"
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

				typedef LLIterator<ValueType, mapped_type> iterator;
				typedef LLIterator<ValueType, mapped_type> const const_iterator;
				typedef sprawl::memory::DynamicPoolAllocator<sizeof(mapped_type)> allocator;

				template<int index, typename RequestedKeyType>
				void get(RequestedKeyType const&, Specialized<index>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<int index, typename RequestedKeyType>
				void find(RequestedKeyType const&, Specialized<index>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<int index, typename RequestedKeyType>
				void find(RequestedKeyType const&, Specialized<index>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<int index, typename RequestedKeyType>
				void cfind(RequestedKeyType const&, Specialized<index>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<int index, typename RequestedKeyType>
				void erase(RequestedKeyType const&, Specialized<index>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				iterator begin()
				{
					return iterator(m_first);
				}

				const_iterator begin() const
				{
					return cbegin();
				}

				const_iterator cbegin() const
				{
					return const_iterator(m_first);
				}

				iterator end()
				{
					return iterator(nullptr);
				}

				const_iterator end() const
				{
					return cend();
				}

				const_iterator cend() const
				{
					return const_iterator(nullptr);
				}

				template<int index, typename RequestedKeyType>
				void has(RequestedKeyType const&, Specialized<index>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				size_t size()
				{
					return m_size;
				}

				size_t bucketCount()
				{
					return m_bucketCount;
				}

				bool empty()
				{
					return m_size == 0;
				}

				void clear()
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
				}

				void rehash()
				{
					for (mapped_type* ptr = m_first; ptr != nullptr; ptr = ptr->next)
					{
						reinsert_(ptr);
					}
				}

				virtual ~HashMap_Impl()
				{
					clear();
				}

			protected:
				bool exists_(mapped_type*)
				{
					return false;
				}

				void reinsert_(mapped_type*)
				{
					//
				}

				void reserve_(size_t newBucketCount)
				{
					m_bucketCount = newBucketCount;
				}

				void nullout_(mapped_type*)
				{
					//
				}

				void insert_(mapped_type* newItem)
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
				}

				HashMap_Impl(size_t startingBucketCount, bool /*inherited*/)
					: m_first(nullptr)
					, m_last(nullptr)
					, m_size(0)
					, m_bucketCount(startingBucketCount)
				{
					//
				}

				HashMap_Impl(HashMap_Impl const& other, bool /*inherited*/)
					: m_first(nullptr)
					, m_last(nullptr)
					, m_size(other.m_size)
					, m_bucketCount(other.m_bucketCount)
				{
					//
				}

				HashMap_Impl(HashMap_Impl&& other, bool /*inherited*/)
					: m_first(other.m_first)
					, m_last(other.m_last)
					, m_size(other.m_size)
					, m_bucketCount(other.m_bucketCount)
				{
					other.m_first = nullptr;
					other.m_last = nullptr;
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

				typedef LLIterator<ValueType, mapped_type> iterator;
				typedef LLIterator<ValueType, mapped_type> const const_iterator;
				typedef sprawl::memory::DynamicPoolAllocator<sizeof(mapped_type)> allocator;

				using Base::get;
				ValueType& get(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					return get_(key)->m_value;
				}

				template<int i, typename T2>
				ValueType& get(T2 const& val)
				{
					return get(val, Specialized<i>());
				}

				using Base::find;
				using Base::cfind;
				iterator find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					mapped_type* ret = get_(key);
					return iterator(ret);
				}

				template<int i, typename T2>
				iterator find(T2 const& val)
				{
					return find(val, Specialized<i>());
				}

				const_iterator find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					return cfind(key);
				}

				template<int i, typename T2>
				iterator find(T2 const& val) const
				{
					return find(val, Specialized<i>());
				}

				const_iterator cfind(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					mapped_type* ret = get_(key);
					return const_iterator(ret);
				}

				template<int i, typename T2>
				iterator cfind(T2 const& val) const
				{
					return cfind(val, Specialized<i>());
				}

				using Base::has;
				bool has(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					return get_(key) != nullptr;
				}

				template<int i, typename T2>
				bool has(T2 const& val)
				{
					return has(val, Specialized<i>());
				}

				void clear()
				{
					for (size_t i = 0; i < this->m_bucketCount; ++i)
					{
						m_thisKeyTable[i] = nullptr;
					}
					Base::clear();
				}

				template<typename... Params>
				bool insert(ValueType const& val, Params... keys)
				{
					mapped_type* newItem = (mapped_type*)allocator::alloc();
					::new((void*)newItem) mapped_type(val, keys...);

					if(exists_(newItem))
					{
						newItem->~mapped_type();
						allocator::free(newItem);
						return false;
					}
					insert_(newItem);

					if(this->m_size > (this->m_bucketCount*0.75))
					{
						reserve(this->m_bucketCount * 2 + 1);
					}

					return true;
				}

				void reserve(size_t newBucketCount)
				{
					reserve_(newBucketCount);

					for (mapped_type* ptr = this->m_first; ptr != nullptr; ptr = ptr->next)
					{
						nullout_(ptr);
						reinsert_(ptr);
					}
				}

				void rehash()
				{
					rehash_();
					Base::rehash();
				}

				using Base::erase;
				void erase(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					erase_(get_(key));
				}

				template<int i, typename T2>
				void erase(T2 const& val)
				{
					return erase(val, Specialized<i>());
				}

				HashMap_Impl(size_t startingBucketCount = 256)
					: Base(startingBucketCount, true)
					, m_thisKeyTable(nullptr)
				{
					reserve(startingBucketCount);
				}

				HashMap_Impl(HashMap_Impl const& other)
					: Base(other, true)
					, m_thisKeyTable(nullptr)
				{
					reserve(this->m_bucketCount);
					for (mapped_type* ptr = other.m_first; ptr; ptr = ptr->next)
					{
						mapped_type* newPtr = (mapped_type*)allocator::alloc();
						::new((void*)newPtr) mapped_type(*ptr);
						insert_(newPtr);
					}
				}

				HashMap_Impl(HashMap_Impl&& other)
					: Base(other, true)
					, m_thisKeyTable(other.m_thisKeyTable)
				{
					other.m_thisKeyTable = nullptr;
					other.reserve(other.m_bucketCount);
				}

				virtual ~HashMap_Impl()
				{
					allocator::free(m_thisKeyTable);
				}

			protected:
				HashMap_Impl(size_t startingBucketCount, bool /*inherited*/)
					: Base(startingBucketCount, true)
					, m_thisKeyTable(nullptr)
				{
					//
				}

				HashMap_Impl(HashMap_Impl const& other, bool /*inherited*/)
					: Base(other, true)
					, m_thisKeyTable(nullptr)
				{
					//
				}

				HashMap_Impl(HashMap_Impl&& other, bool /*inherited*/)
					: Base(other, true)
					, m_thisKeyTable(other.m_thisKeyTable)
				{
					//
				}

				bool exists_(mapped_type* newItem)
				{
					if(get_(newItem->Accessor(Specialized<Idx>()).GetKey()))
					{
						return true;
					}
					return Base::exists_(newItem);
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

				void nullout_(mapped_type* item)
				{
					Specialized<Idx> spec;
					item->SetNext(spec, nullptr);
					item->SetPrev(spec, nullptr);
					Base::nullout_(item);
				}

				void reinsert_(mapped_type* newItem)
				{
					insert_here_(newItem);
					Base::reinsert_(newItem);
				}

				void insert_(mapped_type* newItem)
				{
					insert_here_(newItem);
					Base::insert_(newItem);
				}

				void erase_(mapped_type* item)
				{
					Specialized<Idx> spec;
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
				void insert_here_(mapped_type* newItem)
				{
					Specialized<Idx> spec;
					size_t idx = hash_(newItem->Accessor(spec).GetKey());

					mapped_type* next = m_thisKeyTable[idx];
					newItem->SetNext(spec, next);
					if(next != nullptr)
					{
						next->SetPrev(spec, newItem);
					}
					m_thisKeyTable[idx] = newItem;
					newItem->SetIndex(spec, idx);
				}

				mapped_type* get_(typename Accessor::key_type const& key)
				{
					Specialized<Idx> spec;
					size_t idx = hash_(key);
					mapped_type* hashMatch = m_thisKeyTable[idx];
					while (hashMatch)
					{
						if(hashMatch->Accessor(spec).GetKey() == key)
							return hashMatch;
						hashMatch = hashMatch->Next(spec);
					}
					return nullptr;
				}

				size_t hash_(typename Accessor::key_type const& val)
				{
					return sprawl::Hash<typename Accessor::key_type>()(val) % this->m_bucketCount;
				}

				void rehash_()
				{
					Specialized<Idx> spec;
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

				struct EnsureKeyUnique{};
				mapped_type** m_thisKeyTable;
			};
		}

		template< typename ValueType, typename... Accessors >
		using HashMap = detail::HashMap_Impl<ValueType, detail::AccessorGroup<ValueType, Accessors...>, 1, Accessors...>;
	}
}
