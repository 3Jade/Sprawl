#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#	define SPRAWL_CONSTEXPR const
#	include <Windows.h>
#else
#	define SPRAWL_CONSTEXPR constexpr
#	include <pthread.h>
#endif

#ifndef SPRAWL_MEMORY_NO_FREE
#	define SPRAWL_MEMORY__RELEASE_EMPTY_BLOCKS 0
#endif

namespace sprawl
{
	//TODO: Put this in separate threading library
	class Mutex
	{
	public:
		inline void lock()
		{
#ifdef _WIN32
			EnterCriticalSection(&m_mutexImpl);
#else
			pthread_mutex_lock(&m_mutexImpl);
#endif
		}
		inline void unlock()
		{
#ifdef _WIN32
			LeaveCriticalSection(&m_mutexImpl);
#else
			pthread_mutex_unlock(&m_mutexImpl);
#endif
		}

		Mutex()
#ifdef _WIN32
			: m_mutexImpl()
		{
			InitializeCriticalSection(&m_mutexImpl);
#else
			: m_mutexImpl(PTHREAD_MUTEX_INITIALIZER)
		{
#endif
		}


		~Mutex()
#ifdef _WIN32
		{
			DeleteCriticalSection(&m_mutexImpl);
#else
		{
			pthread_mutex_destroy(&m_mutexImpl);
#endif
		}
	private:
#ifdef _WIN32
		CRITICAL_SECTION m_mutexImpl;
#else
		pthread_mutex_t m_mutexImpl;
#endif
	};

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
			static sprawl::Mutex ms_allocMutex;
		};

		template<size_t sizeOfType, size_t blockSize>
		MemoryBit<sizeOfType>* DynamicPoolAllocator<sizeOfType, blockSize>::ms_firstBit = nullptr;

		template<size_t sizeOfType, size_t blockSize>
		sprawl::Mutex DynamicPoolAllocator<sizeOfType, blockSize>::ms_allocMutex;

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
			ms_allocMutex.lock();

			MemoryBit<sizeOfType>* bit = ms_firstBit;

			if(bit)
			{
				ms_firstBit = bit->next;
				ms_allocMutex.unlock();
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
			ms_allocMutex.unlock();
			return newBits[0].data;
		}

		template<size_t sizeOfType, size_t blockSize>
		void DynamicPoolAllocator<sizeOfType, blockSize>::free(void* addr)
		{
			void* allocType = reinterpret_cast<void*>(reinterpret_cast<intptr_t*>(addr)[-2]);
			if(allocType == 0)
			{
				ms_allocMutex.lock();
				MemoryBit<sizeOfType>* bit = reinterpret_cast<MemoryBit<sizeOfType>*>(reinterpret_cast<intptr_t*>(addr)-2);

				bit->next = ms_firstBit;
				ms_firstBit = bit;
				ms_allocMutex.unlock();
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
			static sprawl::Mutex ms_allocMutex;
		};

		template<size_t sizeOfType, size_t staticBufferSize>
		MemoryBit<sizeOfType>* StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_firstBit = nullptr;

		template<size_t sizeOfType, size_t staticBufferSize>
		MemoryBit<sizeOfType> StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_blockPool[staticBufferSize];

		template<size_t sizeOfType, size_t staticBufferSize>
		size_t StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_highestBitUsed = 0;

		template<size_t sizeOfType, size_t staticBufferSize>
		sprawl::Mutex StaticPoolAllocator<sizeOfType, staticBufferSize>::ms_allocMutex;

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
			ms_allocMutex.lock();

			MemoryBit<sizeOfType>* bit = ms_firstBit;

			if(bit)
			{
				ms_firstBit = bit->next;
				ms_allocMutex.unlock();
				return bit->data;
			}

			MemoryBit<sizeOfType>* newBit = &ms_blockPool[++ms_highestBitUsed];
			newBit->allocType = 0;

			ms_allocMutex.unlock();
			return newBit->data;
		}

		template<size_t sizeOfType, size_t staticBufferSize>
		void StaticPoolAllocator<sizeOfType, staticBufferSize>::free(void* addr)
		{
			void* allocType = reinterpret_cast<void*>(reinterpret_cast<intptr_t*>(addr)[-2]);
			if(allocType == 0)
			{
				ms_allocMutex.lock();
				MemoryBit<sizeOfType>* bit = reinterpret_cast<MemoryBit<sizeOfType>*>(reinterpret_cast<intptr_t*>(addr)-2);

				bit->next = ms_firstBit;
				ms_firstBit = bit;
				ms_allocMutex.unlock();
			}
			else
			{
				::free(reinterpret_cast<intptr_t*>(addr)-2);
			}
		}
	}
}