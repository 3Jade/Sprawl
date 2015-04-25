#pragma once

#include "list/ForwardListItem.hpp"
#include "iterator/ListIterator.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename T>
		class ForwardList;
	}
}

template<typename T>
class sprawl::collections::ForwardList
{
public:
	typedef detail::ForwardListItem<T> ItemType;
	typedef ListIterator<T, ItemType> iterator;
	typedef ListIterator<T, ItemType> const const_iterator;

	ForwardList()
		: m_first(nullptr)
		, m_size(0)
	{
		//
	}

	~ForwardList()
	{
		Clear();
	}

	ForwardList(ForwardList const& other)
	    : m_first(nullptr)
	    , m_size(0)
	{
	    for(auto& value : other)
	    {
		PushFront(value);
	    }
	}

	ForwardList(ForwardList&& other)
	    : m_first(other.m_first)
	    , m_size(other.m_size)
	{
	    other.m_first = nullptr;
	    other.m_size = nullptr;
	}

	void PushFront(T const& item)
	{
		ItemType* newItem = new ItemType(item);
		if(m_first == nullptr)
		{
			m_first = newItem;
			++m_size;
		}
		else
		{
			insertBefore_(newItem, m_first);
		}
	}

	void PushFront(T&& item)
	{
		ItemType* newItem = new ItemType(std::move(item));
		if(m_first == nullptr)
		{
			m_first = newItem;
		}
		else
		{
			insertBefore_(newItem, m_first);
		}
	}

	void PopFront()
	{
		ItemType* item = m_first;
		m_first = m_first->next;
		delete item;
	}

	void Insert(const_iterator& insertAfter, T const& item)
	{
		ItemType* newItem = new ItemType(item);
		ItemType* insertAfterItem = insertAfter.m_currentItem;
		insertAfter_(newItem, insertAfterItem);
	}

	void Insert(const_iterator& insertAfter, T&& item)
	{
		ItemType* newItem = new ItemType(std::move(item));
		ItemType* insertAfterItem = insertAfter.m_currentItem;
		insertAfter_(newItem, insertAfterItem);
	}

	void EraseAfter(const_iterator& iter)
	{
		ItemType* item = iter.m_currentItem;
		if(!item)
		{
			return;
		}
		ItemType* eraseItem = item->next;
		if(!eraseItem)
		{
			return;
		}
		item->next = eraseItem->next;
		delete eraseItem;
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

	T& front()
	{
		return m_first->m_value;
	}

	void Clear()
	{
		ItemType* item = m_first;
		while(item)
		{
			ItemType* del = item;
			item = item->next;
			delete del;
		}
		m_first = nullptr;
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
		}
		insertAfter->next = newItem;
		++m_size;
	}

	void insertBefore_(ItemType* newItem, ItemType* insertBefore)
	{
		newItem->next = insertBefore;
		if(newItem->next == m_first)
		{
			m_first = newItem;
		}
		++m_size;
	}

	ItemType* m_first;
	size_t m_size;
};
