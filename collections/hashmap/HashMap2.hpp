#pragma once

#include "Hash.hpp"
#include "../iterator/LLIterator.hpp"
#include "../accessor/Accessors.hpp"
#include "../accessor/AccessorGroup.hpp"
#include "../../memory/PoolAllocator.hpp"

namespace sprawl
{
	namespace collections
	{
		template<
			typename ValueType,
			typename Accessor1,
			typename Accessor2
		>
		class HashMap<ValueType, Accessor1, Accessor2, NullAccessor>
		{
		public:

			typedef ValueType value_type;
			typedef AccessorGroup<ValueType, Accessor1, Accessor2> mapped_type;
			typedef LLIterator<ValueType, mapped_type> iterator;
			typedef LLIterator<ValueType, mapped_type> const const_iterator;
			typedef sprawl::memory::DynamicPoolAllocator<sizeof(mapped_type)> allocator;

			SPRAWL_GETTER(1);
			SPRAWL_GETTER(2);

			template<int i, typename T>
			ValueType& get(T const& key)
			{
				mapped_type* ret = get_<i>(key);
				return ret->m_value;
			}

			template<int i, typename T>
			iterator find(T const& key)
			{
				mapped_type* ret = get_<i>(key);
				return iterator(ret);
			}

			template<int i, typename T>
			const_iterator find(T const& key) const
			{
				return cfind(key);
			}

			template<int i, typename T>
			const_iterator cfind(T const& key) const
			{
				mapped_type* ret = get_<i>(key);
				return const_iterator(ret);
			}

			SPRAWL_FINDER(1);
			SPRAWL_FINDER(2);

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

			SPRAWL_ERASER(1);
			SPRAWL_ERASER(2);

			template<int i, typename T>
			void erase(T const& key)
			{
				erase_<i>(key);
			}

			bool insert(ValueType const& val)
			{
				mapped_type* newItem = (mapped_type*)allocator::alloc();
				::new((void*)newItem) mapped_type(val);

				if (exists_(newItem))
				{
					newItem->~mapped_type();
					allocator::free(newItem);
					return false;
				}
				insert_<0>(newItem);
				return true;
			}

			template<typename T>
			bool insert(ValueType const& val, T const& key)
			{
				mapped_type* newItem = (mapped_type*)allocator::alloc();
				::new((void*)newItem) mapped_type(val, key);

				if (exists_(newItem))
				{
					newItem->~mapped_type();
					allocator::free(newItem);
					return false;
				}
				insert_<0>(newItem);
				return true;
			}

			template<typename T1, typename T2>
			bool insert(ValueType const& val, T1 const& key1, T2 const& key2)
			{
				mapped_type* newItem = (mapped_type*)allocator::alloc();
				::new((void*)newItem) mapped_type(val, key1, key2);

				if (exists_(newItem))
				{
					newItem->~mapped_type();
					allocator::free(newItem);
					return false;
				}
				insert_<0>(newItem);
				return true;
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

			template<int i, typename T>
			bool has(T const& key)
			{
				return get_<i, T>(key) != nullptr;
			}

			SPRAWL_HASCHECK(1);
			SPRAWL_HASCHECK(2);

			void reserve(size_t newBucketCount)
			{
				reserve_<1>(newBucketCount);
				reserve_<2>(newBucketCount);

				for (mapped_type* ptr = m_first; ptr != nullptr; ptr = ptr->next)
				{
					ptr->next1 = nullptr;
					ptr->prev1 = nullptr;
					ptr->next2 = nullptr;
					ptr->prev2 = nullptr;

					reinsert_(ptr);
				}
			}

			void rehash()
			{
				rehash_<1>();
				rehash_<2>();

				for (mapped_type* ptr = m_first; ptr != nullptr; ptr = ptr->next)
				{
					reinsert_(ptr);
				}
			}

			void clear()
			{
				clear_<1>();
				clear_<2>();

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

			HashMap(size_t startingBucketCount = 256)
				: m_table1(nullptr)
				, m_table2(nullptr)
				, m_first(nullptr)
				, m_last(nullptr)
				, m_size(0)
				, m_bucketCount(startingBucketCount)
			{
				reserve(startingBucketCount);
			}

			HashMap(HashMap const& other)
				: m_table1(nullptr)
				, m_table2(nullptr)
				, m_first(nullptr)
				, m_last(nullptr)
				, m_size(other.m_size)
				, m_bucketCount(other.m_bucketCount)
			{
				reserve(m_bucketCount);
				for (mapped_type* ptr = other.m_first; ptr; ptr = ptr->next)
				{
					mapped_type* newPtr = (mapped_type*)allocator::alloc();
					::new((void*)newPtr) mapped_type(*ptr);
					insert_<0>(newPtr);
				}
			}

			HashMap(HashMap&& other)
				: m_table1(other.m_table1)
				, m_table2(other.m_table2)
				, m_first(other.m_first)
				, m_last(other.m_last)
				, m_size(other.m_size)
				, m_bucketCount(other.m_bucketCount)
			{
				other.m_first = nullptr;
				other.m_last = nullptr;
				other.m_table1 = nullptr;
				other.m_size = 0;
				other.reserve(other.m_bucketCount);
			}

			~HashMap()
			{
				clear();

				allocator::free(m_table1);
				allocator::free(m_table2);
			}

		private:
			template<int i> class Specialized {};

			template<int i>
			void reserve_(size_t newBucketCount)
			{
				reserve_(newBucketCount, Specialized<i>());
			}

			SPRAWL_INDEXED_RESERVE(1);
			SPRAWL_INDEXED_RESERVE(2);

			template<int i>
			void clear_()
			{
				clear_(Specialized<i>());
			}

			SPRAWL_INDEXED_CLEAR(1);
			SPRAWL_INDEXED_CLEAR(2);

			template<int i>
			void rehash_()
			{
				rehash_(Specialized<i>());
			}

			SPRAWL_INDEXED_REHASH(1);
			SPRAWL_INDEXED_REHASH(2);

			template<typename T>
			size_t hash_(T const& val)
			{
				return sprawl::Hash<T>()(val) % m_bucketCount;
			}

			bool exists_(mapped_type* item)
			{
				if (get_<1>(item->m_accessor1.GetKey()))
				{
					return true;
				}
				if (get_<2>(item->m_accessor2.GetKey()))
				{
					return true;
				}
				return false;
			}

			void reinsert_(mapped_type* item)
			{
				insert_<1>(item);
				insert_<2>(item);
			}

			template<int i>
			void insert_(mapped_type* newItem)
			{
				insert_(newItem, Specialized<i>());
			}

			void insert_(mapped_type* newItem, Specialized<0>)
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
				reinsert_(newItem);
				if (m_size > (m_bucketCount*0.75))
				{
					reserve(m_bucketCount * 2 + 1);
				}
			}

			SPRAWL_INDEX_INSERTER(1);
			SPRAWL_INDEX_INSERTER(2);

			template<int i, typename T>
			mapped_type* get_(T const& key)
			{
				return get_(key, Specialized<i>());
			}

			SPRAWL_INDEX_GETTER(1);
			SPRAWL_INDEX_GETTER(2);

			template<int i, typename T>
			void erase_(T const& key)
			{
				return erase_(key, Specialized<i>());
			}

#define SPRAWL_INDEX_ERASER(index) \
			void erase_(typename Accessor##index::key_type const& key, Specialized<index>) \
			{ \
				size_t idx = hash_(key); \
				mapped_type* hashMatch = m_table##index[idx]; \
				while (hashMatch) \
				{ \
					if (hashMatch->m_accessor##index.GetKey() == key) \
					{ \
						if (hashMatch == m_first) \
						{ \
							m_first = hashMatch->next; \
						} \
						if (hashMatch == m_last) \
						{ \
							m_last = hashMatch->prev; \
						} \
						if (hashMatch->prev) \
						{ \
							hashMatch->prev->next = hashMatch->next; \
						} \
						if (hashMatch->next) \
						{ \
							hashMatch->next->prev = hashMatch->prev; \
						} \
						if (hashMatch->prev1) \
						{ \
							hashMatch->prev1->next1 = hashMatch->next1; \
						} \
						else \
						{ \
							m_table1[hashMatch->idx1] = hashMatch->next1; \
						} \
						if (hashMatch->next1) \
						{ \
							hashMatch->next1->prev1 = hashMatch->prev1; \
						} \
						if (hashMatch->prev2) \
						{ \
							hashMatch->prev2->next2 = hashMatch->next2; \
						} \
						else \
						{ \
							m_table2[hashMatch->idx2] = hashMatch->next2; \
						} \
						if (hashMatch->next2) \
						{ \
							hashMatch->next2->prev2 = hashMatch->prev2; \
						} \
						hashMatch->~mapped_type(); \
						allocator::free(hashMatch); \
						--m_size; \
						return; \
					} \
					hashMatch = hashMatch->next##index; \
				} \
			}

			SPRAWL_INDEX_ERASER(1);
			SPRAWL_INDEX_ERASER(2);
#undef SPRAWL_INDEX_ERASER

			mapped_type** m_table1;
			mapped_type** m_table2;

			mapped_type* m_first;
			mapped_type* m_last;

			size_t m_size;
			size_t m_bucketCount;
		};
	}
}
