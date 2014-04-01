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
    class GlobalAllocatorTest : public ::testing::Test
    {
    public:
        virtual void SetUp() {}
        virtual void TearDown() {}
    };



    // Check internal state of GlobalAllocator after calling GlobalAllocator::RequestBlock 
   
    TEST_F(GlobalAllocatorTest, RequestBlock)
	{
		auto instance = gcix::GlobalAllocator::Instance;
		ASSERT_NE(nullptr, instance);

		// Check internals
		ASSERT_EQ(0, instance->Chunks.Count());

		// Request a memory block
		auto blockData0 = instance->RequestBlock(false);
		ASSERT_EQ(1, instance->Chunks.Count());

		// Gets the chunk
		auto chunk0 = instance->Chunks[0];
		ASSERT_NE(nullptr, chunk0);

		// Check that a chunk has the same address as the first block data
		EXPECT_EQ((void*)chunk0, (void*)blockData0);

        EXPECT_TRUE(chunk0->IsFree());
        EXPECT_TRUE(chunk0->HasFreeBlocks());
        EXPECT_FALSE(chunk0->HasRecyclableBlocks());

		// Check block count per chunk
		EXPECT_EQ(Constants::BlockCountPerChunk, chunk0->GetBlockCount());

		// Check allocated blocks
		for (int i = 0; i < chunk0->GetBlockCount(); i++)
		{
			auto block = chunk0->GetBlock(i);

			if (i == 0)
			{
				EXPECT_EQ(block, blockData0);
			}

			EXPECT_TRUE(block->IsFree());
			EXPECT_FALSE(block->IsRecyclable());
			EXPECT_FALSE(block->IsUnavailable());

			// Check that all block data are aligned on a block size
			EXPECT_EQ(0, (intptr_t)block & Constants::BlockSizeInBytesMask);

            // Check line flags are empty
            for (uint32_t lineIndex = Constants::HeaderLineCount; lineIndex < Constants::LineCount; lineIndex++)
            {
                EXPECT_EQ(LineFlags::Empty, block->Header.LineFlags[lineIndex]);

                // Check line datas are empty
                for (uint32_t columnIndex = 0; columnIndex < Constants::LineSizeInBytes; columnIndex++)
                {
                    EXPECT_EQ(0, block->Lines[lineIndex][columnIndex]);
                }
            }
		}

		// Check Bump cursor in BlockData, must be equal to the header size
		EXPECT_EQ(Constants::HeaderSizeInBytes, blockData0->Header.Info.BumpCursor);

		// Check BumpCursorLimit = 0
		EXPECT_EQ(0, blockData0->Header.Info.BumpCursorLimit);

		// Check Chunk min/max memory
		auto minChunkAddress = chunk0;
		auto maxChunkAddress = (Chunk*)((intptr_t)chunk0 + Constants::BlockSizeInBytes * Constants::BlockCountPerChunk);

		// Check pointers inside the chunk
		EXPECT_TRUE(instance->Chunks.Contains(chunk0));
		EXPECT_TRUE(instance->Chunks.Contains((Chunk*)((intptr_t)maxChunkAddress - 1)));

		// Check pointers outside the chunk
		EXPECT_FALSE(instance->Chunks.Contains((Chunk*)((intptr_t)chunk0 - 1)));
		EXPECT_FALSE(instance->Chunks.Contains(maxChunkAddress));

		// Allocate remaining blocks into the chunk
		for (int i = 1; i < chunk0->GetBlockCount(); i++)
		{
			auto nextBlock = instance->RequestBlock(false);
			EXPECT_TRUE(instance->Chunks.Contains((Chunk*)nextBlock));
		}

		// Reallocate a new block from a new chunk
		auto nextBlockOfNextChunj = instance->RequestBlock(false);
		auto nextChunk = (Chunk*)nextBlockOfNextChunj;
		EXPECT_TRUE(nextChunk < minChunkAddress || nextChunk > maxChunkAddress);

		// Recycle freshly allocated block, as they have not been marked, the nextChunk must be freed by the recycle
		// while the firstChunk must be kept for future allocation.
		instance->Recycle();
		EXPECT_EQ(1, instance->Chunks.Count());

		// TODO Add tests after recycle

	}
};
