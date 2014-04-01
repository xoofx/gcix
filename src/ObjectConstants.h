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
	Constants used to allocate an object.
	*/
	class ObjectConstants
	{
	public:
		/**
		Size in bytes of the Object Header containing the 32bits @see ObjectFlags.
		*/
		static const uint8_t HeaderSize = sizeof(uint32_t);

		/**
		Additional size in bytes to add to the object header to store custom objects information (hashcode, sync block... etc.)
		*/
		static const uint8_t AdditionalHeaderOffset = GCIX_OBJECT_HEADER_ADDITIONAL_OFFSET;

		/**
		Total size in bytes of the object header = @see HeaderSize + @see AdditionalHeaderOffset
		*/
		static const uint8_t HeaderTotalSizeInBytes = HeaderSize + AdditionalHeaderOffset;

		/**
		Maximum size in bytes of an object that can be allocated in a block
		*/
		static const size_t MaxObjectSizePerBlock = (Constants::EffectiveBlockSizeInBytes / 4 - HeaderTotalSizeInBytes) & ~3;

		/**
		Offset from the class descriptor address to the @see ObjectVisitorDelegate (used to visit references of an object) 
		*/
		static const uint8_t OffsetToVisitorFromVTBL = GCIX_OFFSET_TO_VISITOR_FROM_VTBL;
	private:
		ObjectConstants() {}

		static_assert((AdditionalHeaderOffset & 3) == 0, "LIBGCIX_OBJECT_HEADER_ADDITIONAL_OFFSET "
			"must be multiple of 4 bytes");
	};
}