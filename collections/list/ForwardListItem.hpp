#pragma once
#include <utility>

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template<typename T>
			struct ForwardListItem;
		}
	}
}

template<typename T>
struct sprawl::collections::detail::ForwardListItem
{
	ForwardListItem(T const& item)
		: next(nullptr)
		, m_value(item)
	{
		//
	}

	ForwardListItem(T&& item)
		: next(nullptr)
		, m_value(std::move(item))
	{
		//
	}

	ForwardListItem<T>* next;
	T m_value;
};
