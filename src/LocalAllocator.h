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

#include "ObjectFlags.h"
#include "LineFlags.h"
#include "Constants.h"
#include "BlockData.h"
#include "Memory.h"
#include "Utility.h"
#include "Stack.h"
#include "ObjectAddress.h"

namespace gcix
{
	class LocalAllocator
	{
	public:
		StandardObjectAddress* Allocate(uint32_t sizeInBytes, void* classDescriptor);
		BlockData* current;
		BlockData* overflow;

		static void Initialize()
		{
			if (Instance == nullptr)
			{
				Instance = new LocalAllocator();
				Instance->Initialize();
			}
		}

		void Collect();

		gcix_thread_local static LocalAllocator* Instance;
	private:
		LocalAllocator() : current(nullptr), overflow(nullptr) 
		{
			stack.Initialize();
		}
		gcix_noinline void StackCallback();
		friend class Stack;

		Stack stack;
	};
}