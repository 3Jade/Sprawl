#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../common/compat.hpp"
#include "../threading/mutex.hpp"

#ifndef SPRAWL_MEMORY_NO_FREE
#	define SPRAWL_MEMORY__RELEASE_EMPTY_BLOCKS 0
#endif

namespace sprawl
{
	namespace memory
	{
		template<size_t sizeOfType>
		struct MemoryBit
		{
			void* allocType;
			MemoryBit* next;
			unsigned char data[sizeOfType];
		};

		template<size_t sizeOfType, size_t blockSize = 32>
		class DynamicPoolAllocator
		{
		public:
			static void* alloc();
			static void* alloc(size_t count);
			static void free(void* addr);

			template<typename T>
			struct rebind
			{
				typedef DynamicPoolAllocator<sizeof(T), blockSize> otherAllocator;
			};

		private:
			static SPRAWL_CONSTEXPR size_t adjustedBlockSize = (blockSize+7) & ~7;
			static MemoryBit<sizeOfType>* ms_firstBit;

#if SPRAWL_MULTITHREADED
			static sprawl::threading::Mutex ms_allocMutex;
#endif
		};

		template<size_t sizeOfType, size_t blockSize>
		MemoryBit<sizeOfType>* DynamicPoolAllocator<sizeOfType, blockSize>::ms_firstBit = nullptr;

#if SPRAWL_MULTITHREADED
		template<size_t sizeOfType, size_t blockSize>
		sprawl::threading::Mutex DynamicPoolAllocator<sizeOfType, blockSize>::ms_allocMutex;
#endif

		template<size_t sizeOfType, size_t blockSize>
		void* DynamicPoolAllocator<sizeOfType, blockSize>::alloc(size_t count)
		{
			void* ret = malloc(count*sizeOfType + (sizeof(intptr_t) * 2));
			memset(ret, 1, sizeof(intptr_t) * 2);
			return reinterpret_cast<unsigned char*>(ret) + (sizeof(intptr_t) * 2);
		}

		template<size_t sizeOfType, size_t blockSize>
		void* DynamicPoolAllocator<sizeOfType, blockSize>::alloc()
		{
#if SPRAWL_MULTITHREADED
			sprawl::threading::ScopedLock lock(ms_allocMutex);
#endif

			MemoryBit<sizeOfType>* bit = ms_firstBit;

			if(bit)
			{
				ms_firstBit = bit->next;
				return bit->data;
			}

			MemoryBit<sizeOfType>* newBits = (MemoryBit<sizeOfType>*)malloc(sizeof(MemoryBit<sizeOfType>) * adjustedBlockSize);
			for(size_t i = 1; i < adjustedBlockSize; ++i)
			{
				newBits[i].next = ms_firstBit;
				newBits[i].allocType = 0;
				ms_firstBit = &newBits[i];
			}
			newBits[0].allocType = 0;

			return newBits[0].data;
		}

		template<size_t sizeOfType, size_t blockSize>
		void DynamicPoolAllocator<sizeOfType, blockSize>::free(void* addr)
		{
			void* allocType = reinterpret_cast<void*>(reinterpret_cast<intptr_t*>(addr)[-2]);
			if(allocType == 0)
			{
#if SPRAWL_MULTITHREADED
				sprawl::threading::ScopedLock lock(ms_allocMutex);
#endif
				MemoryBit<sizeOfType>* bit = reinterpret_cast<MemoryBit<sizeOfType>*>(reinterpret_cast<intptr_t*>(addr)-2);

				bit->next = ms_firstBit;
				ms_firstBit = bit;
			}
			else
			{
				::free(reinterpret_cast<intptr_t*>(addr)-2);
			}
		}

		template<size_t sizeOfType, size_t staticBufferSize = 1024>
		class StaticPoolAllocator
		{
		public:
			static void* alloc();
			static void* alloc(size_t count);
			static void free(void* addr);

			template<typename T>
			struct rebind
			{
				typedef StaticPoolAllocator<sizeof(T), staticBufferSize> otherAllocator;
			};
		private:
			static MemoryBit<sizeOfType>* ms_firstBit;
			static MemoryBit<sizeOfType> ms_blockPool[staticBufferSize];
			static size_t ms_highestBitUsed;
#if SPRAWL_MULTITHREADED
			static sprawl::threading::Mutex ms_allocMutex;
#endif
		};

		template<size_t sizeOfType, size_t staticBufferSize>
		MemoryBit<sizeOfType>* StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_firstBit = nullptr;

		template<size_t sizeOfType, size_t staticBufferSize>
		MemoryBit<sizeOfType> StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_blockPool[staticBufferSize];

		template<size_t sizeOfType, size_t staticBufferSize>
		size_t StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_highestBitUsed = 0;

#if SPRAWL_MULTITHREADED
		template<size_t sizeOfType, size_t staticBufferSize>
		sprawl::threading::Mutex StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_allocMutex;
#endif

		template<size_t sizeOfType, size_t staticBufferSize>
		void* StaticPoolAllocator<sizeOfType, staticBufferSize>::alloc(size_t count)
		{
			void* ret = malloc(count*sizeOfType + (sizeof(intptr_t) * 2));
			memset(ret, 1, sizeof(intptr_t) * 2);
			return reinterpret_cast<unsigned char*>(ret) + (sizeof(intptr_t) * 2);
		}

		template<size_t sizeOfType, size_t staticBufferSize>
		void* StaticPoolAllocator<sizeOfType, staticBufferSize>::alloc()
		{
#if SPRAWL_MULTITHREADED
			sprawl::threading::ScopedLock lock(ms_allocMutex);
#endif

			MemoryBit<sizeOfType>* bit = ms_firstBit;

			if(bit)
			{
				ms_firstBit = bit->next;
				return bit->data;
			}

			MemoryBit<sizeOfType>* newBit = &ms_blockPool[++ms_highestBitUsed];
			newBit->allocType = 0;

			return newBit->data;
		}

		template<size_t sizeOfType, size_t staticBufferSize>
		void StaticPoolAllocator<sizeOfType, staticBufferSize>::free(void* addr)
		{
			void* allocType = reinterpret_cast<void*>(reinterpret_cast<intptr_t*>(addr)[-2]);
			if(allocType == 0)
			{
#if SPRAWL_MULTITHREADED
				sprawl::threading::ScopedLock lock(ms_allocMutex);
#endif
				MemoryBit<sizeOfType>* bit = reinterpret_cast<MemoryBit<sizeOfType>*>(reinterpret_cast<intptr_t*>(addr)-2);

				bit->next = ms_firstBit;
				ms_firstBit = bit;
			}
			else
			{
				::free(reinterpret_cast<intptr_t*>(addr)-2);
			}
		}
	}
}
