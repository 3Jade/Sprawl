#pragma once

//Support for legacy compilers lacking variadic template support
#if (defined(_WIN32) && _MSC_VER < 1800) \
	|| (defined(__clang__) && __clang_minor__ < 2 || (__clang_major__ == 2 && __clang_minor__ < 9)) \
	|| (defined(__GNUC__) && __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3)) \
	|| defined(SPRAWL_NO_VARIADIC_TEMPLATES)

#define SPRAWL_GETTER(index) \
	struct EnsureKey##index##Unique{}; \
	inline ValueType& get(typename Accessor##index::key_type const& key, EnsureKey##index##Unique* = nullptr) \
	{ \
		return get<index>(key); \
	}

#define SPRAWL_HASCHECK(index) \
	inline bool has(typename Accessor##index::key_type const& key, EnsureKey##index##Unique* = nullptr) \
	{ \
		return get_<index, typename Accessor##index::key_type>(key) != nullptr; \
	}

#define SPRAWL_ERASER(index) \
	inline void erase(typename Accessor##index::key_type const& key, EnsureKey##index##Unique* = nullptr) \
	{ \
		erase<index>(key); \
	}

#define SPRAWL_FINDER(index) \
	inline iterator& find(typename Accessor##index::key_type const& key, EnsureKey##index##Unique* = nullptr) \
	{ \
		return find<index>(key); \
	} \
	\
	inline const_iterator& find(typename Accessor##index::key_type const& key, EnsureKey##index##Unique* = nullptr) const \
	{ \
		return cfind<index>(key); \
	}

#define SPRAWL_INDEXED_RESERVE(index) \
	void reserve_(size_t newBucketCount, Specialized<index>) \
	{ \
		if (m_table##index) \
		{ \
			allocator::free(m_table##index); \
		} \
		m_table##index = (mapped_type**)allocator::alloc(newBucketCount); \
		for (size_t i = 0; i < m_bucketCount; ++i) \
		{ \
			m_table##index[i] = nullptr; \
		} \
	}

#define SPRAWL_INDEXED_CLEAR(index) \
	void clear_(Specialized<index>) \
	{ \
		for (size_t i = 0; i < m_bucketCount; ++i) \
		{ \
			m_table##index[i] = nullptr; \
		} \
	}

#define SPRAWL_INDEXED_REHASH(index) \
	void rehash_(Specialized<index>) \
	{ \
		for (size_t i = 0; i < m_bucketCount; ++i) \
		{ \
			mapped_type* item = m_table##index[i]; \
			while (item) \
			{ \
				mapped_type* item_next = item->next##index; \
				item->next##index = nullptr; \
				item->prev##index = nullptr; \
				item = item_next; \
			} \
			m_table##index[i] = nullptr; \
		} \
	}


#define SPRAWL_INDEX_INSERTER(index) \
	void insert_(mapped_type* t, Specialized<index>) \
	{ \
		size_t idx = hash_(t->m_accessor##index.GetKey()); \
		t->next##index = m_table##index[idx]; \
		if (t->next##index != nullptr) \
		{ \
			t->next##index->prev##index = t; \
		} \
		m_table##index[idx] = t; \
		t->idx##index = idx; \
	}


#define SPRAWL_INDEX_GETTER(index) \
	mapped_type* get_(typename Accessor##index::key_type const& key, Specialized<index>) \
	{ \
		size_t idx = hash_(key); \
		mapped_type* hashMatch = m_table##index[idx]; \
		while (hashMatch) \
		{ \
			if (hashMatch->m_accessor##index.GetKey() == key) \
				return hashMatch; \
			hashMatch = hashMatch->next##index; \
		} \
		return nullptr; \
	}

//Wish there were a better way to do this. To keep from holding memory around that we're not using, edit in three places.
//Just one of the reasons for limiting the number of accessors to 3.
#include "hashmap/HashMap3.hpp"
#include "hashmap/HashMap2.hpp"
#include "hashmap/HashMap1.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType>
		using HashSet = HashMap<ValueType, SelfAccessor<ValueType>>;
	}
}

#undef SPRAWL_GETTER
#undef SPRAWL_HASCHECK
#undef SPRAWL_ERASER
#undef SPRAWL_FINDER
#undef SPRAWL_INDEXED_RESERVE
#undef SPRAWL_INDEXED_CLEAR
#undef SPRAWL_INDEXED_REHASH
#undef SPRAWL_INDEX_INSERTER
#undef SPRAWL_INDEX_GETTER
#else
#include "hashmap/HashMap_Variadic.hpp"
#endif
