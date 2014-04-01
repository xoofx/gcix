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
			return (void*)((uint8_t*)this + Constants::ChunkSizeInBytes - sizeof(void*));
		}

		/**
		Determines whether this chunk is completely free.
		*/
		inline bool IsFree() const
		{
			ChunkHeader& header = Header();
			return header.BlockUnavailableCount == 0 && header.BlockRecyclableCount == 0;
		}

		/**
		Determines whether this chunk contains free blocks.
		*/
		inline bool HasFreeBlocks() const
		{
			ChunkHeader& header = Header();
			return  (((int32_t)header.BlockUnavailableCount + header.BlockRecyclableCount) < GetBlockCount());
		}

		/**
		Determines whether this chunk contains recyclable blocks.
		*/
		inline bool HasRecyclableBlocks() const
		{
			ChunkHeader& header = Header();
			return header.BlockRecyclableCount > 0;
		}

	private:
		friend class GlobalAllocator;
		Chunk() {}

		/**
		Allocate a chunk.
		*/
		static Chunk* Allocate();

		/**
		Delete a chunk.
		*/
		void Delete();

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
				ChunkHeader& header = Header();
				header.BlockRecyclableCount--;
				header.BlockUnavailableCount++;
				// A recyclable block remains in a recyclable state after this call, in order to let LocalAllocator identifying
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
				ChunkHeader& header = Header();
				header.BlockUnavailableCount++;
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
			ChunkHeader& header = Header();
			header.BlockUnavailableCount = 0;
			header.BlockRecyclableCount = 0;

			auto count = GetBlockCount();
			for(int i = 0; i < count; i++)
			{
				auto block = GetBlock(i);
				block->Recycle();
				if (block->IsUnavailable())
				{
					header.BlockUnavailableCount++;
				}
				else if (block->IsRecyclable())
				{
					header.BlockRecyclableCount++;
				}
			}
		}

		/**
		Returns a reference to the chunk header.
		*/
		inline ChunkHeader& Header() const
		{
			return ((BlockData*)this)->Header.Info.Chunk;
		}
	};
}