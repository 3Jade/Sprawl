#pragma once
#include <iterator>
#include "../../common/specialized.hpp"

namespace sprawl
{
	template<typename ValueType, typename AccessorType>
	class LLIterator : public std::iterator<std::forward_iterator_tag, ValueType, std::ptrdiff_t, ValueType*, ValueType&>
	{
	public:
		typedef AccessorType* accessor_type;
		LLIterator(AccessorType* item)
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

		template<int i>
		auto Key() const -> decltype(accessor_type(nullptr)->Accessor(Specialized<i>()).GetKey())
		{
			return m_currentItem->Accessor(Specialized<i>()).GetKey();
		}

		auto Key() const -> decltype(accessor_type(nullptr)->Accessor(Specialized<0>()).GetKey())
		{
			return m_currentItem->Accessor(Specialized<0>()).GetKey();
		}

		ValueType& Value()
		{
			return m_currentItem->m_value;
		}

		ValueType const& Value() const
		{
			return m_currentItem->m_value;
		}

		LLIterator<ValueType, AccessorType>& operator++()
		{
			m_currentItem = m_currentItem ? m_currentItem->next : nullptr;
			return *this;
		}

		LLIterator<ValueType, AccessorType> operator++(int)
		{
			LLIterator<ValueType, AccessorType> tmp(*this);
			++(*this);
			return tmp;
		}

		LLIterator<ValueType, AccessorType> const& operator++() const
		{
			m_currentItem = m_currentItem ? m_currentItem->next : nullptr;
			return *this;
		}

		LLIterator<ValueType, AccessorType> const operator++(int) const
		{
			LLIterator<ValueType, AccessorType> tmp(*this);
			++(*this);
			return tmp;
		}

		LLIterator<ValueType, AccessorType> operator+(int steps)
		{
			AccessorType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = item->next;
			}
			return LLIterator<ValueType, AccessorType>(item);
		}

		const LLIterator<ValueType, AccessorType> operator+(int steps) const
		{
			AccessorType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = item->next;
			}
			return LLIterator<ValueType, AccessorType>(item);
		}

		bool operator==(LLIterator<ValueType, AccessorType> const& rhs) const
		{
			return m_currentItem == rhs.m_currentItem;
		}

		bool operator!=(const LLIterator<ValueType, AccessorType>& rhs) const
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

		LLIterator<ValueType, AccessorType> Next()
		{
			return LLIterator<ValueType, AccessorType>(m_currentItem ? m_currentItem->next : nullptr);
		}

		const LLIterator<ValueType, AccessorType> Next() const
		{
			return LLIterator<ValueType, AccessorType>(m_currentItem ? m_currentItem->next : nullptr);
		}

		operator bool()
		{
			return m_currentItem != nullptr;
		}

	protected:
		mutable AccessorType* m_currentItem;
	};
}
