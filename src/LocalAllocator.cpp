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

#include "LocalAllocator.h"
#include "GlobalAllocator.h"
#include "Marker.h"

namespace gcix
{
	gcix_thread_local LocalAllocator* LocalAllocator::Instance;

	gcix_noinline void LocalAllocator::StackCallback()
	{
		gcix_assert(LocalAllocator::Instance != nullptr);
		gcix_assert(GlobalAllocator::Instance != nullptr);

		// printf("Stack base:%p, top %p\n", stack.GetBottomOfStack(), stack.GetToOfStack());

		auto startPointer = (uint32_t*)stack.GetToOfStack();
		auto endPointer = (uint32_t*)stack.GetBottomOfStack();
		// TODO: Add statistics

		GlobalAllocator::Instance->ClearMarked();

		auto globalAllocator = GlobalAllocator::Instance;
		int marked = 0;
		while (startPointer < endPointer)
		{
			auto pointerToCheck = *(void**)startPointer;

			auto object = globalAllocator->FindObjectConservative(pointerToCheck);

			if (object != nullptr)
			{
				Marker::Mark(object);
				marked++;
			}
			startPointer++;
		}

		// TODO: Add statistics

		GlobalAllocator::Instance->Recycle();
	}

	void LocalAllocator::Collect()
	{
		stack.Capture(this);
	}


	StandardObjectAddress* LocalAllocator::Allocate(uint32_t sizeInBytes, void* classDescriptor)
	{
		gcix_assert(LocalAllocator::Instance != nullptr);
		gcix_assert(GlobalAllocator::Instance != nullptr);
		gcix_assert(classDescriptor != nullptr);

		if (GlobalAllocator::Instance->CollectRequested())
		{
			stack.Capture(this);

			// Reset this allocator current block in order to fetch a new block
			current = nullptr;
			overflow = nullptr;
		}

		gcix_assert(sizeInBytes > 0 
			&& (sizeInBytes - ObjectFlags::HeaderSize - ObjectFlags::AdditionalHeaderOffset) < Constants::BlockSizeInBytes);

		// Align to 4 bytes
		sizeInBytes = Utility::Align(sizeInBytes, 4);

		// Total size to allocate
		uint32_t totalSizeInBytes = sizeInBytes + ObjectFlags::HeaderSize + ObjectFlags::AdditionalHeaderOffset;
		auto isMediumSizedObject = totalSizeInBytes > Constants::LineSizeInBytes;

		// Start with current block handler
		BlockData** pBlockData = &current;
		bool requestOverflow = false;

		while (true)
		{
			BlockData* blockData = *pBlockData;

			// If no block allocated yet, we have to allocate a new one
			if (!blockData)
			{
				goto allocateBlock;
			}

			// Calculate the line index
			uint32_t& bumpCursor = blockData->Header.Info.BumpCursor;
			uint32_t& bumpCursorLimit = blockData->Header.Info.BumpCursorLimit;

			// Next position
			uint32_t bumpCursorEnd = bumpCursor + totalSizeInBytes;

			// ------------------------------------------------
			// If we cannot allocate into the current block
			// we have to allocate into new one
			// ------------------------------------------------
			if (bumpCursorEnd & Constants::BlockSizeInBytesInverseMask)
			{
				goto allocateBlock;
			}

			// ------------------------------------------------
			// Recyclable block, try to find a hole
			// ------------------------------------------------
			if (blockData->IsRecyclable() && bumpCursorEnd > bumpCursorLimit)
			{
				// If we are trying to allocate a medium object and a current hole exist but is not enough large,
				// then switch to overflow block handler
				if (isMediumSizedObject && bumpCursorLimit)
				{
					pBlockData = &overflow;
					continue;
				}

				uint32_t newCursorLineIndex = 0;
				uint32_t newCursorLimitLineIndex = 0;

				// Number of lines we are expecting to find
				uint32_t expectedLineCounts = (totalSizeInBytes + Constants::LineSizeInBytes - 1) >> Constants::LineBits;

				// The indexo of the first line we are going to try to find a range of free lines
				uint32_t newLineIndex = (bumpCursorLimit ? 
					(bumpCursorLimit + 1): 
					bumpCursor) >> Constants::LineBits;

				// Find a hole
				auto pFlags = blockData->Header.LineFlags;
				for(uint32_t i = newLineIndex; i < Constants::LineCount; i++)
				{
					LineFlags& lineFlags = pFlags[i];
					if ((lineFlags & LineFlags::Marked) != 0)
					{
						if (newCursorLineIndex > 0  && expectedLineCounts <= (i - newCursorLineIndex))
						{
							newCursorLimitLineIndex = i;	
							goto holeFound;
						}
						newCursorLineIndex = 0;
					}
					else
					{
						if (newCursorLineIndex == 0)
						{
							newCursorLineIndex = i;
						}
						// Clear line flags
						lineFlags = LineFlags::Empty;
					}
				}
				// If we can fit 
				if (newCursorLineIndex > 0 && 
					(expectedLineCounts <= (Constants::LineCount - newCursorLineIndex)))
				{
					newCursorLimitLineIndex = Constants::LineCount;
				}
				else
				{
					// For Medium Object, If first allocation into a hole fails, switch to overflow block handler
					if (isMediumSizedObject)
					{
						pBlockData = &overflow;
						continue;
					}

					// If we can't find a hole into the current block, we have to allocate a new block
					goto allocateBlock;
				}
holeFound:
				bumpCursor = newCursorLineIndex << Constants::LineBits;
				bumpCursorLimit = newCursorLimitLineIndex << Constants::LineBits;
			}

			// ------------------------------------------------
			//  Bump allocation
			// ------------------------------------------------
			StandardObjectAddress* object = (StandardObjectAddress*)((uint8_t*)blockData->Lines + bumpCursor);
			uint8_t offsetInLine = bumpCursor & Constants::LineSizeInBytesMask;
			uint32_t lineIndex = bumpCursor >> Constants::LineBits;
			LineFlags& lineFlags = blockData->Header.LineFlags[lineIndex];

			// Set the object header + add the hashcode
			// uint32_t hashCode = (((uint32_t)lineData) + (((uint32_t)lineData) >> 15)) * 16807;
			object->Initialize(sizeInBytes);
			object->SetClassDescriptor(classDescriptor);

			if ((lineFlags & LineFlags::ContainsObject) == 0)
			{
				lineFlags = (LineFlags)(offsetInLine | (uint8_t)LineFlags::ContainsObject);
			}

			// Advance the current position to the next memory slot
			bumpCursor += totalSizeInBytes;

			return object;

			// ------------------------------------------------
			//  Get or create the next block
			// ------------------------------------------------
allocateBlock:
			// Gets a new block for the current handler
			*pBlockData = GlobalAllocator::Instance->RequestBlock(pBlockData == &overflow);

			// If new block is null, then we are running out of space, return nullptr
			if (*pBlockData == nullptr)
			{
				return nullptr;
			}
		}
	}
}

