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
#include <algorithm>

#include "List.h"
#include "BlockData.h"
#include "Memory.h"
#include "Chunk.h"
#include "ObjectAddress.h"
#include "OrderedBucketRange.h"

namespace gcix
{
	class OrderedBucketRangeChunkHelper 
	{
	public:
		inline static Chunk* GetEndOfItem(Chunk* chunk)
		{
			return (Chunk*)((uint8_t*)chunk + Constants::ChunkSizeInBytes);
		}
	};

	class OrderedBucketRangeLargeObjectHelper
	{
	public:
		inline static LargeObjectAddress* GetEndOfItem(LargeObjectAddress* object)
		{
			return (LargeObjectAddress*)((uint8_t*)object + object->Size());
		}
	};

	class GlobalAllocator
	{
		friend class Collector;
	public:
		BlockData* RequestBlock(bool requestForEmptyBlock);

		LargeObjectAddress* AllocateLargeObject(uint32_t size, void* classDescriptor);

		inline bool CollectRequested() const
		{
			return collectRequested;
		}

		ObjectAddress* FindObjectConservative(void* ptr);

		static void Initialize()
		{
			if (Instance == nullptr)
			{
				Instance = new GlobalAllocator();
			}
		}

		inline size_t TotalBytesAllocated()
		{
			return totalAllocated;
		}

		inline size_t AllocatedBytesSinceLastCollect()
		{
			return allocatedSinceLastCollect;
		}

		/**
		Clear all marked blocks/objects 
		*/
		void ClearMarked();

		void Recycle();

		static GlobalAllocator* Instance;
		bool collectRequested;
		size_t allocatedSinceLastCollect;

	private:
		GlobalAllocator() : 
			nextRecyclableChunkIndex(-1), 
			nextFreeChunkIndex(-1),
			nextBlockIndexInChunk(0), 
			totalAllocated(0), 
			allocatedSinceLastCollect(0),
			collectRequested(false),
			useRecyclableBlocks(false)
		{
		}

		static const int BucketCount = 512;
		static const int ChunkPerBucketCount = 8;

		/* Gets the total number of block */
		inline uint32_t GetBlockCount()
		{
			return Chunks.Count() * Constants::BlockCountPerChunk;
		}

		/* Gets a blockdata for the specified index */
		inline BlockData* GetBlock(uint32_t index)
		{
			gcix_assert(index < GetBlockCount());

			auto chunkIndex = index >> Constants::BlockCountBitsPerChunk;
			auto blockIndexInChunk = index & Constants::BlockCountPerChunkMask;

			return Chunks[chunkIndex]->GetBlock(blockIndexInChunk);
		}

		inline void AddAllocatedSize(size_t size)
		{
			// Update allocation counters
			totalAllocated += size;
			allocatedSinceLastCollect += size;;
			if (allocatedSinceLastCollect >= Constants::CollectTriggerLimit )
			{
				collectRequested = true;
			}
		}

		OrderedBucketRange<Chunk, OrderedBucketRangeChunkHelper> Chunks;

		OrderedBucketRange<LargeObjectAddress, OrderedBucketRangeLargeObjectHelper> LargeObjects;

		/* Pointer to the current chunk in used for allocation*/
		int32_t nextRecyclableChunkIndex;

		int32_t nextFreeChunkIndex;

		/* Next block index in the chunk */
		int32_t nextBlockIndexInChunk;

		size_t totalAllocated;

		bool useRecyclableBlocks;

		// -----------------------------------------------------------
		// Unit tets
		// -----------------------------------------------------------
		FRIEND_TEST(GlobalAllocator, RequestBlock);
	};
}