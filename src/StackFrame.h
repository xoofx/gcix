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

namespace gcix
{
	class StackFrame
	{
	public:
		StackFrame() : bottomOfStack(nullptr), topOfStack(nullptr) {}

		inline void Initialize()
		{
			bottomOfStack = GetCurrentStack();
		}

		template<typename T>
		gcix_noinline void Capture(T* context)
		{
			gcix_assert(bottomOfStack != nullptr);
			topOfStack = GetCurrentStack();
			context->StackCallback();
		}

		inline void* GetBottomOfStack() const
		{
			return bottomOfStack;
		}

		inline void* GetToOfStack() const
		{
			return topOfStack;
		}
	private:
		gcix_disable_new_delete_operator();

		inline void* GetCurrentStack() const
		{
			volatile int stackHere = 0;
			return (void*)&stackHere;
		}

		void* bottomOfStack;
		void* topOfStack;
	};
}