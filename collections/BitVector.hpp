#pragma once

#include "../common/compat.hpp"
#include "../string/StringBuilder.hpp"
#include "../string/String.hpp"
#include <limits>
#include <stdlib.h>

namespace sprawl
{
	namespace collections
	{
		template<uint64_t maxBits>
		class BitSet;
		class BitVector;
	}
}

template<uint64_t maxBits>
class sprawl::collections::BitSet
{
public:
	BitSet()
		: m_bitArray()
	{
		Reset();
	}

	void Set(uint64_t bit)
	{
		size_t offset = getOffset_(bit);
		m_bitArray[offset] |= (uint64_t(1) << bit);
	}

	void Unset(uint64_t bit)
	{
		size_t offset = getOffset_(bit);
		m_bitArray[offset] &= ~(uint64_t(1) << bit);
	}

	void Flip(uint64_t bit)
	{
		size_t offset = getOffset_(bit);
		m_bitArray[offset] ^= (uint64_t(1) << bit);
	}

	bool HasBit(uint64_t bit) const
	{
		size_t offset = getOffset_(bit);
		return ((m_bitArray[offset] & (uint64_t(1) << bit)) != 0);
	}

	SPRAWL_CONSTEXPR uint64_t Size() const
	{
		return maxBits;
	}

	bool Any() const
	{
		uint64_t testArray[sizeof(m_bitArray) / sizeof(uint64_t)];
		memset(testArray, 0, sizeof(testArray));
		return (memcmp(testArray, m_bitArray, sizeof(m_bitArray)) != 0);
	}

	bool None() const
	{
		return !Any();
	}

	bool All() const
	{
		//Memcmp would be nice here, but since we might have extra unused bits,
		//getting the right value to compare against is a bit tough.
		for(size_t i = 0; i < Size(); ++i)
		{
			if(!HasBit(i))
			{
				return false;
			}
		}
		return true;
	}

	void Reset()
	{
		memset(m_bitArray, 0, sizeof(m_bitArray));
	}

	uint64_t Count() const
	{
		uint64_t result = 0;
		size_t const numIndexes = sizeof(m_bitArray)/sizeof(int64_t);
		for(size_t i = 0; i < numIndexes; ++i)
		{
			result += countBitsSet_(m_bitArray[i]);
		}
		return result;
	}

	sprawl::String ToString()
	{
		sprawl::StringBuilder builder(maxBits, false);
		for(size_t i = maxBits; i > 0; --i)
		{
			if(HasBit(i-1))
			{
				builder << "1";
			}
			else
			{
				builder << "0";
			}
		}
		return builder.Str();
	}

private:
	uint64_t countBitsSet_(uint64_t value) const
	{
		uint64_t result = 0;
		for(uint64_t i = 0; i < 64; ++i)
		{
			if((value & (uint64_t(1) << i)) != 0)
			{
				++result;
			}
		}
		return result;
	}

	size_t getOffset_(uint64_t& bit) const
	{
		size_t offset = bit / 64;
		bit = bit % 64;
		return offset;
	}

	uint64_t m_bitArray[(maxBits / 64) + 1];
};


class sprawl::collections::BitVector
{
public:
	BitVector()
		: m_bitArray(new uint64_t[1])
		, m_size(0)
	{
		Reset();
	}

	~BitVector()
	{
		delete[] m_bitArray;
	}

	BitVector(BitVector const& other)
		: m_bitArray(new uint64_t[other.m_size / 64 + 1])
		, m_size(other.m_size)
	{
		memcpy(m_bitArray, other.m_bitArray, (other.m_size / 64 + 1) * sizeof(uint64_t));
	}

	BitVector(BitVector&& other)
		: m_bitArray(other.m_bitArray)
		, m_size(other.m_size)
	{
		other.m_bitArray = new uint64_t[1];
		other.m_size = 0;
	}

	BitVector& operator=(BitVector const& other)
	{
		m_bitArray = new uint64_t[other.m_size / 64 + 1];
		m_size = other.m_size;
		memcpy(m_bitArray, other.m_bitArray, (other.m_size / 64 + 1) * sizeof(uint64_t));
		return *this;
	}

	BitVector& operator=(BitVector&& other)
	{
		m_bitArray = other.m_bitArray;
		m_size = other.m_size;
		other.m_bitArray = new uint64_t[1];
		other.m_size = 0;
		return *this;
	}

	void Set(uint64_t bit)
	{
		size_t offset = getOffset_(bit);
		checkGrow_(offset, bit);
		m_bitArray[offset] |= (uint64_t(1) << bit);
	}

	void Unset(uint64_t bit)
	{
		size_t offset = getOffset_(bit);
		checkGrow_(offset, bit);
		m_bitArray[offset] &= ~(uint64_t(1) << bit);
	}

	void Flip(uint64_t bit)
	{
		size_t offset = getOffset_(bit);
		checkGrow_(offset, bit);
		m_bitArray[offset] ^= (uint64_t(1) << bit);
	}

	bool HasBit(uint64_t bit) const
	{
		size_t offset = getOffset_(bit);
		if(offset > m_size / 64)
		{
			return false;
		}
		return ((m_bitArray[offset] & (uint64_t(1) << bit)) != 0);
	}

	uint64_t Size() const
	{
		return m_size;
	}

	bool Any() const
	{
		uint64_t* testArray = (uint64_t*)alloca((m_size / 64 + 1) * sizeof(uint64_t));
		memset(testArray, 0, (m_size / 64 + 1) * sizeof(uint64_t));
		return (memcmp(testArray, m_bitArray, ((m_size / 64) + 1) * sizeof(uint64_t)) != 0);
	}

	bool None() const
	{
		return !Any();
	}

	bool All() const
	{
		//Memcmp would be nice here, but since we might have extra unused bits,
		//getting the right value to compare against is a bit tough.
		for(size_t i = 0; i < Size(); ++i)
		{
			if(!HasBit(i))
			{
				return false;
			}
		}
		return true;
	}

	void Reset()
	{
		memset(m_bitArray, 0, ((m_size / 64) + 1) * sizeof(uint64_t));
	}

	uint64_t Count() const
	{
		uint64_t result = 0;
		size_t const numIndexes = m_size / 64 + 1;
		for(size_t i = 0; i < numIndexes; ++i)
		{
			result += countBitsSet_(m_bitArray[i]);
		}
		return result;
	}

	sprawl::String ToString()
	{
		sprawl::StringBuilder builder(m_size, false);
		for(size_t i = m_size; i > 0; --i)
		{
			if(HasBit(i-1))
			{
				builder << "1";
			}
			else
			{
				builder << "0";
			}
		}
		return builder.Str();
	}

private:
	void checkGrow_(uint64_t newOffset, uint64_t bit)
	{
		if(newOffset > (m_size / 64))
		{
			uint64_t* newArray = new uint64_t[newOffset + 1];
			memset(newArray, 0, (newOffset + 1) * sizeof(uint64_t));
			memcpy(newArray, m_bitArray, ((m_size / 64) + 1) * sizeof(uint64_t));
			delete[] m_bitArray;
			m_bitArray = newArray;
		}

		uint64_t const fullBit = newOffset * 64 + bit;

		if(m_size <= fullBit)
		{
			m_size = fullBit + 1;
		}
	}

	uint64_t countBitsSet_(uint64_t value) const
	{
		uint64_t result = 0;
		for(uint64_t i = 0; i < 64; ++i)
		{
			if((value & (uint64_t(1) << i)) != 0)
			{
				++result;
			}
		}
		return result;
	}

	size_t getOffset_(uint64_t& bit) const
	{
		size_t offset = bit / 64;
		bit = bit % 64;
		return offset;
	}

	uint64_t* m_bitArray;
	size_t m_size;
};
