#pragma once
#include <iterator>
#include "../../common/specialized.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename T>
		class List;

		template<typename T>
		class ForwardList;
	}

	template<typename ValueType, typename WrapperType, typename tag>
	class ListIterator : public std::iterator<tag, ValueType, std::ptrdiff_t, ValueType*, ValueType&>
	{
	public:
		typedef WrapperType* accessor_type;
		ListIterator(WrapperType* item)
			: m_currentItem(item)
		{
			//
		}

		ValueType& operator*()
		{
			return m_currentItem->m_value;
		}

		ValueType* operator->()
		{
			return &m_currentItem->m_value;
		}


		ValueType const& operator*() const
		{
			return m_currentItem->m_value;
		}

		ValueType const* operator->() const
		{
			return &m_currentItem->m_value;
		}

		ValueType& Value()
		{
			return m_currentItem->m_value;
		}

		ValueType const& Value() const
		{
			return m_currentItem->m_value;
		}

		ListIterator<ValueType, WrapperType, tag>& operator++()
		{
			m_currentItem = m_currentItem ? m_currentItem->next : nullptr;
			return *this;
		}

		ListIterator<ValueType, WrapperType, tag> operator++(int)
		{
			ListIterator<ValueType, WrapperType, tag> tmp(*this);
			++(*this);
			return tmp;
		}

		ListIterator<ValueType, WrapperType, tag> const& operator++() const
		{
			m_currentItem = m_currentItem ? m_currentItem->next : nullptr;
			return *this;
		}

		ListIterator<ValueType, WrapperType, tag> const operator++(int) const
		{
			ListIterator<ValueType, WrapperType, tag> tmp(*this);
			++(*this);
			return tmp;
		}

		ListIterator<ValueType, WrapperType, tag> operator+(int steps)
		{
			WrapperType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = item->next;
			}
			return ListIterator<ValueType, WrapperType, tag>(item);
		}

		ListIterator<ValueType, WrapperType, tag> const operator+(int steps) const
		{
			WrapperType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = item->next;
			}
			return ListIterator<ValueType, WrapperType, tag>(item);
		}

		ListIterator<ValueType, WrapperType, tag>& operator--()
		{
			m_currentItem = m_currentItem ? m_currentItem->prev : nullptr;
			return *this;
		}

		ListIterator<ValueType, WrapperType, tag> operator--(int)
		{
			ListIterator<ValueType, WrapperType, tag> tmp(*this);
			--(*this);
			return tmp;
		}

		ListIterator<ValueType, WrapperType, tag> const& operator--() const
		{
			m_currentItem = m_currentItem ? m_currentItem->prev : nullptr;
			return *this;
		}

		ListIterator<ValueType, WrapperType, tag> const operator--(int) const
		{
			ListIterator<ValueType, WrapperType, tag> tmp(*this);
			--(*this);
			return tmp;
		}

		ListIterator<ValueType, WrapperType, tag> operator-(int steps)
		{
			WrapperType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = item->prev;
			}
			return ListIterator<ValueType, WrapperType, tag>(item);
		}

		ListIterator<ValueType, WrapperType, tag> const operator-(int steps) const
		{
			WrapperType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = item->prev;
			}
			return ListIterator<ValueType, WrapperType, tag>(item);
		}

		bool operator==(ListIterator<ValueType, WrapperType, tag> const& rhs) const
		{
			return m_currentItem == rhs.m_currentItem;
		}

		bool operator!=(ListIterator<ValueType, WrapperType, tag> const& rhs) const
		{
			return !this->operator==(rhs);
		}

		operator bool() const
		{
			return m_currentItem != nullptr;
		}

		bool operator!() const
		{
			return m_currentItem != nullptr;
		}

		bool Valid() const
		{
			return m_currentItem != nullptr;
		}

		bool More() const
		{
			return m_currentItem != nullptr && m_currentItem->next != nullptr;
		}

		ListIterator<ValueType, WrapperType, tag> Next()
		{
			return ListIterator<ValueType, WrapperType, tag>(m_currentItem ? m_currentItem->next : nullptr);
		}

		ListIterator<ValueType, WrapperType, tag> const Next() const
		{
			return ListIterator<ValueType, WrapperType, tag>(m_currentItem ? m_currentItem->next : nullptr);
		}

		operator bool()
		{
			return m_currentItem != nullptr;
		}

	protected:
		friend class sprawl::collections::List<ValueType>;
		friend class sprawl::collections::ForwardList<ValueType>;
		mutable WrapperType* m_currentItem;
	};
}
