#pragma once

#include "Hash.hpp"
#include "../iterator/MapIterator.hpp"
#include "../accessor/Accessors.hpp"
#include "../accessor/AccessorGroup_Windows.hpp"
#include "../../memory/PoolAllocator.hpp"
#include "../../common/specialized.hpp"
#include <xtr1common>

using std::_Nil;

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template<typename ValueType, typename mapped_type, size_t Idx, typename ThisAccessor = _Nil, _MAX_CLASS_LIST>
			class HashMap_Impl;

			template< typename ValueType, typename mapped_type, size_t Idx>
			class HashMap_Impl<ValueType, mapped_type, Idx, _Nil, _MAX_NIL_LIST >
			{
			public:
				typedef ValueType value_type;

				typedef MapIterator<ValueType, mapped_type> iterator;
				typedef MapIterator<ValueType, mapped_type> const const_iterator;
				typedef sprawl::memory::DynamicPoolAllocator<sizeof(mapped_type)> allocator;

				template<typename RequestedKeyType>
				inline void get(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline iterator find(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline const_iterator find(RequestedKeyType const&, Specialized<Idx>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline const_iterator cfind(RequestedKeyType const&, Specialized<Idx>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void erase(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				inline size_t bucketSize(int, Specialized<Idx>) const
				{
					return ValueType::error_invalid_index();
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
				inline void has(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				inline size_t size()
				{
					return m_size;
				}

				inline size_t bucketCount() const
				{
					return m_bucketCount;
				}

				inline bool empty()
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

				inline void rehash()
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
				inline bool check_and_insert_(mapped_type* newItem)
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
			
#define _CLASS_HASHMAP_IMPL( \
			TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			template< typename ValueType, typename mapped_type, size_t Idx, typename Accessor COMMA LIST(_CLASS_TYPEX)> \
			class HashMap_Impl<ValueType, mapped_type, Idx, Accessor, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> \
				: public HashMap_Impl< ValueType, mapped_type, Idx + 1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD) > \
			{ \
			public: \
				typedef HashMap_Impl<ValueType, mapped_type, Idx + 1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> Base; \
				 \
				typedef ValueType value_type; \
				 \
				typedef MapIterator<ValueType, mapped_type> iterator; \
				typedef MapIterator<ValueType, mapped_type> const const_iterator; \
				typedef sprawl::memory::DynamicPoolAllocator<sizeof(mapped_type)> allocator; \
				 \
				using Base::get; \
				inline ValueType& get(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) \
				{ \
					return get_(key)->m_value; \
				} \
				inline ValueType const& get(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const \
				{ \
					return get_(key)->m_value; \
				} \
				 \
				using Base::find; \
				using Base::cfind; \
				inline iterator find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) \
				{ \
					mapped_type* ret = get_(key); \
					return iterator(ret); \
				} \
				 \
				inline const_iterator find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const \
				{ \
					return cfind(key); \
				} \
				 \
				inline const_iterator cfind(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const \
				{ \
					mapped_type* ret = const_cast<mapped_type*>(get_(key)); \
					return const_iterator(ret); \
				} \
				 \
				using Base::has; \
				inline bool has(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) \
				{ \
					return get_(key) != nullptr; \
				} \
				 \
				template<int index> \
				inline int bucketSize(int i, Specialized<Idx> spec = Specialized<Idx>()) const \
				{ \
					int ret = 0; \
					mapped_type *item = m_thisKeyTable[i]; \
					while(item) \
					{ \
						++ret; \
						item = item->Next(spec); \
					} \
					return ret; \
				} \
				 \
				inline void clear() \
				{ \
					for (size_t i = 0; i < this->m_bucketCount; ++i) \
					{ \
						m_thisKeyTable[i] = nullptr; \
					} \
					Base::clear(); \
				} \
				 \
				inline iterator insert(ValueType const& val) \
				{ \
					mapped_type* newItem = (mapped_type*)allocator::alloc(); \
					::new((void*)newItem) mapped_type(val); \
					\
					if(!check_and_insert_(newItem)) \
					{ \
						newItem->~mapped_type(); \
						allocator::free(newItem); \
						return iterator(nullptr); \
					} \
					 \
					if(this->m_size > (this->m_bucketCount*0.5)) \
					{ \
						reserve(this->m_bucketCount * 2 + 1); \
					} \
					\
					return iterator(newItem); \
				} \
				 \
				_HASHMAP_VARIADICS( TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
				 \
				void reserve(size_t newBucketCount) \
				{ \
					reserve_(newBucketCount); \
					\
					for (mapped_type* ptr = this->m_first; ptr != nullptr; ptr = ptr->next) \
					{ \
						nullout_(ptr); \
						reinsert_(ptr); \
					} \
				} \
				 \
				inline void rehash() \
				{ \
					rehash_(); \
					Base::rehash(); \
				} \
				 \
				using Base::erase; \
				inline void erase(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) \
				{ \
					erase_(get_(key)); \
				} \
				 \
				virtual ~HashMap_Impl() \
				{ \
					if(m_thisKeyTable) \
					{ \
						allocator::free(m_thisKeyTable); \
					} \
				} \
				 \
			protected: \
				HashMap_Impl(size_t startingBucketCount) \
					: Base(startingBucketCount) \
					, m_thisKeyTable(nullptr) \
				{ \
					 \
				} \
				 \
				HashMap_Impl(HashMap_Impl const& other) \
					: Base(other) \
					, m_thisKeyTable(nullptr) \
				{ \
					 \
				} \
				 \
				HashMap_Impl(HashMap_Impl&& other) \
					: Base(std::move(other)) \
					, m_thisKeyTable(other.m_thisKeyTable) \
				{ \
					other.m_thisKeyTable = nullptr; \
				} \
				 \
				void reserve_(size_t newBucketCount) \
				{ \
					if(m_thisKeyTable) \
					{ \
						allocator::free(m_thisKeyTable); \
					} \
					m_thisKeyTable = (mapped_type**)allocator::alloc(newBucketCount); \
					for (size_t i = 0; i < newBucketCount; ++i) \
					{ \
						m_thisKeyTable[i] = nullptr; \
					} \
					Base::reserve_(newBucketCount); \
				} \
				 \
				inline void nullout_(mapped_type* item) \
				{ \
					Specialized<Idx> spec; \
					item->SetNext(spec, nullptr); \
					item->SetPrev(spec, nullptr); \
					Base::nullout_(item); \
				} \
				 \
				inline void reinsert_(mapped_type* newItem) \
				{ \
					insert_here_(newItem); \
					Base::reinsert_(newItem); \
				} \
				 \
				inline void insert_(mapped_type* newItem) \
				{ \
					insert_here_(newItem); \
					Base::insert_(newItem); \
				} \
				 \
				bool check_and_insert_(mapped_type* newItem) \
				{ \
					Specialized<Idx> spec; \
					typename Accessor::key_type const& key = newItem->Accessor(spec).GetKey(); \
					 \
					size_t hash = hash_(key); \
					newItem->SetHash(spec, hash); \
					size_t index = hash % this->m_bucketCount; \
					 \
					mapped_type* hashMatch = m_thisKeyTable[index]; \
					while (hashMatch) \
					{ \
						if(hashMatch->Accessor(spec).GetKey() == key) \
							return hashMatch; \
						hashMatch = hashMatch->Next(spec); \
					} \
					 \
					if(hashMatch) \
					{ \
						return false; \
					} \
					 \
					if(!Base::check_and_insert_(newItem)) \
					{ \
						return false; \
					} \
					 \
					insert_here_(newItem, index); \
					return true; \
				} \
				 \
				void erase_(mapped_type* item) \
				{ \
					Specialized<Idx> spec; \
					mapped_type* prev = item->Prev(spec); \
					mapped_type* next = item->Next(spec); \
					\
					if(prev) \
					{ \
						prev->SetNext(spec, next); \
					} \
					else \
					{ \
						m_thisKeyTable[item->Idx(spec)] = next; \
					} \
					\
					if (next) \
					{ \
						next->SetPrev(spec, prev); \
					} \
					Base::erase_(item); \
				} \
				 \
				 \
			private: \
				inline void insert_here_(mapped_type* newItem) \
				{ \
					Specialized<Idx> spec; \
					size_t idx = newItem->GetHash(spec) % this->m_bucketCount; \
					 \
					mapped_type* next = m_thisKeyTable[idx]; \
					newItem->SetNext(spec, next); \
					if(next != nullptr) \
					{ \
						next->SetPrev(spec, newItem); \
					} \
					m_thisKeyTable[idx] = newItem; \
					newItem->SetIndex(spec, idx); \
				} \
				 \
				inline void insert_here_(mapped_type* newItem, size_t idx) \
				{ \
					Specialized<Idx> spec; \
					 \
					mapped_type* next = m_thisKeyTable[idx]; \
					newItem->SetNext(spec, next); \
					if(next != nullptr) \
					{ \
						next->SetPrev(spec, newItem); \
					} \
					m_thisKeyTable[idx] = newItem; \
					newItem->SetIndex(spec, idx); \
				} \
				 \
				inline mapped_type const* get_(typename Accessor::key_type const& key) const \
				{ \
					Specialized<Idx> spec; \
					size_t hash = hash_(key); \
					size_t idx = hash % this->m_bucketCount; \
					mapped_type* hashMatch = m_thisKeyTable[idx]; \
					while(hashMatch) \
					{ \
						if(hashMatch->GetHash(spec) == hash && hashMatch->Accessor(spec).GetKey() == key) \
							return hashMatch; \
						hashMatch = hashMatch->Next(spec); \
					} \
					return nullptr; \
				} \
				 \
				inline mapped_type* get_(typename Accessor::key_type const& key) \
				{ \
					Specialized<Idx> spec; \
					size_t hash = hash_(key); \
					size_t idx = hash % this->m_bucketCount; \
					mapped_type* hashMatch = m_thisKeyTable[idx]; \
					while(hashMatch) \
					{ \
						if(hashMatch->GetHash(spec) == hash && hashMatch->Accessor(spec).GetKey() == key) \
							return hashMatch; \
						hashMatch = hashMatch->Next(spec); \
					} \
					return nullptr; \
				} \
				 \
				inline size_t hash_(typename Accessor::key_type const& val) \
				{ \
					return sprawl::Hash<typename Accessor::key_type>::Compute(val); \
				} \
				 \
				void rehash_() \
				{ \
					Specialized<Idx> spec; \
					for (size_t i = 0; i < this->m_bucketCount; ++i) \
					{ \
						mapped_type* item = m_thisKeyTable[i]; \
						while (item) \
						{ \
							mapped_type* item_next = item->Next(spec); \
							item->SetNext(spec, nullptr); \
							item->SetPrev(spec, nullptr); \
							item = item_next; \
						} \
						m_thisKeyTable[i] = nullptr; \
					} \
				} \
				 \
				struct EnsureKeyUnique{}; \
				mapped_type** m_thisKeyTable; \
			};

#define _HASHMAP_VARIADIC_VARIADICS(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CLASS_TEMPLATE_LIST, CLASS_PADDING_LIST, CLASS_LIST, CLASS_COMMA) \
			template<typename FirstArg COMMA LIST(_CLASS_TYPE)> \
			bool insert(ValueType const& val, FirstArg const& arg COMMA LIST(_TYPE_REFREF_ARG)) \
			{ \
				mapped_type* newItem = (mapped_type*)allocator::alloc(); \
				::new((void*)newItem) mapped_type(val, arg COMMA LIST(_FORWARD_ARG)); \
				\
				if(!check_and_insert_(newItem)) \
				{ \
					newItem->~mapped_type(); \
					allocator::free(newItem); \
					return false; \
				} \
					\
				if(this->m_size > (this->m_bucketCount*0.5)) \
				{ \
					reserve(this->m_bucketCount * 2 + 1); \
				} \
				\
				return true; \
			} 

#define _HASHMAP_VARIADICS(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			_HASHMAP_VARIADIC_VARIADICS(_TEM_LIST0, _PAD_LIST0, _RAW_LIST0, , TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_HASHMAP_VARIADIC_VARIADICS(_TEM_LIST1, _PAD_LIST1, _RAW_LIST1, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_HASHMAP_VARIADIC_VARIADICS(_TEM_LIST2, _PAD_LIST2, _RAW_LIST2, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_HASHMAP_VARIADIC_VARIADICS(_TEM_LIST3, _PAD_LIST3, _RAW_LIST3, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_HASHMAP_VARIADIC_VARIADICS(_TEM_LIST4, _PAD_LIST4, _RAW_LIST4, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_HASHMAP_VARIADIC_VARIADICS(_TEM_LIST5, _PAD_LIST5, _RAW_LIST5, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA)
			
			_VARIADIC_EXPAND_0X(_CLASS_HASHMAP_IMPL, , , , )
		}
		
		template<typename ValueType, typename ThisAccessor = _Nil, _MAX_CLASS_LIST>
		class HashMap;

#define _CLASS_HASHMAP( \
		TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
		template< typename ValueType COMMA LIST(_CLASS_TYPEX)> \
		class HashMap<ValueType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> \
			: public detail::HashMap_Impl< \
				ValueType, \
				detail::AccessorGroup<ValueType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)>, \
				0, \
				LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD) \
			> \
		{ \
		public: \
			typedef detail::HashMap_Impl< \
				ValueType, \
				detail::AccessorGroup<ValueType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)>, \
				0, \
				LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD) \
			> Base; \
			typedef detail::AccessorGroup<ValueType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> mapped_type; \
			 \
			using Base::get; \
			template<int i, typename T2> \
			inline ValueType& get(T2 const& val) \
			{ \
				return Base::get(val, Specialized<i>()); \
			} \
			template<int i, typename T2> \
			inline ValueType const& get(T2 const& val) const\
			{ \
				return Base::get(val, Specialized<i>()); \
			} \
			 \
			using Base::find; \
			template<int i, typename T2> \
			inline typename Base::iterator find(T2 const& val) \
			{ \
				return Base::find(val, Specialized<i>()); \
			} \
			 \
			template<int i, typename T2> \
			inline typename Base::iterator find(T2 const& val) const \
			{ \
				return Base::find(val, Specialized<i>()); \
			} \
			 \
			using Base::cfind; \
			template<int i, typename T2> \
			inline typename Base::iterator cfind(T2 const& val) const \
			{ \
				return Base::cfind(val, Specialized<i>()); \
			} \
			 \
			using Base::erase; \
			template<int i, typename T2> \
			inline void erase(T2 const& val) \
			{ \
				return Base::erase(val, Specialized<i>()); \
			} \
			 \
			using Base::has; \
			template<int i, typename T2> \
			inline bool has(T2 const& val) \
			{ \
				return Base::has(val, Specialized<i>()); \
			} \
			 \
			HashMap(size_t startingBucketCount = 256) \
				: Base(startingBucketCount) \
			{ \
				Base::reserve(startingBucketCount); \
			} \
			 \
			HashMap(HashMap const& other) \
				: Base(other) \
			{ \
				Base::reserve(this->m_bucketCount); \
				for (mapped_type* ptr = other.m_first; ptr; ptr = ptr->next) \
				{ \
					mapped_type* newPtr = (mapped_type*)Base::allocator::alloc(); \
					::new((void*)newPtr) mapped_type(*ptr); \
					Base::insert_(newPtr); \
				} \
			} \
			 \
			HashMap(HashMap&& other) \
				: Base(std::move(other)) \
			{ \
				other.reserve(other.m_bucketCount); \
			} \
			 \
			HashMap& operator=(HashMap const& other) \
			{ \
				Base::clear(); \
				this->m_bucketCount = other.m_bucketCount; \
				this->m_size = other.m_size; \
				Base::reserve(this->m_bucketCount); \
				for (mapped_type* ptr = other.m_first; ptr; ptr = ptr->next) \
				{ \
					mapped_type* newPtr = (mapped_type*)Base::allocator::alloc(); \
					::new((void*)newPtr) mapped_type(*ptr); \
					Base::insert_(newPtr); \
				} \
				return *this; \
			} \
			\
		};
		_VARIADIC_EXPAND_0X(_CLASS_HASHMAP, , , , )
	}
}
