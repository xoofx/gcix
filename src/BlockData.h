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
#include "ChunkHeader.h"
#include "Memory.h"
#include "ObjectAddress.h"
#include "LineFlags.h"
#include "BlockFlags.h"

namespace gcix
{
	/**
	A line data used by @see BlockData
	*/
	struct LineData
	{
		uint8_t Buffer[Constants::LineSizeInBytes];
	};

	/**
	A block data is storing allocated object into lines. It contains a simple metadata in the header for
	each line (LineFlags).
	*/
	union BlockData
	{
		struct
		{
			union
			{
				struct
				{
					uint32_t BumpCursor;
					uint32_t BumpCursorLimit;

					BlockFlags BlockFlags;
					uint8_t UsedLineCount;
					uint8_t ConsecutiveUsedLineCount;
					uint8_t Pinned;
					uint8_t BlockIndex;

					/* Chunk description only valid in the first block data of a chunk */
					ChunkHeader Chunk;
				} Info;

				/* One LineFlags per line */
				LineData padding;
			};

			/* One LineFlags per line */
			LineFlags LineFlags[Constants::LineCount];
		} Header;

		// 32768 - 256 of object allocation
		LineData Lines[Constants::LineCount];

		/**
		Determines whether this block is recyclable.
		*/
		inline bool IsRecyclable()
		{
			return Header.Info.BlockFlags == BlockFlags::Recyclable;
		}

		/**
		Determines whether this block is unavailable.
		*/
		inline bool IsUnavailable()
		{
			return Header.Info.BlockFlags == BlockFlags::Unavailable;
		}

		/**
		Determines whether this block is completely free.
		*/
		inline bool IsFree()
		{
			return Header.Info.BlockFlags == BlockFlags::Free;
		}

		/**
		Determines whether the specified line in this block contains an object.
		*/
		inline bool ContainsObject(uint8_t lineIndex) const
		{
			gcix_assert(lineIndex >= Constants::HeaderLineCount);
			return (Header.LineFlags[lineIndex] & LineFlags::ContainsObject) != 0;
		}

		/**
		Gets the first object stored at the specified line. ContainsObject() must be called before calling this method
		*/
		inline StandardObjectAddress* GetFirstObject(uint8_t lineIndex) const
		{
			gcix_assert(ContainsObject(lineIndex));

			auto offset = (uint8_t)(Header.LineFlags[lineIndex] & LineFlags::FirstObjectOffsetMask);
			return (StandardObjectAddress*)&Lines[lineIndex].Buffer[offset];
		}


		/** 
		Get the block associated with this address
		*/
		static inline BlockData* FromObject(StandardObjectAddress* object)
		{
			gcix_assert(object != nullptr);
			gcix_assert(object->IsStandardObject());

			return (BlockData*)((intptr_t)object & Constants::BlockSizeInBytesInverseMask);
		}

		/**
		Mark lines associated with the specified object.
		*/
		inline void MarkLines(StandardObjectAddress* object)
		{
			uint32_t offset = (intptr_t)object - (intptr_t)this;
			uint32_t lineFrom = offset >> Constants::LineBits;
			uint32_t lineTo = (offset + object->Size()) >> Constants::LineBits;
			for(uint32_t i = lineFrom; i <= lineTo; i++)
			{
				LineFlags& lineFlags = Header.LineFlags[i];
				lineFlags |= LineFlags::Marked;
			}
			Header.Info.BlockFlags = BlockFlags::Unavailable;
		}
	private:
		friend class Chunk;

		/**
		Clears this block as not marked. This is performed before a new full collection.
		*/
		inline void ClearMarked()
		{
			Header.Info.BlockFlags = BlockFlags::Free;
			for(uint32_t i = Constants::HeaderLineCount; i < Constants::LineCount; i++)
			{
				Header.LineFlags[i] &= ~LineFlags::Marked;
			}
		}

		/**
		Initialize this block
		*/
		inline void Initialize()
		{
			// Set the header and line flags to zero
			auto pLines = (uint32_t*)this;
			for(uint32_t i = 0; i < sizeof(Header)/sizeof(uint32_t); i++)
				*pLines++ = 0;

			Header.Info.BumpCursor = Constants::HeaderSizeInBytes;
		}

		/**
		Sets the flags of this block
		*/
		void SetFlags(BlockFlags flags)
		{
			Header.Info.BlockFlags = flags;
		}

		/**
		Clears unmarked lines and mark the block free, recyclable or marked.
		*/
		inline void Recycle()
		{
			Header.Info.BumpCursor = 0;
			Header.Info.BumpCursorLimit = 0;
			Header.Info.UsedLineCount = 0;
			Header.Info.ConsecutiveUsedLineCount = 0;

			// If the block is marked, check if it is recyclable (at least one free line)
			if (IsUnavailable())
			{
				bool previousLineWasUsed = false;
				for(uint32_t i = Constants::HeaderLineCount; i < Constants::LineCount; i++)
				{
					if ((Header.LineFlags[i] & LineFlags::Marked) != 0)
					{
						Header.Info.UsedLineCount++;
						if (previousLineWasUsed)
						{
							Header.Info.ConsecutiveUsedLineCount++;
						}
						previousLineWasUsed = true;
						if (Header.Info.BumpCursorLimit == 0 && Header.Info.BumpCursor != 0)
						{
							Header.Info.BumpCursorLimit = i << Constants::LineBits;
						}
					}
					else
					{
						previousLineWasUsed = false;
						Header.LineFlags[i] = LineFlags::Empty;
						if (Header.Info.BumpCursor == 0)
						{
							Header.Info.BumpCursor = i << Constants::LineBits;
						}

						// Clear the line to recycle.
						Memory::ClearSmall(&Lines[i], Constants::LineSizeInBytes);
					}
				}

				if (Header.Info.BumpCursor != 0 && Header.Info.BumpCursorLimit == 0)
				{
					Header.Info.BumpCursorLimit = Constants::LineCount << Constants::LineBits;
				}

				Header.Info.BlockFlags = Header.Info.UsedLineCount == Constants::EffectiveLineCount ? BlockFlags::Unavailable :
					BlockFlags::Recyclable;
			}
			else
			{
				Header.Info.BlockFlags = BlockFlags::Free;
			}

			if (Header.Info.BumpCursor == 0)
			{
				Header.Info.BumpCursor = Constants::HeaderSizeInBytes;
			}
		}
	};
	static_assert(sizeof(BlockData) == Constants::BlockSizeInBytes, "Size of BlockData doesn't match expected size");
}