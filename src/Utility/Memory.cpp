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

#include "Utility\Memory.h"
#ifdef GCIX_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#else
#include <algorithm>
#endif

namespace gcix
{
#ifdef GCIX_PLATFORM_WINDOWS
	static HANDLE processHeapHandle;

	void Memory::Initialize()
	{
		if (processHeapHandle == nullptr)
		{
			processHeapHandle = ::GetProcessHeap();
		}
	}

	void Memory::Free(void* ptr)
	{
		gcix_assert(processHeapHandle != nullptr);
		::HeapFree(processHeapHandle, 0, ptr);
	}

	void* Memory::Allocate(size_t size)
	{
		gcix_assert(processHeapHandle != nullptr);
		return ::HeapAlloc(processHeapHandle, 0, size);
	}

	void* Memory::AllocateZero(size_t size)
	{
		gcix_assert(processHeapHandle != nullptr);
		return ::HeapAlloc(processHeapHandle, HEAP_ZERO_MEMORY, size);
	}

	void* Memory::ReAllocate(void* ptr, size_t size)
	{
		gcix_assert(processHeapHandle != nullptr);
		return ptr == nullptr ? ::HeapAlloc(processHeapHandle, 0, size) : ::HeapReAlloc(processHeapHandle, 0, ptr, size);
	}
#else
	void Memory::Initialize()
	{
	}

	void Memory::Free(void* ptr)
	{
		return free(ptr);
	}

	void* Memory::Allocate(size_t size)
	{
		return malloc(size);
	}

	void* Memory::AllocateZero(size_t size)
	{
		void* ptr = malloc(size);
		if (ptr == nullptr)
		{
			return ptr;
		}
		memset(ptr, 0, size);
		return ptr;
	}

	void* Memory::ReAllocate(void* ptr, size_t size)
	{
		return realloc(ptr, size);
	}
#endif
}