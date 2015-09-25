#pragma once
#include <iterator>
#include "../../common/specialized.hpp"

namespace sprawl
{
	template<typename ValueType, typename ParentType>
	class DequeIterator;
}

template<typename ValueType, typename ParentType>
class sprawl::DequeIterator : public std::iterator<std::random_access_iterator_tag, ValueType, std::ptrdiff_t, ValueType*, ValueType&>
{
public:
	DequeIterator(size_t index, ParentType* parent)
		: m_currentIndex(index)
		, m_parent(parent)
	{
		//
	}

	ValueType& operator*()
	{
		return m_parent->operator[](m_currentIndex);
	}

	ValueType* operator->()
	{
		return &m_parent->operator[](m_currentIndex);
	}


	ValueType const& operator*() const
	{
		return m_parent->operator[](m_currentIndex);
	}

	ValueType const* operator->() const
	{
		return &m_parent->operator[](m_currentIndex);
	}

	ValueType& Value()
	{
		return m_parent->operator[](m_currentIndex);
	}

	ValueType const& Value() const
	{
		return m_parent->operator[](m_currentIndex);
	}

	DequeIterator<ValueType, ParentType>& operator++()
	{
		++m_currentIndex;
		return *this;
	}

	DequeIterator<ValueType, ParentType> operator++(int)
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex++, m_parent);
	}

	DequeIterator<ValueType, ParentType> const& operator++() const
	{
		++m_currentIndex;
		return *this;
	}

	DequeIterator<ValueType, ParentType> const operator++(int) const
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex++, m_parent);
	}

	DequeIterator<ValueType, ParentType> operator+(int steps)
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex+steps, m_parent);
	}

	DequeIterator<ValueType, ParentType> const operator+(int steps) const
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex+steps, m_parent);
	}

	DequeIterator<ValueType, ParentType>& operator--()
	{
		--m_currentIndex;
		return *this;
	}

	DequeIterator<ValueType, ParentType> operator--(int)
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex--, m_parent);
	}

	DequeIterator<ValueType, ParentType> const& operator--() const
	{
		--m_currentIndex;
		return *this;
	}

	DequeIterator<ValueType, ParentType> const operator--(int) const
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex--, m_parent);
	}

	DequeIterator<ValueType, ParentType> operator-(int steps)
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex - steps, m_parent);
	}

	DequeIterator<ValueType, ParentType> const operator-(int steps) const
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex - steps, m_parent);
	}

	bool operator==(DequeIterator<ValueType, ParentType> const& rhs) const
	{
		return m_currentIndex == rhs.m_currentIndex;
	}

	bool operator!=(DequeIterator<ValueType, ParentType> const& rhs) const
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

	DequeIterator<ValueType, ParentType> Next()
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex + 1, m_parent);
	}

	DequeIterator<ValueType, ParentType> const Next() const
	{
		return DequeIterator<ValueType, ParentType>(m_currentIndex + 1, m_parent);
	}

	size_t operator-(DequeIterator<ValueType, ParentType> const& other)
	{
		return m_currentIndex - other.m_currentIndex;
	}

	ValueType& operator[](size_t index)
	{
		return m_parent->operator[](m_currentIndex + index);
	}

	ValueType const& operator[](size_t index) const
	{
		return m_parent->operator[](m_currentIndex + index);
	}

	bool operator<(DequeIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentIndex < rhs.m_currentIndex;
	}

	bool operator>(DequeIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentIndex > rhs.m_currentIndex;
	}

	bool operator<=(DequeIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentIndex <= rhs.m_currentIndex;
	}

	bool operator>=(DequeIterator<ValueType, ParentType> const& rhs)
	{
		return m_currentIndex >= rhs.m_currentIndex;
	}

	size_t Index()
	{
		return m_currentIndex;
	}

protected:
	mutable size_t m_currentIndex;
	ParentType* m_parent;
};
