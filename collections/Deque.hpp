#pragma once

#include "array/Helpers.hpp"
#include "iterator/DequeIterator.hpp"
#include <stdexcept>
#include <stdint.h>
#include "../common/compat.hpp"
#include <stdlib.h>


namespace sprawl
{
	namespace collections
	{
		template<typename T>
		class Deque;
	}
}

template<typename T>
class sprawl::collections::Deque
{
public:
	typedef DequeIterator<T, Deque<T>> iterator;
	typedef DequeIterator<T, Deque<T>> const_iterator;

	Deque(sprawl::collections::Capacity startingCapacity = sprawl::collections::Capacity(16))
		: m_circleBuffer((T*)(malloc(startingCapacity.count * sizeof(T))))
		, m_size(0)
		, m_capacity(startingCapacity.count)
		, m_zeroIndex(0)
	{

	}

	Deque(sprawl::collections::Fill fillCount)
		: m_circleBuffer((T*)(malloc(fillCount.count * sizeof(T))))
		, m_size(fillCount.count)
		, m_capacity(fillCount.count)
		, m_zeroIndex(0)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_circleBuffer[i]) T();
		}
	}

	Deque(sprawl::collections::Fill fillCount, T const& value)
		: m_circleBuffer((T*)(malloc(fillCount.count * sizeof(T))))
		, m_size(fillCount.count)
		, m_capacity(fillCount.count)
		, m_zeroIndex(0)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_circleBuffer[i]) T(value);
		}
	}

	template<typename... Params>
	Deque(sprawl::collections::Fill fillCount, Params&& ... params)
		: m_circleBuffer((T*)(malloc(fillCount.count * sizeof(T))))
		, m_size(fillCount.count)
		, m_capacity(fillCount.count)
		, m_zeroIndex(0)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_circleBuffer[i]) T(std::forward<Params>(params)...);
		}
	}

	Deque(Deque const& other)
		: m_circleBuffer(other.m_size > 0 ? (T*)(malloc(other.m_size * sizeof(T))) : nullptr)
		, m_size(other.m_size)
		, m_capacity(other.m_size)
		, m_zeroIndex(0)
	{
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_circleBuffer[i]) T(other[i]);
		}
	}

	Deque(Deque&& other)
		: m_circleBuffer(other.m_circleBuffer)
		, m_size(other.m_size)
		, m_capacity(other.m_capacity)
		, m_zeroIndex(other.m_zeroIndex)
	{
		other.m_circleBuffer = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_zeroIndex = 0;
	}

	Deque& operator=(Deque const& other)
	{
		cleanup_();
		m_size = 0;
		Reserve(other.m_size);

		m_size = other.m_size;
		m_zeroIndex = 0;

		for(ssize_t i = 0; i < m_size; ++i)
		{
			new(&m_circleBuffer[i]) T(other[i]);
		}
		return *this;
	}

	Deque& operator=(Deque&& other)
	{
		cleanup_();

		m_circleBuffer = other.m_circleBuffer;
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		m_zeroIndex = other.m_zeroIndex;

		other.m_circleBuffer = nullptr;
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_zeroIndex = 0;

		return *this;
	}

	~Deque()
	{
		cleanup_();
	}

	T& operator[](ssize_t index)
	{
		if (index < 0)
		{
			index += m_size;
		}
		return m_circleBuffer[translateIndex_(index)];
	}

	T& At(ssize_t index)
	{
		if (index < 0)
		{
			index += m_size;
		}
		if(index > m_size)
		{
			throw std::out_of_range("sprawl::Deque::At");
		}
		ssize_t const translated = translateIndex_(index);
		return m_circleBuffer[translated];
	}

	T const& operator[](ssize_t index) const
	{
		if (index < 0)
		{
			index += m_size;
		}
		return m_circleBuffer[translateIndex_(index)];
	}

	T const& At(ssize_t index) const
	{
		if (index < 0)
		{
			index += m_size;
		}
		if(index > m_size)
		{
			throw std::out_of_range("sprawl::Deque::At");
		}
		ssize_t const translated = translateIndex_(index);
		return m_circleBuffer[translated];
	}

	T& Front()
	{
		return m_circleBuffer[m_zeroIndex];
	}

	T& Back()
	{
		return m_circleBuffer[translateIndex_(m_size -1)];
	}

	T const& Front() const
	{
		return m_circleBuffer[m_zeroIndex];
	}

	T const& Back() const
	{
		return m_circleBuffer[translateIndex_(m_size -1)];
	}

	void PushBack(T const& value)
	{
		checkGrow_(1);
		new(&m_circleBuffer[translateIndex_(m_size++)]) T(value);
	}

	void PushBack(T&& value)
	{
		checkGrow_(1);
		new(&m_circleBuffer[translateIndex_(m_size++)]) T(std::move(value));
	}

	template<typename... Params>
	void EmplaceBack(Params &&... params)
	{
		checkGrow_(1);
		new(&m_circleBuffer[translateIndex_(m_size++)]) T(std::forward<Params>(params)...);
	}

	void PushFront(T const& value)
	{
		checkGrow_(1);
		m_zeroIndex = translateIndex_(-1);
		new(&m_circleBuffer[m_zeroIndex]) T(value);
		++m_size;
	}

	void PushFront(T&& value)
	{
		checkGrow_(1);
		m_zeroIndex = translateIndex_(-1);
		new(&m_circleBuffer[m_zeroIndex]) T(std::move(value));
		++m_size;
	}

	template<typename... Params>
	void EmplaceFront(Params &&... params)
	{
		checkGrow_(1);
		m_zeroIndex = translateIndex_(-1);
		new(&m_circleBuffer[m_zeroIndex]) T(std::forward<Params>(params)...);
		++m_size;
	}

	void PopBack()
	{
		m_circleBuffer[translateIndex_(--m_size)].~T();
	}

	void PopFront()
	{
		m_circleBuffer[m_zeroIndex].~T();
		++m_zeroIndex;
		if(m_zeroIndex >= m_capacity)
		{
			m_zeroIndex = 0;
		}
		--m_size;
	}

	void ShrinkToFit()
	{
		if(m_size != m_capacity)
		{
			ssize_t newCapacity = m_size;
			reallocate_(newCapacity);
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
			reallocate_(newCapacity);
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
			m_circleBuffer[translateIndex_(i)].~T();
		}
		for(ssize_t i = m_size; i < newSize; ++i)
		{
			new(&m_circleBuffer[translateIndex_(i)]) T(value);
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
			m_circleBuffer[translateIndex_(i)].~T();
		}
		m_size = 0;
	}

	void Swap(Deque& other)
	{
		char tmpData[sizeof(Deque)];

		memcpy(tmpData, this, sizeof(Deque));
		memcpy(this, &other, sizeof(Deque));
		memcpy(&other, tmpData, sizeof(Deque));
	}

	void Insert(const_iterator position, T const& value)
	{
		int idx = position.Index();
		shiftRight_(idx, 1);
		new(&m_circleBuffer[translateIndex_(idx)]) T(value);
	}

	void Insert(const_iterator position, T&& value)
	{
		int idx = position.Index();
		shiftRight_(idx, 1);
		new(&m_circleBuffer[translateIndex_(idx)]) T(std::move(value));
	}

	template<typename... Params>
	void Emplace(const_iterator position, Params &&... params)
	{
		int idx = position.Index();
		shiftRight_(idx, 1);
		new(&m_circleBuffer[translateIndex_(idx)]) T(std::forward<Params>(params)...);
	}

	void Erase(const_iterator position)
	{
		int idx = position.Index();
		shiftLeft_(idx, 1);
	}

	iterator begin()
	{
		return iterator(0, this);
	}

	iterator end()
	{
		return iterator(m_size, this);
	}

	const_iterator cbegin() const
	{
		return const_iterator(0, this);
	}

	const_iterator cend() const
	{
		return const_iterator(m_size, this);
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
	ssize_t translateIndex_(int index) const
	{
		ssize_t translated = m_zeroIndex + index;
		if(translated < 0)
		{
			translated += m_capacity;
		}
		else if(translated >= m_capacity)
		{
			translated -= m_capacity;
		}
		return translated;
	}

	ssize_t reverseIndex_(T* element) const
	{
		ssize_t const offsetFromFront = element - m_circleBuffer;
		ssize_t adjustedOffset = offsetFromFront - m_zeroIndex;
		if(adjustedOffset < 0)
		{
			adjustedOffset += m_capacity;
		}
		return adjustedOffset;
	}

	void shiftRight_(ssize_t idx, ssize_t amount)
	{
		checkGrow_(amount);
		for(ssize_t i = (m_size + amount) - 1; i > idx; --i)
		{
			if(idx >= m_size)
			{
				new(&m_circleBuffer[translateIndex_(i)]) T(std::move(m_circleBuffer[translateIndex_(i - amount)]));
			}
			else
			{
				m_circleBuffer[translateIndex_(i)] = std::move(m_circleBuffer[translateIndex_(i - amount)]);
			}
		}
		m_size += amount;
	}

	void shiftLeft_(ssize_t idx, ssize_t amount)
	{
		for(ssize_t i = idx; i < m_size - amount; ++i)
		{
			m_circleBuffer[translateIndex_(i)] = std::move(m_circleBuffer[translateIndex_(i + amount)]);
		}
		for(ssize_t i = m_size-amount; i < m_size; ++i)
		{
			m_circleBuffer[translateIndex_(i)].~T();
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
		ssize_t newCapacity = m_size + (amount > m_size ? amount : m_size);
		reallocate_(newCapacity);
	}

	void reallocate_(ssize_t newCapacity)
	{
		T* newArray = (T*)(malloc(newCapacity * sizeof(T)));
		for(ssize_t i = 0; i < m_size; ++i)
		{
			new (&newArray[i]) T(std::move(m_circleBuffer[translateIndex_(i)]));
			m_circleBuffer[translateIndex_(i)].~T();
		}
		free(m_circleBuffer);
		m_circleBuffer = newArray;
		m_zeroIndex = 0;
		m_capacity = newCapacity;
	}

	void cleanup_()
	{
		if(m_circleBuffer)
		{
			for(ssize_t i = 0; i < m_size; ++i)
			{
				m_circleBuffer[translateIndex_(i)].~T();
			}
			free(m_circleBuffer);
			m_circleBuffer = nullptr;
		}
	}

	T* m_circleBuffer;
	ssize_t m_size;
	ssize_t m_capacity;
	ssize_t m_zeroIndex;
};
