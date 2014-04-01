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

#include "gtest/gtest.h"

#include "GlobalAllocator.h"

namespace gcix
{
	TEST(GlobalAllocator, RequestBlock)
	{
		gcix::GlobalAllocator::Initialize();

		auto instance = gcix::GlobalAllocator::Instance;
		EXPECT_NE(nullptr, instance);

		// Check internals
		EXPECT_EQ(0, instance->Chunks.Count());

		// Request a memory block
		auto blockData0 = instance->RequestBlock(false);
		EXPECT_EQ(1, instance->Chunks.Count());
		EXPECT_EQ(true, blockData0->IsFree());
		EXPECT_EQ(false, blockData0->IsRecyclable());
		EXPECT_EQ(false, blockData0->IsUnavailable());

		// Gets the chunk
		auto chunk0 = instance->Chunks[0];

		// Check that a chunk has the same address as the first block data
		EXPECT_EQ((void*)chunk0, (void*)blockData0);

		// Check block count
		EXPECT_EQ(Constants::BlockCountPerChunk, chunk0->GetBlockCount());

		for (int i = 0; i < chunk0->GetBlockCount(); i++)
		{
			auto block = chunk0->GetBlock(i);

			if (i == 0)
			{
				EXPECT_EQ(block, blockData0);
			}

			// Check that all block data are aligned on a block size
			EXPECT_EQ(0, (intptr_t)block & gcix::Constants::BlockSizeInBytesInverseMask);
		}

		// Check Bump cursor in BlockData, must be equal to the header size
		EXPECT_EQ(Constants::HeaderSizeInBytes, blockData0->Header.Info.BumpCursor);

		// Check BumpCursorLimit = 0
		EXPECT_EQ(0, blockData0->Header.Info.BumpCursorLimit);
	}
};
