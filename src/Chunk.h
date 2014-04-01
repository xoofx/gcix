#pragma once
// Copyright (c) 2014, Alexandre Mutel
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following 
// conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
//    disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
//    disclaimer in the documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Common.h"
#include "Constants.h"
#include "BlockData.h"

namespace gcix
{
	/**
	A chunk contains several continous blocks in memory, aligned on a block size in memory.
	*/
	class Chunk
	{
	public:
		/**
		Header of a chunk.
		*/
		ChunkHeader Header;

		/**
		Gets the number of blocks this chunk contains.
		*/
		inline int32_t GetBlockCount() const
		{
			return (int32_t)Constants::BlockCountPerChunk;
		}

		/**
		Gets the block for the specified index. Index must be < to @see GetBlockCount()
		*/
		inline BlockData* GetBlock(int32_t index) const
		{
			gcix_assert(index < GetBlockCount());
			return &((BlockData*)this)[index];
		}


		inline void* GetEndOfChunk() const
		{
			return (void*)((intptr_t)this + Constants::ChunkSizeInBytes - sizeof(void*));
		}

		/**
		Determines whether this chunk is completely free.
		*/
		inline bool IsFree() const
		{
			return Header.BlockUnavailableCount == 0 && Header.BlockRecyclableCount == 0;
		}

		/**
		Determines whether this chunk contains free blocks.
		*/
		inline bool HasFreeBlocks() const
		{
			return  (((int32_t)Header.BlockUnavailableCount + Header.BlockRecyclableCount) < GetBlockCount());
		}

		/**
		Determines whether this chunk contains recyclable blocks.
		*/
		inline bool HasRecyclableBlocks() const
		{
			return Header.BlockRecyclableCount > 0;
		}

	private:
		friend class GlobalAllocator;
		// Nothing in the default constructor, as everything is done in the custom new operator
		Chunk() {} 

		/**
		Overrides new operator for chunk for handling special alignment and initialization.
		*/
		static void* operator new (size_t size)
		{
			// We are not using the original size of the chunk, as a chunk is a special block of memory
			auto rawChunk = Memory::AllocateZero(Constants::ChunkSizeInBytes + Constants::BlockSizeInBytes);

			// out of memory, early exit
			if (rawChunk == nullptr)
			{
				return nullptr;
			}

			// Align on a Block size boundary
			auto chunk = (Chunk*)(((intptr_t)rawChunk + Constants::BlockSizeInBytes - 1)
				& (~((intptr_t)(1 << Constants::BlockBits) - 1)));

			// Initialize all blocks
			for (int i = 0; i < chunk->GetBlockCount(); i++)
			{
				chunk->GetBlock(i)->Initialize();
			}

			// Set the chunk header AFTER block are initialized (as block initialize clear all information)
			chunk->Header.AllocationOffset = (int32_t)((intptr_t)rawChunk - (intptr_t)chunk);
			chunk->Header.BlockUnavailableCount = 0;
			chunk->Header.BlockRecyclableCount = 0;

			// TODO: Reuse space lost by alignment as a storage for an additionnal LargeObjectSpace for objects with a size 
			// < Constants::BlockSizeInBytes

			return (void*)chunk;
		}

		/**
		Overrides delete operator for chunk. 
		*/
		static void operator delete (void *p)
		{
			// Add gcix_assert here.
			Chunk* chunk = (Chunk*)p;
			auto rawChunk = (intptr_t)p + chunk->Header.AllocationOffset;
			Memory::Free((void*)rawChunk);
		}
		/**
		Clear marked flags on blocks.
		*/
		inline void ClearMarked()
		{
			auto count = GetBlockCount();
			for(int i = 0; i < count; i++)
			{
				GetBlock(i)->ClearMarked();
			}
		}

		/**
		Test if the block is recyclable, return true and update internal statistics. The block should then be used for 
		recyclable allocation.
		@param block the block to check if it is recyclable.
		*/
		inline bool TryGetRecyclableBlock(BlockData* block)
		{
			gcix_assert(block != nullptr);
			if (block->IsRecyclable())
			{
				Header.BlockRecyclableCount--;
				Header.BlockUnavailableCount++;
				// A recyclable block remains in a recyclable state after this call, in order to let ThreadLocalAllocator identifying
				// them
				return true;
			}
			return false;
		}

		/**
		Test if the block is recyclable, return true and update internal statistics. The block should then be used for 
		recyclable allocation.
		@param block the block to check if it is recyclable.
		*/
		inline bool TryGetFreeBlock(BlockData* block)
		{
			gcix_assert(block != nullptr);
			if (block->IsFree())
			{
				Header.BlockUnavailableCount++;
				// A free block will be unavailable after this call.
				block->SetFlags(BlockFlags::Unavailable);
				return true;
			}
			return false;
		}

		/**
		Recycle all blocks and update internal block and chunk statistics.
		*/
		inline void Recycle()
		{
			Header.BlockUnavailableCount = 0;
			Header.BlockRecyclableCount = 0;

			auto count = GetBlockCount();
			for(int i = 0; i < count; i++)
			{
				auto block = GetBlock(i);
				block->Recycle();
				if (block->IsUnavailable())
				{
					Header.BlockUnavailableCount++;
				}
				else if (block->IsRecyclable())
				{
					Header.BlockRecyclableCount++;
				}
			}
		}
	};
}