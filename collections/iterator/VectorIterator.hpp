#pragma once
#include <iterator>
#include "../../common/specialized.hpp"

namespace sprawl
{
	template<typename ValueType, typename ParentType>
	class VectorIterator;
}

template<typename ValueType, typename ParentType>
class sprawl::VectorIterator : public std::iterator<std::random_access_iterator_tag, ValueType, std::ptrdiff_t, ValueType*, ValueType&>
{
public:
	VectorIterator(ValueType* item, ParentType* parent)
		: m_currentItem(item)
		, m_parent(parent)
	{
		//
	}

	ValueType& operator*()
	{
		return *m_currentItem;
	}

	ValueType* operator->()
	{
		return m_currentItem;
	}


	ValueType const& operator*() const
	{
		return *m_currentItem;
	}

	ValueType const* operator->() const
	{
		return m_currentItem;
	}

	ValueType& Value()
	{
		return *m_currentItem;
	}

	ValueType const& Value() const
	{
		return *m_currentItem;
	}

	VectorIterator<ValueType, ParentType>& operator++()
	{
		++m_currentItem;
		return *this;
	}

	VectorIterator<ValueType, ParentType> operator++(int)
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem++, m_parent);
	}

	VectorIterator<ValueType, ParentType> const& operator++() const
	{
		++m_currentItem;
		return *this;
	}

	VectorIterator<ValueType, ParentType> const operator++(int) const
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem++, m_parent);
	}

	VectorIterator<ValueType, ParentType> operator+(int steps)
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem+steps, m_parent);
	}

	VectorIterator<ValueType, ParentType> const operator+(int steps) const
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem+steps, m_parent);
	}

	VectorIterator<ValueType, ParentType>& operator--()
	{
		--m_currentItem;
		return *this;
	}

	VectorIterator<ValueType, ParentType> operator--(int)
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem--, m_parent);
	}

	VectorIterator<ValueType, ParentType> const& operator--() const
	{
		--m_currentItem;
		return *this;
	}

	VectorIterator<ValueType, ParentType> const operator--(int) const
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem--, m_parent);
	}

	VectorIterator<ValueType, ParentType> operator-(int steps)
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem - steps, m_parent);
	}

	VectorIterator<ValueType, ParentType> const operator-(int steps) const
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem - steps, m_parent);
	}

	bool operator==(VectorIterator<ValueType, ParentType> const& rhs) const
	{
		return m_currentItem == rhs.m_currentItem;
	}

	bool operator!=(VectorIterator<ValueType, ParentType> const& rhs) const
	{
		return !this->operator==(rhs);
	}

	operator bool() const
	{
		return Valid();
	}

	bool operator!() const
	{
		return !Valid();
	}

	bool Valid() const
	{
		return operator>=(m_parent->begin()) && operator<(m_parent->end());
	}

	bool More() const
	{
		return Valid() && Next().Valid();
	}

	VectorIterator<ValueType, ParentType> Next()
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem + 1, m_parent);
	}

	VectorIterator<ValueType, ParentType> const Next() const
	{
		return VectorIterator<ValueType, ParentType>(m_currentItem + 1, m_parent);
	}

	size_t operator-(VectorIterator<ValueType, ParentType> const& other)
	{
		return m_currentItem - other.m_currentItem;
	}

	ValueType& operator[](size_t index)
	{
		return m_currentItem[index];
	}

	ValueType const& operator[](size_t index) const
	{
		return m_currentItem[index];
	}

	bool operator<(VectorIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentItem < rhs.m_currentItem;
	}

	bool operator>(VectorIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentItem > rhs.m_currentItem;
	}

	bool operator<=(VectorIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentItem <= rhs.m_currentItem;
	}

	bool operator>=(VectorIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentItem >= rhs.m_currentItem;
	}

protected:
	mutable ValueType* m_currentItem;
	ParentType* m_parent;
};
