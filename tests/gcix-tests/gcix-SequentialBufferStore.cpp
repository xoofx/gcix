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

#include "Collections\SequentialStoreBuffer.h"

namespace gcix
{
	class SequentialStoreBufferTest : public ::testing::Test
    {
    public:
        virtual void SetUp() {}
        virtual void TearDown() {}
    };

    /**
    Check internal state of GlobalAllocator after calling GlobalAllocator::RequestBlock 
    */
	TEST_F(SequentialStoreBufferTest, TestAllocation)
	{
		gcix::DefaultSequentialStoreBufferAllocator allocator(128);
		gcix::DefaultSequentialStoreBufferHandle handler(&allocator);

		auto nextBuffer = handler.buffer;
		ASSERT_NE(nullptr, nextBuffer);
		ASSERT_EQ(0, (intptr_t)nextBuffer & (gcix::DefaultSequentialStoreBufferAllocator::Size - 1));

		int pointerCount = gcix::DefaultSequentialStoreBufferAllocator::Size / sizeof(void*) - 3;

		for (int i = 0; i < pointerCount; i++)
		{
			handler.Push((void*)(i + 1));
		}

		auto nextBuffer2 = handler.buffer;
		ASSERT_NE(nextBuffer, nextBuffer2);
		ASSERT_EQ(0, (intptr_t)nextBuffer2 & (gcix::DefaultSequentialStoreBufferAllocator::Size - 1));

		for (int i = pointerCount; i >= 1; i--)
		{
			auto ptr = (intptr_t)handler.Pop();
			ASSERT_EQ((intptr_t)i, ptr);
		}

		ASSERT_NE(nextBuffer, handler.buffer);

		ASSERT_EQ(nullptr, handler.Pop());
		ASSERT_NE(nextBuffer, handler.buffer);

		ASSERT_EQ(nullptr, handler.Pop());
		ASSERT_NE(nextBuffer, handler.buffer);
	}
};
