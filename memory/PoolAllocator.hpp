#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../common/compat.hpp"
#include "../threading/threadlocal.hpp"

#ifndef _WIN32
#	include <unistd.h>
#	include <sys/mman.h>
#endif

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

		template<size_t sizeOfType>
		class PoolAllocator
		{
		public:
			static void* alloc();
			static void* alloc(size_t count);
			static void free(void* addr);

			template<typename T>
			struct rebind
			{
				typedef PoolAllocator<sizeof(T)> otherAllocator;
			};

		private:
			static size_t const ms_blocksPerAlloc;
			static size_t const ms_allocSize;

#if SPRAWL_MULTITHREADED
			static sprawl::threading::ThreadLocal<MemoryBit<sizeOfType>*> ms_firstBit;
#else
			static MemoryBit<sizeOfType>* ms_firstBit;
#endif
			static inline size_t max_(size_t a, size_t b)
			{
				return a < b ? b : a;
			}

			static inline size_t getPageSize_()
			{
#ifdef _WIN32
				SYSTEM_INFO info;
				GetSystemInfo(&info);
				return PoolAllocator::max_(info.dwPageSize, sizeOfType);
#else
				return PoolAllocator::max_(sysconf(_SC_PAGESIZE), sizeOfType);
#endif
			}
		};

#if SPRAWL_MULTITHREADED
		template<size_t sizeOfType>
		sprawl::threading::ThreadLocal<MemoryBit<sizeOfType>*> PoolAllocator<sizeOfType>::ms_firstBit;
#else
		template<size_t sizeOfType>
		MemoryBit<sizeOfType>* PoolAllocator<sizeOfType>::ms_firstBit = nullptr;
#endif

		template<size_t sizeOfType>
		size_t const PoolAllocator<sizeOfType>::ms_blocksPerAlloc = PoolAllocator<sizeOfType>::max_(PoolAllocator<sizeOfType>::getPageSize_() / sizeof(MemoryBit<sizeOfType>), 32);

		template<size_t sizeOfType>
		size_t const PoolAllocator<sizeOfType>::ms_allocSize = ms_blocksPerAlloc * sizeof(MemoryBit<sizeOfType>);

		template<size_t sizeOfType>
		void* PoolAllocator<sizeOfType>::alloc(size_t count)
		{
			void* ret = malloc(count*sizeOfType + (sizeof(intptr_t) * 2));
			memset(ret, 1, sizeof(intptr_t) * 2);
			return reinterpret_cast<unsigned char*>(ret) + (sizeof(intptr_t) * 2);
		}

		template<size_t sizeOfType>
		void* PoolAllocator<sizeOfType>::alloc()
		{
#if SPRAWL_MULTITHREADED
			MemoryBit<sizeOfType>* bit = *ms_firstBit;
#else
			MemoryBit<sizeOfType>* bit = ms_firstBit;
#endif

			if(bit)
			{
				ms_firstBit = bit->next;
				return bit->data;
			}

			MemoryBit<sizeOfType>* newBits = (MemoryBit<sizeOfType>*)
#ifdef _WIN32
				VirtualAlloc(nullptr, ms_allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
				mmap(nullptr, ms_allocSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
			size_t const maxBlock = ms_blocksPerAlloc - 1;
			for(size_t i = 1; i < maxBlock; ++i)
			{
				newBits[i].allocType = 0;
				newBits[i].next = &newBits[i+1];
			}
			newBits[maxBlock].allocType = 0;
			newBits[maxBlock].next = nullptr;

			newBits[0].allocType = 0;

			ms_firstBit = &newBits[1];

			return newBits[0].data;
		}

		template<size_t sizeOfType>
		void PoolAllocator<sizeOfType>::free(void* addr)
		{
			void* allocType = reinterpret_cast<void*>(reinterpret_cast<intptr_t*>(addr)[-2]);
			if(allocType == 0)
			{
				MemoryBit<sizeOfType>* bit = reinterpret_cast<MemoryBit<sizeOfType>*>(reinterpret_cast<intptr_t*>(addr)-2);

#if SPRAWL_MULTITHREADED
				bit->next = *ms_firstBit;
#else
				bit->next = ms_firstBit;
#endif
				ms_firstBit = bit;
			}
			else
			{
				::free(reinterpret_cast<intptr_t*>(addr)-2);
			}
		}
	}
}
