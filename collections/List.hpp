#pragma once

#include "list/ListItem.hpp"
#include "iterator/ListIterator.hpp"
#include "../memory/PoolAllocator.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename T>
		class List;
	}
}

template<typename T>
class sprawl::collections::List
{
public:
	typedef detail::ListItem<T> ItemType;
	typedef ListIterator<T, ItemType, std::bidirectional_iterator_tag> iterator;
	typedef ListIterator<T, ItemType, std::bidirectional_iterator_tag> const const_iterator;
	typedef sprawl::memory::PoolAllocator<sizeof(ItemType)> allocator;

	List()
		: m_first(nullptr)
		, m_last(nullptr)
		, m_size(0)
	{
		//
	}

	List(List const& other)
		: m_first(nullptr)
		, m_last(nullptr)
		, m_size(0)
	{
		ItemType* item = other.m_first;
		while(item)
		{
			PushBack(item->m_value);
			item = item->next;
		}
	}

	List(List&& other)
		: m_first(other.m_first)
		, m_last(other.m_last)
		, m_size(other.m_size)
	{
		other.m_first = nullptr;
		other.m_last = nullptr;
		other.m_size = 0;
	}

	~List()
	{
		Clear();
	}

	void PushBack(T const& item)
	{
		ItemType* newItem = (ItemType*)allocator::alloc();
		new(newItem) ItemType(item);
		if(m_first == nullptr)
		{
			m_first = newItem;
			m_last = newItem;
			++m_size;
		}
		else
		{
			insertAfter_(newItem, m_last);
		}
	}

	void PushBack(T&& item)
	{
		ItemType* newItem = (ItemType*)allocator::alloc();
		new(newItem) ItemType(std::move(item));
		if(m_first == nullptr)
		{
			m_first = newItem;
			m_last = newItem;
			++m_size;
		}
		else
		{
			insertAfter_(newItem, m_last);
		}
	}

	void PushFront(T const& item)
	{
		ItemType* newItem = (ItemType*)allocator::alloc();
		new(newItem) ItemType(item);
		if(m_first == nullptr)
		{
			m_first = newItem;
			m_last = newItem;
			++m_size;
		}
		else
		{
			insertBefore_(newItem, m_first);
		}
	}

	void PushFront(T&& item)
	{
		ItemType* newItem = (ItemType*)allocator::alloc();
		new(newItem) ItemType(std::move(item));
		if(m_first == nullptr)
		{
			m_first = newItem;
			m_last = newItem;
			++m_size;
		}
		else
		{
			insertBefore_(newItem, m_first);
		}
	}

	void PopFront()
	{
		ItemType* item = m_first;
		m_first = item->next;
		m_first->prev = nullptr;
		item->~ItemType();
		allocator::free(item);
		--m_size;
	}

	void PopBack()
	{
		ItemType* item = m_last;
		m_last = item->prev;
		m_last->next = nullptr;
		item->~ItemType();
		allocator::free(item);
		--m_size;
	}

	void Insert(const_iterator& insertAfter, T const& item)
	{
		ItemType* newItem = (ItemType*)allocator::alloc();
		new(newItem) ItemType(item);
		ItemType* insertItem = insertAfter.m_currentItem;
		insertBefore_(newItem, insertItem);
	}

	void Insert(const_iterator& insertAfter, T&& item)
	{
		ItemType* newItem = (ItemType*)allocator::alloc();
		new(newItem) ItemType(std::move(item));
		ItemType* insertItem = insertAfter.m_currentItem;
		insertBefore_(newItem, insertItem);
	}

	void Erase(const_iterator& iter)
	{
		ItemType* item = iter.m_currentItem;
		if(!item)
		{
			return;
		}
		if(item == m_first)
		{
			m_first = item->next;
		}
		if(item == m_last)
		{
			m_last = item->prev;
		}
		if(item->next)
		{
			item->next->prev = item->prev;
		}
		if(item->prev)
		{
			item->prev->next = item->next;
		}
		item->~ItemType();
		allocator::free(item);
		--m_size;
	}

	size_t Size()
	{
		return m_size;
	}

	bool Empty()
	{
		return m_size == 0;
	}

	T& Front()
	{
		return m_first->m_value;
	}

	T& Back()
	{
		return m_last->m_value;
	}

	T const& Front() const
	{
		return m_first->m_value;
	}

	T const& Back() const
	{
		return m_last->m_value;
	}

	void Clear()
	{
		ItemType* item = m_first;
		while(item)
		{
			ItemType* del = item;
			item = item->next;
			del->~ItemType();
			allocator::free(del);
		}
		m_first = nullptr;
		m_last = nullptr;
		m_size = 0;
	}

	iterator begin()
	{
		return iterator(m_first);
	}

	iterator end()
	{
		return iterator(nullptr);
	}

	const_iterator begin() const
	{
		return const_iterator(m_first);
	}

	const_iterator end() const
	{
		return const_iterator(nullptr);
	}

	const_iterator cbegin()
	{
		return const_iterator(m_first);
	}

	const_iterator cend()
	{
		return const_iterator(nullptr);
	}

private:
	void insertAfter_(ItemType* newItem, ItemType* insertAfter)
	{
		if(insertAfter->next)
		{
			newItem->next = insertAfter->next;
			insertAfter->next->prev = newItem;
		}
		insertAfter->next = newItem;
		newItem->prev = insertAfter;
		if(newItem->prev == m_last)
		{
			m_last = newItem;
		}
		++m_size;
	}

	void insertBefore_(ItemType* newItem, ItemType* insertBefore)
	{
		if(insertBefore->prev)
		{
			newItem->prev = insertBefore->prev;
			insertBefore->prev->next = newItem;
		}
		insertBefore->prev = newItem;
		newItem->next = insertBefore;
		if(newItem->next == m_first)
		{
			m_first = newItem;
		}
		++m_size;
	}

	ItemType* m_first;
	ItemType* m_last;
	size_t m_size;
};
