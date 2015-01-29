#pragma once
#include <utility>

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template<typename T>
			struct ListItem;
		}
	}
}

template<typename T>
struct sprawl::collections::detail::ListItem
{
	ListItem(T const& item)
		: next(nullptr)
		, prev(nullptr)
		, m_value(item)
	{
		//
	}

	ListItem(T&& item)
		: next(nullptr)
		, prev(nullptr)
		, m_value(std::move(item))
	{
		//
	}

	ListItem<T>* next;
	ListItem<T>* prev;
	T m_value;
};
