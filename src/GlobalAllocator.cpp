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

#include "GlobalAllocator.h"

#include "Collections\List.h"
#include "ObjectFlags.h"
#include "LineFlags.h"
#include "Utility\Memory.h"
#include "Threading\Thread.h"
#include "Threading\ManualResetEvent.h"

namespace gcix
{
	/**
	Initializze the @see GlobalCollector::Instance variable.
	*/
	void GlobalAllocator::Initialize()
	{
		if (Instance == nullptr)
		{
			Memory::Initialize();
			Instance = new GlobalAllocator();
		}
	}

	BlockData* GlobalAllocator::RequestBlock(bool requestForEmptyBlock)
	{
		gcix_assert(GlobalAllocator::Instance != nullptr);

		gcix_lock(mutexChunks);

		// Update allocation counters
		AddAllocatedSize(Constants::TotalChunkSizeInBytes);

		// Allocate from recyclable blocks
		if (useRecyclableBlocks && !requestForEmptyBlock)
		{
			for(; nextRecyclableChunkIndex < Chunks.Count(); nextRecyclableChunkIndex++)
			{
				auto chunk = Chunks[nextRecyclableChunkIndex];
				if (chunk->HasRecyclableBlocks())
				{
					for(; nextBlockIndexInChunk < chunk->GetBlockCount(); nextBlockIndexInChunk++)
					{
						auto block = chunk->GetBlock(nextBlockIndexInChunk);
						if (chunk->TryGetRecyclableBlock(block))
						{
							nextBlockIndexInChunk++;
							return block;
						}
					}
				}
				nextBlockIndexInChunk = 0;
			}

			// We have exhausted all chunks with recyclable blocks
			nextRecyclableChunkIndex = -1;
			useRecyclableBlocks = false;
		}

		// Allocate from free blocks
		if (nextFreeChunkIndex >= 0)
		{
			for(; nextFreeChunkIndex < Chunks.Count(); nextFreeChunkIndex++)
			{
				auto chunk = Chunks[nextFreeChunkIndex];
				if (chunk->HasFreeBlocks())
				{
					for(; nextBlockIndexInChunk < chunk->GetBlockCount(); nextBlockIndexInChunk++)
					{
						auto block = chunk->GetBlock(nextBlockIndexInChunk);
						if (chunk->TryGetFreeBlock(block))
						{
							nextBlockIndexInChunk++;
							return block;
						}
					}
				}
				nextBlockIndexInChunk = 0;
			}
			// We have exhausted all chunks with free blocks
		}

		// Create new chunk and free blocks
		Chunk* chunk = new Chunk();

		// out of memory, early exit
		if (chunk == nullptr)
		{
			return nullptr;
		}

		// Set the current chunk and current block index in the chunk
		nextFreeChunkIndex = Chunks.Count();
		nextBlockIndexInChunk = 0;

		Chunks.Add(chunk);

		return chunk->GetBlock(nextBlockIndexInChunk++);
	}

	void GlobalAllocator::ClearMarked()
	{
		auto count = Chunks.Count();
		for(int i = 0; i < count; i++)
		{
			auto chunk = Chunks[i];
			chunk->ClearMarked();
		}

		// Clear marked for large objects, batch per 4 objects
		count = LargeObjects.Count();
		if (count > 0)
		{
			LargeObjectAddress** pLargeObject = &LargeObjects[0];
			for(int i = 0; i < count/4; i++)
			{
				(*pLargeObject++)->UnMark();
				(*pLargeObject++)->UnMark();
				(*pLargeObject++)->UnMark();
				(*pLargeObject++)->UnMark();
			}
			count &= 3;
			while (count-- > 0)
			{
				(*pLargeObject++)->UnMark();
			}
		}
	}

	void GlobalAllocator::Recycle()
	{
		allocatedSinceLastCollect = 0;
		collectRequested = false;
		nextRecyclableChunkIndex = -1;
		nextFreeChunkIndex = -1;
		nextBlockIndexInChunk = 0;

		// Recycle chunk/blocks
		int freeChunkTotalCount = 0;
		for (int i = 0; i < Chunks.Count(); i++)
		{
			auto chunk = Chunks[i];
			chunk->Recycle();

			if (chunk->HasRecyclableBlocks() && nextRecyclableChunkIndex < 0)
			{
				nextRecyclableChunkIndex = i;
			}
			else if (chunk->HasFreeBlocks())
			{
				if (chunk->IsFree())
				{
					freeChunkTotalCount++;
				}
				if (nextFreeChunkIndex < 0)
				{
					nextFreeChunkIndex = i;
				}
			}
		}

		// Check if we have any recyclable blocks to reuse
		useRecyclableBlocks = nextRecyclableChunkIndex >= 0;

		/*
		// Free chunks that can be completely freed
		if (freeChunkTotalCount > Constants::MinimumFreeChunkToKeepAliveAfterRecycle)
		{
			for (int i = Chunks.Count() - 1; i >= 0; i--)
			{
				auto chunk = Chunks[i];
				if (chunk->IsFree())
				{
					Chunks.Remove(i);
					delete chunk;
					FreeAllocatedSize(Constants::TotalChunkSizeInBytes);
					break;
					//freeChunkTotalCount--;
					//if (freeChunkTotalCount == Constants::MinimumFreeChunkToKeepAliveAfterRecycle)
					//{
					//	break;
					//}
				}
			}

			// Recalculate min/max only after deleting all chunks
			Chunks.ResetMinMax();
		}
		*/

		// Recycle large objects
		bool largeObjectRemoved = false;
		for (int i = LargeObjects.Count() - 1; i >= 0; i--)
		{
			auto largeObject = LargeObjects[i];
			if (!largeObject->IsMarked())
			{
				auto size = largeObject->Size();
				totalAllocated -= size;
				Memory::Free(largeObject);
				LargeObjects.Remove(i);
				largeObjectRemoved = true;
			}
		}
		if (largeObjectRemoved)
		{
			LargeObjects.ResetMinMax();
		}
	}

	LargeObjectAddress* GlobalAllocator::AllocateLargeObject(uint32_t size, void* classDescriptor)
	{
		gcix_assert(GlobalAllocator::Instance != nullptr);
		gcix_assert(size > ObjectConstants::MaxObjectSizePerBlock);
		gcix_assert(classDescriptor != nullptr);

		// Allocate the object with standard allocation, align size of object on 16 bytes
		auto sizeOfLargeObject = Memory::Align(size + ObjectConstants::HeaderTotalSizeInBytes, 16);
		LargeObjectAddress* object = (LargeObjectAddress*)Memory::AllocateZero(sizeOfLargeObject);

		// out of memory, early exit
		if (object == nullptr)
		{
			return nullptr;
		}

		object->Initialize(sizeOfLargeObject);
		object->SetClassDescriptor(classDescriptor);

		// TODO use a separate lock for LOB?
		gcix_lock(mutexLargeObjects);

		LargeObjects.Add(object);
		// Update allocation counters
		AddAllocatedSize(Constants::ChunkSizeInBytes);

		return object;
	}

	ObjectAddress* GlobalAllocator::FindObjectConservative(void* ptr)
	{
		gcix_assert(GlobalAllocator::Instance != nullptr);

		// Discard immediately pointers that are outside the min/max chunk memory range
		if (Chunks.Contains((Chunk*)ptr))
		{
			// Get the chunk bucket associated with this address
			auto bucket = Chunks.GetBucket((Chunk*)ptr);

			// Iterate on chunks
			auto pChunk = &(*bucket)[0];
		
			// Iterate on all chunks
			for(Chunk* chunk; ptr >= (chunk = *pChunk); pChunk++)
			{
				if (ptr < chunk->GetEndOfChunk())
				{
					auto blockData = (BlockData*)((size_t)ptr & Constants::AlignSizeMask);
					auto offsetInBlock = (uint32_t)((size_t)ptr - (size_t)blockData);
					auto lineInBlock = offsetInBlock >> Constants::LineBits;

					for(uint32_t lineIndex = lineInBlock; lineIndex >= Constants::HeaderLineCount; lineIndex--)
					{
						if (blockData->ContainsObject(lineIndex))
						{
							auto object = blockData->GetFirstObject(lineIndex);
							if (ptr < object)
							{
								continue;
							}

							while (true)
							{
								if (StandardObjectAddress::IsInteriorPointerOrNext(object, ptr))
								{
									return object;
								}
								else if (object == nullptr)
								{
									// If there is no other objects, or the next object after the pointer we are checking, 
									// return null
									goto CheckLargeObject;
								}
							} 
						}
					}
					break;
				}
			}
		}

CheckLargeObject:
		// Discard immediately pointers that are outside the min/max memory range for LOB
		if (LargeObjects.Contains((LargeObjectAddress*)ptr))
		{
			// Get the chunk bucket associated with this addrses
			auto bucket = LargeObjects.GetBucket((LargeObjectAddress*)ptr);

			// Iterate on chunks
			auto pObjs = &(*bucket)[0];
		
			// Iterate on all chunks
			for(LargeObjectAddress* obj; ptr >= (obj = *pObjs); pObjs++)
			{
				if (obj->Contains(ptr))
				{
					return obj;
				}
			}
		}

		return nullptr;
	}

	void GlobalAllocator::AddGcRoot(void** gcRoot)
	{
		gcix_assert(gcRoot != nullptr);

		gcix_lock(mutexRoots);
		gcRoots.Add(gcRoot);
	}

	void GlobalAllocator::RemoveGcRoot(void** gcRoot)
	{
		gcix_assert(gcRoot != nullptr);

		gcix_lock(mutexRoots);
		int32_t index;
		if (gcRoots.Find(gcRoot, index))
		{
			gcRoots.Remove(index);
		}
	}

	GlobalAllocator* GlobalAllocator::Instance;
}