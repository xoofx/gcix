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

#include "Threading\Thread.h"
#ifdef GCIX_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

namespace gcix
{

#ifdef GCIX_PLATFORM_WINDOWS
	Thread::Thread(ThreadRunDelegate task, void *context) : thread(task, context)
	{
	}

	Thread::~Thread()
	{
	}

	/**
	Locks this mutex.
	*/
	void Thread::Join()
	{
		thread.join();
	}

#else
	Thread::Thread()
	{
	}

	Thread::~Thread()
	{
	}

	/**
	Locks this mutex.
	*/
	void Thread::Join()
	{
		mutex.lock();
	}

#endif

}

