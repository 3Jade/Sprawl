#pragma once

#include <stdexcept>
#include "iterator/VectorIterator.hpp"
#include "array/Helpers.hpp"
#include "../common/compat.hpp"
#include "string.h"

namespace sprawl
{
	namespace collections
	{
		template<typename T, ssize_t size>
		class Array;
	}
}

template<typename T, ssize_t size>
class sprawl::collections::Array
{
public:
	typedef VectorIterator<T, Array<T, size>> iterator;
	typedef VectorIterator<T, Array<T, size>> const const_iterator;

	T& operator[](ssize_t index)
	{
		if (index < 0)
		{
			index += size;
		}
		return m_array[index];
	}

	T& At(ssize_t index)
	{
		if (index < 0)
		{
			index += size;
		}
		if(index > size)
		{
			throw std::out_of_range("sprawl::Vector::At");
		}
		return m_array[index];
	}

	T const& operator[](ssize_t index) const
	{
		if (index < 0)
		{
			index += size;
		}
		return m_array[index];
	}

	T const& At(ssize_t index) const
	{
		if (index < 0)
		{
			index += size;
		}
		if(index > size)
		{
			throw std::out_of_range("sprawl::Vector::At");
		}
		return m_array[index];
	}

	T& Front()
	{
		return m_array[0];
	}

	T& Back()
	{
		return m_array[size -1];
	}

	T const& Front() const
	{
		return m_array[0];
	}

	T const& Back() const
	{
		return m_array[size -1];
	}

	T* Data()
	{
		return m_array;
	}

	ssize_t Size()
	{
		return size;
	}

	bool Empty()
	{
		return size == 0;
	}

	iterator begin()
	{
		return iterator(m_array, this);
	}

	iterator end()
	{
		return iterator(m_array + size, this);
	}

	const_iterator cbegin() const
	{
		return const_iterator(m_array, this);
	}

	const_iterator cend() const
	{
		return const_iterator(m_array + size, this);
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator end() const
	{
		return cend();
	}

	void Fill(T const& value)
	{
		for(ssize_t i = 0; i < size; ++i)
		{
			m_array[i] = value;
		}
	}

private:
	T m_array[size];
};
