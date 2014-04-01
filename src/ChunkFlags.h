#pragma once

#include "Common.h"

namespace gcix
{
	/**
	ChunkFlags (8bits)
	*/
	class ChunkFlags
	{
	public:
		/**
		The chunk is completely free
		*/
		static const uint8_t None = 0x00;

		/**
		The chunk has free blocks
		*/
		static const uint8_t HasFreeBlocks = 0x01;

		/**
		Marked indicating that this chunk is marked and at least one block is marked.
		*/
		static const uint8_t HasMarkedBlocks = 0x02;

		/**
		Recyclable indicating that this chunk is recyclable and contains at least one recyclable block
		*/
		static const uint8_t HasRecyclableBlocks = 0x04;
	private:
		ChunkFlags() {}
	};	
}