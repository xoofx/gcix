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
#include "Constants.h"

namespace gcix
{
	/**
	ObjectFlags are stored at ObjectFlags::ObjectHeaderOffset bytes before the address of an object.
	Unlike original Immix paper that was using only 1 byte, we are using 4 bytes, to leverage on alignment
	and to store additional information that will avoid to go through a type descriptor for some basic ops:
	- We store the Marked flag, used by the Mark GC pass
	- We store the InnerObject flag, used to identify inner object inside a container object
	- If InnerObject is set, the InnerObjectOffset contains the offset relative to the start of the container object
	- We store the size (instead of having to query the object descriptor when we need to move the object)
	- We store if an object is pinned.
	- We store a PointerFree indicator to avoid scanning the object if there are no pointers in it
	- We store a Forward bit in case the object has been forwarded. The address of the forwarded object is stored at the object
	  level (offset 0)
	- We store a flag to indicate whether the object contains less than 15 object references (in the first 64 bytes of the 
	  object)
	Layout depends on the endianess:
	For Little Endian:                      0
	|                                       | <- Object Address
	|   -4    |   -3    |   -2    |   -1    |
	|SSSS SSTT|SSSS SSSS|SSSS SSSS|MSSS SSSS|
	For Big Endian:                         |
	|                                       | 
	|   -4    |   -3    |   -2    |   -1    |
	|MIWS SSSS|SSSS SSSS|SSSS SSSS|SSSS SSLF|
	*/
	class ObjectFlags
	{
	public:

		/** 
		ObjectType bits (T) 
		*/
		static const uint32_t ObjectTypeMask = 0x00000003;

		/** 
		Marked bit (M) 
		*/
		static const uint8_t  MarkedHigh = 0x80; 
		static const uint32_t Marked      = 0x80000000; // Caution, the marker bit is always expected to be at this position
														// as we are relying on the last bit to use a signed comparison

		/**
		Sticky log bit (L) use to mark objects that have their references changes since last small collection, used when
		generational sticky collection.
		*/
		static const uint8_t  StickyLogHigh = 0x40;
		static const uint32_t StickyLog = 0x40000000;

		/** 
		Size Mask bits (S)
		The size is stored as a multiple of 4 bytes 
		(objects are allocated on a 4 bytes boundary)
		*/
		static const uint32_t SizeMask    = ((Constants::BlockSizeInBytes >> 2) - 1) << 2; // 0x1FFC 

		/**
		Large Size Mask bits (S)
		The size is stored as a multiple of 16 bytes 
		(objects are allocated on a 16 bytes boundary)
		*/
		static const uint32_t LargeSizeAndInnerObjectOffsetMask = ~(Marked | StickyLog | ObjectTypeMask);
	private:
		ObjectFlags(){}
	};
}