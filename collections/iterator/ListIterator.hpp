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

	template<typename ValueType, typename WrapperType>
	class ListIterator : public std::iterator<std::forward_iterator_tag, ValueType, std::ptrdiff_t, ValueType*, ValueType&>
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

		ValueType& operator->()
		{
			return m_currentItem->m_value;
		}


		ValueType const& operator*() const
		{
			return m_currentItem->m_value;
		}

		ValueType const& operator->() const
		{
			return m_currentItem->m_value;
		}

		ValueType& Value()
		{
			return m_currentItem->m_value;
		}

		ValueType const& Value() const
		{
			return m_currentItem->m_value;
		}

		ListIterator<ValueType, WrapperType>& operator++()
		{
			m_currentItem = m_currentItem ? m_currentItem->next : nullptr;
			return *this;
		}

		ListIterator<ValueType, WrapperType> operator++(int)
		{
			ListIterator<ValueType, WrapperType> tmp(*this);
			++(*this);
			return tmp;
		}

		ListIterator<ValueType, WrapperType> const& operator++() const
		{
			m_currentItem = m_currentItem ? m_currentItem->next : nullptr;
			return *this;
		}

		ListIterator<ValueType, WrapperType> const operator++(int) const
		{
			ListIterator<ValueType, WrapperType> tmp(*this);
			++(*this);
			return tmp;
		}

		ListIterator<ValueType, WrapperType> operator+(int steps)
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
			return ListIterator<ValueType, WrapperType>(item);
		}

		ListIterator<ValueType, WrapperType> const operator+(int steps) const
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
			return ListIterator<ValueType, WrapperType>(item);
		}

		bool operator==(ListIterator<ValueType, WrapperType> const& rhs) const
		{
			return m_currentItem == rhs.m_currentItem;
		}

		bool operator!=(ListIterator<ValueType, WrapperType> const& rhs) const
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

		ListIterator<ValueType, WrapperType> Next()
		{
			return ListIterator<ValueType, WrapperType>(m_currentItem ? m_currentItem->next : nullptr);
		}

		ListIterator<ValueType, WrapperType> const Next() const
		{
			return ListIterator<ValueType, WrapperType>(m_currentItem ? m_currentItem->next : nullptr);
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
