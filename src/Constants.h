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
	/**
	Constants used by the allocator
	Warning, all the values here are not really intended to be modified
	*/
	class Constants
	{
	public:
		/** Block in bit size = 16 bits ~ 65536 bytes */
		static const uint32_t BlockBits = 16;

		/** Line in bit size = 8 bits ~ 256 bytes */
		static const uint32_t LineBits = 8;

		static const uint32_t LineCountBits = BlockBits - LineBits;

		/** Number of lines in a block = 256 */
		static const uint32_t LineCount = 1 << LineCountBits;
		
		/** Number of lines in the header = 2  */
		static const uint32_t HeaderLineCount = (LineCount >> LineBits) * 2;

		/** Number of lines effectively available in a block = 256 - 2 lines = 254  */
		static const uint32_t EffectiveLineCount = LineCount - HeaderLineCount;

		/** Size of block available for allocation without headers = 254 * 256 bytes = 65024 bytes */
		static const uint32_t EffectiveBlockSizeInBytes = EffectiveLineCount << LineBits;

		/** Size of the header in bytes */
		static const uint32_t HeaderSizeInBytes = HeaderLineCount << LineBits;

		/** Size of a block in bytes */
		static const uint32_t BlockSizeInBytes = 1 << BlockBits;

		/** Mask of a block size */
		static const uint32_t BlockSizeInBytesMask = BlockSizeInBytes - 1;;

		/** Inverse Mask of a block size */
		static const size_t BlockSizeInBytesInverseMask = ~(size_t)BlockSizeInBytesMask;

		/** Size of a line in bytes */
		static const uint32_t LineSizeInBytes = 1 << LineBits;
		static_assert(LineSizeInBytes <= 256, "LineSizeInBytes must be < 256");
		static_assert((LineSizeInBytes & 3) == 0, "LineSizeInBytes must be multiple of 4");

		/** Mask of the size of a line */
		static const uint32_t LineSizeInBytesMask = LineSizeInBytes - 1;

		/** Inverse Mask of the size of a line */
		static const uint32_t LineSizeInBytesInverseMask = ~LineSizeInBytesMask;

		/** Number of bits-block per allocation chunk */
		static const uint32_t BlockCountBitsPerChunk = 3;

		/** Number of block per allocation chunk */
		static const uint32_t BlockCountPerChunk = 1 << BlockCountBitsPerChunk;

		/** Mask of block per allocation chunk */
		static const uint32_t BlockCountPerChunkMask = BlockCountPerChunk-1;

		/** Total size of a chunk of blocks, including alloc alignment/local large object space */
		static const uint32_t ChunkSizeInBytes = BlockSizeInBytes * BlockCountPerChunk;

		/** Try to collect every CollectTriggerLimit bytes allocated */
		static const size_t CollectTriggerLimit = ChunkSizeInBytes * 7;

		/** Try to collect every CollectTriggerLimit bytes allocated */
		static const size_t AlignSizeMask = ~((size_t)BlockSizeInBytesMask);

	private:
		Constants(){}
	};
}