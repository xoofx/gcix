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

#include "gcix.h"
#include "Constants.h"
#include "ObjectConstants.h"
#include "GlobalAllocator.h"
#include "ThreadLocalAllocator.h"

namespace gcix
{
	static_assert(StandardObjectMaxSizeInBytes == ObjectConstants::MaxObjectSizePerBlock, "Size [StandardObjectMaxSizeInBytes]"
		" declared in gcix.h doesn't match size declared in [ObjectConstants::MaxObjectSizePerBlock)");

	void Initialize()
	{
		GlobalAllocator::Initialize();
	}

	/**
	Initialize the current mutator thread. Must be called from any threads (including the main one) that is going to perform
	managed allocation.
	*/
	void InitializeMutatorThread()
	{
		gcix_assert(GlobalAllocator::Instance != nullptr);
		ThreadLocalAllocator::Initialize();
	}

	/**
	Allocates a standard size managed object.
	@param size Size in bytes of the object. Must be > 0 and <= StandardObjectMaxSizeInBytes
	@param userClassDescriptor Pointer to the object class descriptor that will be setup on the header of the object. Cannot be
	null.
	*/
	void* AllocateStandardObject(uint32_t size, void* userClassDescriptor)
	{
		gcix_assert(GlobalAllocator::Instance != nullptr);
		gcix_assert(ThreadLocalAllocator::Instance != nullptr);
		auto objectAddress = ThreadLocalAllocator::Instance->Allocate(size, userClassDescriptor);
		return objectAddress->ToUserObject();
	}

	/**
	Allocates a large size managed object.
	@param size Size in bytes of the object. Must be > StandardObjectMaxSizeInBytes
	@param userClassDescriptor Pointer to the object class descriptor that will be setup on the header of the object. Cannot be
	*/
	void* AllocateLargeObject(uint32_t size, void* userClassDescriptor)
	{
		gcix_assert(GlobalAllocator::Instance != nullptr);
		auto objectAddress = ThreadLocalAllocator::Instance->AllocateLargeObject(size, userClassDescriptor);
		return objectAddress->ToUserObject();
	}
}
