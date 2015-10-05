#pragma once

#include <stdexcept>
#include "iterator/VectorIterator.hpp"
#include "array/Helpers.hpp"
#include "../common/compat.hpp"
#include "string.h"
#include <stdlib.h>

namespace sprawl
{
	namespace collections
	{
		template<typename T>
		class Vector;
	}
}

template<typename T>
class sprawl::collections::Vector
{
public:
	typedef VectorIterator<T, Vector<T>> iterator;
	typedef VectorIterator<T, Vector<T>> const const_iterator;

	explicit Vector(sprawl::collections::Capacity startingCapacity = sprawl::collections::Capacity(16))
		: m_array(startingCapacity.count > 0 ? (T*)(malloc(startingCapacity.count * sizeof(T))) : nullptr)
		, m_size(0)
		, m_capacity(startingCapacity.count)
	{
		//
	}

	Vector(Fill fillCount)
		: m_array(fillCount.count > 0 ? (T*)(malloc(fillCount.count * sizeof(T))) : nullptr)
		, m_size(fillCount.count)
		, m_capacity(fillCount.count)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_array[i]) T();
		}
	}

	Vector(Fill fillCount, T const& value)
		: m_array(fillCount.count > 0 ? (T*)(malloc(fillCount.count * sizeof(T))) : nullptr)
		, m_size(fillCount.count)
		, m_capacity(fillCount.count)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_array[i]) T(value);
		}
	}

	template<typename... Params>
	Vector(Fill fillCount, Params &&... params)
		: m_array(fillCount.count > 0 ? (T*)(malloc(fillCount.count * sizeof(T))) : nullptr)
		, m_size(fillCount.count)
		, m_capacity(fillCount.count)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_array[i]) T(std::forward<Params>(params)...);
		}
	}

	Vector(Vector const& other)
		: m_array(other.m_size > 0 ? (T*)(malloc(other.m_size * sizeof(T))) : nullptr)
		, m_size(other.m_size)
		, m_capacity(other.m_size)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_array[i]) T(other.m_array[i]);
		}
	}

	Vector(Vector&& other)
		: m_array(other.m_array)
		, m_size(other.m_size)
		, m_capacity(other.m_capacity)
	{
		other.m_array = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
	}

	Vector& operator=(Vector const& other)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			m_array[i].~T();
		}
		m_size = 0;
		Reserve(other.m_size);

		m_size = other.m_size;

		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_array[i]) T(other.m_array[i]);
		}
		return *this;
	}

	Vector& operator=(Vector&& other)
	{
		cleanup_();

		m_array = other.m_array;
		m_size = other.m_size;
		m_capacity = other.m_capacity;

		other.m_array = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;

		return *this;
	}

	~Vector()
	{
		cleanup_();
	}

	T& operator[](ssize_t index)
	{
		if (index < 0)
		{
			index += m_size;
		}
		return m_array[index];
	}

	T& At(ssize_t index)
	{
		if (index < 0)
		{
			index += m_size;
		}
		if(index > m_size)
		{
			throw std::out_of_range("sprawl::Vector::At");
		}
		return m_array[index];
	}

	T const& operator[](ssize_t index) const
	{
		if (index < 0)
		{
			index += m_size;
		}
		return m_array[index];
	}

	T const& At(ssize_t index) const
	{
		if (index < 0)
		{
			index += m_size;
		}
		if(index > m_size)
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
		return m_array[m_size -1];
	}

	T const& Front() const
	{
		return m_array[0];
	}

	T const& Back() const
	{
		return m_array[m_size -1];
	}

	void PushBack(T const& value)
	{
		checkGrow_(1);
		new(&m_array[m_size++]) T(value);
	}

	void PushBack(T&& value)
	{
		checkGrow_(1);
		new(&m_array[m_size++]) T(std::move(value));
	}

	template<typename... Params>
	void EmplaceBack(Params &&... params)
	{
		checkGrow_(1);
		new(&m_array[m_size++]) T(std::forward<Params>(params)...);
	}

	T* Data()
	{
		return m_array;
	}

	T const* Data() const
	{
		return m_array;
	}

	void PopBack()
	{
		m_array[--m_size].~T();
	}

	void ShrinkToFit()
	{
		if(m_size != m_capacity)
		{
			m_capacity = m_size;
			reallocate_();
		}
	}

	ssize_t Size() const
	{
		return m_size;
	}

	ssize_t Capacity() const
	{
		return m_capacity;
	}

	void Reserve(ssize_t newCapacity)
	{
		if(newCapacity > m_capacity)
		{
			m_capacity = newCapacity;
			reallocate_();
		}
	}

	void Resize(ssize_t newSize)
	{
		Resize(newSize, T());
	}

	void Resize(ssize_t newSize, T const& value)
	{
		Reserve(newSize);
		//Either delete lost items or create new ones; one loop happens, the other doesn't
		for(ssize_t i = newSize; i < m_size; ++i)
		{
			m_array[i].~T();
		}
		for(ssize_t i = m_size; i < newSize; ++i)
		{
			new(&m_array[i]) T(value);
		}
		m_size = newSize;
	}

	bool Empty()
	{
		return m_size == 0;
	}

	void Clear()
	{
		for(int i = 0; i < m_size; ++i)
		{
			m_array[i].~T();
		}
		m_size = 0;
	}

	void Swap(Vector& other)
	{
		char tmpData[sizeof(Vector)];

		memcpy(tmpData, this, sizeof(Vector));
		memcpy(this, &other, sizeof(Vector));
		memcpy(&other, tmpData, sizeof(Vector));
	}

	void Insert(const_iterator position, T const& value)
	{
		int idx = &position.Value() - m_array;
		shiftRight_(idx, 1);
		new(&m_array[idx]) T(value);
	}

	void Insert(const_iterator position, T&& value)
	{
		int idx = &position.Value() - m_array;
		shiftRight_(idx, 1);
		new(&m_array[idx]) T(std::move(value));
	}

	template<typename... Params>
	void Emplace(const_iterator position, Params &&... params)
	{
		int idx = &position.Value() - m_array;
		shiftRight_(idx, 1);
		new(&m_array[idx]) T(std::forward<Params>(params)...);
	}

	void Erase(const_iterator position)
	{
		int idx = &position.Value() - m_array;
		shiftLeft_(idx, 1);
	}

	iterator begin()
	{
		return iterator(m_array, this);
	}

	iterator end()
	{
		return iterator(m_array + m_size, this);
	}

	const_iterator cbegin() const
	{
		return const_iterator(m_array, this);
	}

	const_iterator cend() const
	{
		return const_iterator(m_array + m_size, this);
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator end() const
	{
		return cend();
	}

private:
	void shiftRight_(ssize_t idx, ssize_t amount)
	{
		checkGrow_(amount);
		for(ssize_t i = (m_size + amount) - 1; i > idx; --i)
		{
			if(idx >= m_size)
			{
				new(&m_array[i]) T(std::move(m_array[i-amount]));
			}
			else
			{
				m_array[i] = std::move(m_array[i-amount]);
			}
		}
		m_size += amount;
	}

	void shiftLeft_(ssize_t idx, ssize_t amount)
	{
		for(ssize_t i = idx; i < m_size - amount; ++i)
		{
			m_array[i] = std::move(m_array[i + amount]);
		}
		for(ssize_t i = m_size-amount; i < m_size; ++i)
		{
			m_array[i].~T();
		}
		m_size -= amount;
	}

	void checkGrow_(ssize_t amount)
	{
		if(m_size + amount > m_capacity)
		{
			grow_(amount);
		}
	}

	void grow_(ssize_t amount)
	{
		m_capacity = m_size + (amount > m_size ? amount : m_size);
		reallocate_();
	}

	void reallocate_()
	{
		T* newArray = (T*)(malloc(m_capacity * sizeof(T)));
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new (&newArray[i]) T(std::move(m_array[i]));
			m_array[i].~T();
		}
		free(m_array);
		m_array = newArray;
	}

	void cleanup_()
	{
		if(m_array)
		{
			for(ssize_t i = 0; i < m_size; ++i)
			{
				m_array[i].~T();
			}
			free(m_array);
			m_array = nullptr;
		}
	}

	T* m_array;
	ssize_t m_size;
	ssize_t m_capacity;
};
