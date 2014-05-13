#pragma once
#include <iterator>

namespace sprawl
{
	template<typename ValueType, typename AccessorType>
	class LLIterator : public std::iterator<std::forward_iterator_tag, ValueType, std::ptrdiff_t, ValueType*, ValueType&>
	{
	public:
		LLIterator(AccessorType* item)
			: m_currentItem(item)
			, m_nextItem(m_currentItem ? m_currentItem->next : nullptr)
		{
			//
		}

		ValueType& operator*()
		{
			return m_currentItem->m_value;
		}

		ValueType& operator->()
		{
			return *m_currentItem->m_value;
		}


		ValueType const& operator*() const
		{
			return m_currentItem->m_value;
		}

		ValueType const& operator->() const
		{
			return *m_currentItem->m_value;
		}

		LLIterator<ValueType, AccessorType>& operator++()
		{
			m_currentItem = m_nextItem;
			m_nextItem = m_currentItem ? m_currentItem->next : nullptr;
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
			m_currentItem = m_nextItem;
			m_nextItem = m_currentItem ? m_currentItem->next : nullptr;
			return *this;
		}

		LLIterator<ValueType, AccessorType> const operator++(int) const
		{
			LLIterator<ValueType, AccessorType> tmp(*this);
			++(*this);
			return tmp;
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

	protected:
		AccessorType* m_currentItem;
		AccessorType* m_nextItem;
	};
}
