#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#	define SPRAWL_CONSTEXPR const
#else
#	define SPRAWL_CONSTEXPR constexpr
#endif

#ifndef SPRAWL_MEMORY_NO_FREE
#	define SPRAWL_MEMORY__RELEASE_EMPTY_BLOCKS 0
#endif

namespace sprawl
{
	namespace memory
	{
		template<size_t sizeOfType, size_t blockSize>
		struct MemoryBlock
		{
			void Init()
			{
				data = header + paddedHeaderSize;
				end = data + dataSize;
				for(size_t i = 0; i < blockSize; ++i)
				{
					size_t offset = i * (singleBlockSize);
					*reinterpret_cast<intptr_t*>(data + offset) = reinterpret_cast<intptr_t>(this);
				}

				//At creation the allocator returns the address of data directly.
				allocs = 1;
				next = nullptr;

				memset(header, 0, paddedHeaderSize);
				header[0] = 1;
			}

			bool full()
			{
				return allocs == blockSize;
			}

			void* TakeFirstEmptySlot();

			static SPRAWL_CONSTEXPR size_t headerSize = ((blockSize/8));
			static SPRAWL_CONSTEXPR size_t paddedHeaderSize = ((headerSize + 7) & ~7) + 32;
			static SPRAWL_CONSTEXPR size_t singleBlockSize = sizeOfType + sizeof(MemoryBlock<sizeOfType, blockSize>*);
			static SPRAWL_CONSTEXPR size_t dataSize = singleBlockSize * blockSize;
			static SPRAWL_CONSTEXPR size_t dataSizeWithHeader = dataSize + paddedHeaderSize;

			unsigned char header[dataSizeWithHeader];
			unsigned char* data;
			unsigned char* end;
			MemoryBlock* next;

			int allocs;
		};

		template<size_t sizeOfType, size_t blockSize>
		void* MemoryBlock<sizeOfType, blockSize>::TakeFirstEmptySlot()
		{
			for( size_t i = 0; i < headerSize; ++i )
			{
				unsigned char& c = *(header + i);
				if( c == 0xFF )
					continue;

				for( size_t j = 0; j < 8; ++j )
				{
					unsigned char bits = (1 << j);
					if( ( c & bits ) == 0 )
					{
						c |= bits;
						++allocs;
						return data + (singleBlockSize * (i * 8 + j)) + sizeof(MemoryBlock<sizeOfType, blockSize>*);
					}
				}
			}
			return nullptr;
		}



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
			static MemoryBlock<sizeOfType, adjustedBlockSize>* ms_first_block;
		};

		template<size_t sizeOfType, size_t blockSize>
		MemoryBlock<sizeOfType, DynamicPoolAllocator<sizeOfType, blockSize>::adjustedBlockSize>* DynamicPoolAllocator<sizeOfType, blockSize>::ms_first_block = nullptr;

		template<size_t sizeOfType, size_t blockSize>
		void* DynamicPoolAllocator<sizeOfType, blockSize>::alloc(size_t count)
		{
			void* ret = malloc(count*sizeOfType + sizeof(intptr_t));
			memset(ret, 0, sizeof(intptr_t));
			return reinterpret_cast<unsigned char*>(ret) + sizeof(intptr_t);
		}

		template<size_t sizeOfType, size_t blockSize>
		void* DynamicPoolAllocator<sizeOfType, blockSize>::alloc()
		{
			MemoryBlock<sizeOfType, adjustedBlockSize>* currentBlock = ms_first_block;

			if(currentBlock)
			{
				void* ret = currentBlock->TakeFirstEmptySlot();
				if(currentBlock->full())
				{
					ms_first_block = currentBlock->next;
					currentBlock->next = nullptr;
				}
				return ret;
			}

			ms_first_block = (MemoryBlock<sizeOfType, adjustedBlockSize>*)malloc(sizeof(MemoryBlock<sizeOfType, adjustedBlockSize>));
			ms_first_block->Init();
			return ms_first_block->data + sizeof(MemoryBlock<sizeOfType, adjustedBlockSize>*);
		}

		template<size_t sizeOfType, size_t blockSize>
		void DynamicPoolAllocator<sizeOfType, blockSize>::free(void* addr)
		{
			MemoryBlock<sizeOfType, adjustedBlockSize>* currentBlock = reinterpret_cast<MemoryBlock<sizeOfType, adjustedBlockSize>*>(reinterpret_cast<intptr_t*>(addr)[-1]);

			if(currentBlock)
			{
				size_t slot = static_cast<size_t>(reinterpret_cast<unsigned char*&>(addr) - currentBlock->data) / MemoryBlock<sizeOfType, adjustedBlockSize>::singleBlockSize;
				size_t offset = slot / 8;
				slot %= 8;
				*(currentBlock->header + offset) &= ~(1 << slot);
#if SPRAWL_MEMORY__RELEASE_EMPTY_BLOCKS
				if(--currentBlock->allocs == 0)
				{
					currentBlock->prev->next = currentBlock->next;
					currentBlock->next->prev = currentBlock->prev;
					if(currentBlock == ms_first_block)
					{
						ms_first_block = currentBlock->next;
					}
					::free(currentBlock);
				}
				else
#endif
				{
					currentBlock->next = ms_first_block;
					ms_first_block = currentBlock;
				}
			}
			else
			{
				::free(reinterpret_cast<intptr_t*>(addr)-1);
			}
		}

		template<size_t sizeOfType, size_t blockSize = 32, size_t numBlocks = 512>
		class StaticPoolAllocator
		{
		public:
			static void* alloc();
			static void* alloc(size_t count);
			static void free(void* addr);

			template<typename T>
			struct rebind
			{
				typedef StaticPoolAllocator<sizeof(T), blockSize, numBlocks> otherAllocator;
			};
		private:
			static SPRAWL_CONSTEXPR size_t adjustedBlockSize = (blockSize+7) & ~7;
			static MemoryBlock<sizeOfType, adjustedBlockSize>* ms_first_block;
			static MemoryBlock<sizeOfType, adjustedBlockSize> ms_blockPool[numBlocks];
			static size_t ms_currentBlock;
		};

		template<size_t sizeOfType, size_t blockSize, size_t numBlocks>
		MemoryBlock<sizeOfType, StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::adjustedBlockSize>* StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::ms_first_block = nullptr;

		template<size_t sizeOfType, size_t blockSize, size_t numBlocks>
		MemoryBlock<sizeOfType, StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::adjustedBlockSize> StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::ms_blockPool[numBlocks];

		template<size_t sizeOfType, size_t blockSize, size_t numBlocks>
		size_t StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::ms_currentBlock = 0;

		template<size_t sizeOfType, size_t blockSize, size_t numBlocks>
		void* StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::alloc(size_t count)
		{
			void* ret = malloc(count*sizeOfType + sizeof(intptr_t));
			memset(ret, 0, sizeof(intptr_t));
			return reinterpret_cast<unsigned char*>(ret) + sizeof(intptr_t);
		}

		template<size_t sizeOfType, size_t blockSize, size_t numBlocks>
		void* StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::alloc()
		{
			MemoryBlock<sizeOfType, adjustedBlockSize>* currentBlock = ms_first_block;

			if(currentBlock)
			{
				void* ret = currentBlock->TakeFirstEmptySlot();
				if(currentBlock->full())
				{
					ms_first_block = currentBlock->next;
					currentBlock->next = nullptr;
				}
				return ret;
			}

			ms_first_block = ms_blockPool + ms_currentBlock;
			++ms_currentBlock;
			ms_first_block->Init();
			return ms_first_block->data + sizeof(MemoryBlock<sizeOfType, adjustedBlockSize>*);
		}

		template<size_t sizeOfType, size_t blockSize, size_t numBlocks>
		void StaticPoolAllocator<sizeOfType, blockSize, numBlocks>::free(void* addr)
		{
			MemoryBlock<sizeOfType, adjustedBlockSize>* currentBlock = reinterpret_cast<MemoryBlock<sizeOfType, adjustedBlockSize>*>(reinterpret_cast<intptr_t*>(addr)[-1]);

			if(currentBlock)
			{
				size_t slot = static_cast<size_t>(reinterpret_cast<unsigned char*&>(addr) - currentBlock->data) / MemoryBlock<sizeOfType, adjustedBlockSize>::singleBlockSize;
				size_t offset = slot / 8;
				slot %= 8;
				*(currentBlock->header + offset) &= ~(1 << slot);

				currentBlock->next = ms_first_block;
				ms_first_block = currentBlock;
			}
			else
			{
				::free(reinterpret_cast<intptr_t*>(addr)-1);
			}
		}


	}
}
