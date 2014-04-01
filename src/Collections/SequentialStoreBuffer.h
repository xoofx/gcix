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
#include "Utility\Memory.h"
#include "Threading\Mutex.h"
#include "Collections\List.h"

namespace gcix
{
	template<int TSize = 4096, int TChunkCount = 8>
	class SequentialStoreBuffer;

	template<int TSize = 4096, int TChunkCount = 8>
	class SequentialStoreBufferHandle;

	template<int TSize = 4096, int TChunkCount = 8>
	class SequentialStoreBufferAllocator
	{
	public:
		const static int Size = TSize;

		typedef SequentialStoreBuffer<TSize, TChunkCount>* SequentialStoreBufferPtr;

		SequentialStoreBufferAllocator(int capacity) : Chunks(capacity), FreeBuffers(capacity), nextIndexInChunk(0)
		{
		}

		~SequentialStoreBufferAllocator()
		{
			for (int i = 0; i < Chunks.Count(); i++)
			{
				Memory::Free(Chunks[i]);
			}
		}

		gcix_overrides_new_delete();
	private:


		friend class SequentialStoreBufferHandle<TSize, TChunkCount>;

		SequentialStoreBufferPtr GetNextBuffer()
		{
			gcix_lock(mutexChunks);
			int freeBufferCount = FreeBuffers.Count() - 1;
			if (freeBufferCount >= 0)
			{
				auto ssb = FreeBuffers[freeBufferCount];
				ssb->Initialize();
				FreeBuffers.Remove(freeBufferCount);
				return ssb;
			}

			void* chunk = Chunks.Count() > 0 ? Chunks[Chunks.Count() - 1] : nullptr;

			if (chunk == nullptr || nextIndexInChunk == TChunkCount)
			{
				chunk = Memory::Allocate(TSize * (TChunkCount + 1));
				Chunks.Add(chunk);
				nextIndexInChunk = 0;
			}

			SequentialStoreBufferPtr ssbs = (SequentialStoreBufferPtr)(((intptr_t)chunk + TSize - 1) & ~((intptr_t)TSize - 1));
			if (nextIndexInChunk == 0)
			{
				for (int i = 0; i < TChunkCount; i++)
				{
					ssbs[i].Initialize();
				}
			}

			return &ssbs[nextIndexInChunk++];
		}

		void Recycle(SequentialStoreBufferPtr buffer)
		{
			gcix_lock(mutexChunks);
			FreeBuffers.Add(buffer);
		}

		Mutex mutexChunks;
		List<void*> Chunks;
		List<SequentialStoreBufferPtr> FreeBuffers;
		int nextIndexInChunk;

		// -----------------------------------------------------------
		// Unit tets
		// -----------------------------------------------------------
		FRIEND_TEST(SequentialStoreBufferTest, TestAllocation);

		static_assert(TSize > (sizeof(void*)* 2) && (TSize & (TSize - 1)) == 0, "Invalid TSize. Must be power of two and > sizeof(void*)");
	};

	/**
	Main memory class handling memory allocation in gcix
	*/
	template<int TSize = 4096, int TChunkCount = 8>
	class SequentialStoreBuffer
	{
	private:
		SequentialStoreBuffer<TSize, TChunkCount>* Previous;
		void** Next;
		void*  Empty; // initialize to nullptr
		void*  Pointers[TSize / sizeof(void*)-3];

		SequentialStoreBuffer() {}
		gcix_disable_new_delete_operator();

		friend class SequentialStoreBufferHandle<TSize, TChunkCount>;
		friend class SequentialStoreBufferAllocator<TSize, TChunkCount>;

		inline void Initialize()
		{
			Previous = nullptr;
			Empty = nullptr;
			Next = Pointers;
		}

		inline bool IsFull() const
		{
			return ((intptr_t)Next & (TSize - 1)) == 0;
		}

		inline bool IsEmpty() const
		{
			return Next == Pointers;
		}

		inline void Push(void* pointer)
		{
			*Next++ = pointer;
		}

		inline void* Pop()
		{
			Next--;
			auto ptr = *Next;
			if (ptr == nullptr)
			{
				Next++;
			}
			return ptr;
		}

		inline void SeekToEnd()
		{
			Next = (void**)((intptr_t)this + TSize);
		}

		// -----------------------------------------------------------
		// Unit tets
		// -----------------------------------------------------------
		FRIEND_TEST(SequentialStoreBufferTest, TestAllocation);
	};

	template<int TSize = 4096, int TChunkCount = 8>
	class SequentialStoreBufferHandle
	{
	private:
		SequentialStoreBufferAllocator<TSize, TChunkCount>* allocator;
		SequentialStoreBuffer<TSize, TChunkCount>* buffer;
	public:
		SequentialStoreBufferHandle(SequentialStoreBufferAllocator<TSize, TChunkCount>* allocatorArg) : allocator(allocatorArg)
		{
			buffer = allocator->GetNextBuffer();
		}

		~SequentialStoreBufferHandle()
		{
			allocator->Recycle(buffer);
		}

		inline void Push(void* pointer)
		{
			gcix_assert(pointer != nullptr);
			buffer->Push(pointer);
			if (buffer->IsFull())
			{
				Overflow();
			}
		}

		inline void* Pop()
		{
			auto ptr = buffer->Pop();
			if (ptr == nullptr)
			{
				ptr = Recycle();
			}
			return ptr;
		}
	private:
		SequentialStoreBufferHandle() {}

		gcix_noinline void Overflow()
		{
			auto previous = buffer;
			buffer->Next = (void**)allocator->GetNextBuffer();
			buffer = (SequentialStoreBuffer<TSize, TChunkCount>*)buffer->Next;
			buffer->Previous = previous;
		}

		gcix_noinline void* Recycle()
		{
			auto previous = buffer->Previous;
			buffer->Previous = nullptr;
			if (previous == nullptr)
			{
				return nullptr;
			}
			allocator->Recycle(buffer);
			buffer = previous;
			buffer->SeekToEnd();
			return buffer->Pop();
		}

		// -----------------------------------------------------------
		// Unit tets
		// -----------------------------------------------------------
		FRIEND_TEST(SequentialStoreBufferTest, TestAllocation);
	};


	typedef SequentialStoreBufferAllocator<4096> DefaultSequentialStoreBufferAllocator;
	typedef SequentialStoreBufferHandle<4096> DefaultSequentialStoreBufferHandle;

}