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

namespace gcix
{
	/*
	OrderedBucketRange implementation
	*/
	template<typename T, typename THelper, size_t BucketSize = Constants::BlockSizeInBytes, uint32_t BucketCount = 512, uint32_t ItemPerBucketCount = 16>
	class OrderedBucketRange
	{
	private:
		/* A list of chunk, ordered by address */
		List<T*> Items;
		List<T*> BucketItems[BucketCount];
		T* minItem;
		T* maxItem;

		OrderedBucketRange(const OrderedBucketRange<T, THelper>& list)
		{
		}
	public:
		OrderedBucketRange() : Items(BucketCount*ItemPerBucketCount), minItem(nullptr), maxItem(nullptr)
		{
			for(int i = 0; i < BucketCount; i++)
			{
				BucketItems[i].SetCapacity(ItemPerBucketCount);
				BucketItems[i].Add((T*)(int)-1);
			}
		}

		inline int32_t Count() const
		{
			return Items.Count();
		}

		inline bool Contains(T* item) const
		{
			return item >= minItem && item < maxItem;
		}

		inline void Add(T* item)
		{
			// Add in order to Chunks
			ListExtensions::AddOrdered(Items, item);

			// Get the current bucket
			auto bucket = GetBucket(item);
			auto endItem = THelper::GetEndOfItem(item);
			
			auto endItemRaw = (intptr_t)endItem;
			for(auto startItem = (intptr_t)item; startItem < endItemRaw; startItem += BucketSize)
			{
				auto bucket = GetBucket((T*)startItem);
				if (!ListExtensions::AddOrdered(*bucket, item))
				{
					break;
				}
			}

			// Update min/max chunk address
			if (minItem == nullptr || item < minItem)
			{
				minItem = item;
			}
			if (endItem > maxItem)
			{
				maxItem = endItem;
			}
		}

		inline void Remove(int index)
		{
			auto item = Items[index];
			Items.Remove(index);

			// Get the current bucket
			auto bucket = GetBucket(item);
			auto endItem = THelper::GetEndOfItem(item);
			
			auto endItemRaw = (intptr_t)endItem;
			for (auto startItem = (intptr_t)item; startItem < endItemRaw; startItem += BucketSize)
			{
				auto bucket = GetBucket((T*)startItem);
				index = ListExtensions::FindOrderedIndex(*bucket, item);
				if (index < bucket->Count() && (*bucket)[index] == item)
				{
					bucket->Remove(index);
				}
				else
				{
					break;
				}
			}
		}

		void ResetMinMax()
		{
			// Recompute min/max
			minItem = nullptr;
			maxItem = nullptr;
			auto count = Items.Count();
			for (int i = 0; i < count; i++)
			{
				auto item = Items[i];
				auto endItem = THelper::GetEndOfItem(item);

				// Update min/max chunk address
				if (minItem == nullptr || item < minItem)
				{
					minItem = item;
				}
				if (endItem > maxItem)
				{
					maxItem = endItem;
				}
			}
		}

		inline List<T*>* GetBucket(T* approximateValue)
		{
			return &BucketItems[((size_t)approximateValue / BucketSize) & (BucketCount-1)];
		}

		inline T* &operator[](uint32_t index)
		{ 
			return Items[index]; 
		}
	};
}