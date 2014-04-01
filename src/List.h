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
#include "Memory.h"

namespace gcix
{
	/*
	List implementation
	*/
	template<typename T>
	class List
	{
	private:
		int32_t capacity;
		int32_t count;
		T* items;
	public:
		List(int32_t capacity) : capacity(0), count(0), items(nullptr)
		{
			SetCapacity(capacity);
		}

		List() : capacity(0), count(0), items(nullptr)
		{
		}

		List(const List<T>& list)
		{
			SetCapacity(list.capacity);
			count = list.count;
			for(uint32_t i = 0; i < count; i++)
			{
				items[i] = list[i];
			}
		}

		~List()
		{
			Memory::Free(items);
		}

		inline int32_t Count() const
		{
			return count;
		}

		inline int32_t GetCapacity() const
		{
			return capacity;
		}

		inline void SetCapacity(int32_t newCapacity)
		{
			if (newCapacity <= capacity)
			{
				return;
			}
			capacity = newCapacity;
			items = (T*)Memory::ReAllocate(items, capacity * sizeof(T));
		}

		inline void Clear()
		{
			count = 0;
		}

		inline void Add(const T& item)
		{
			if (count == capacity)
			{
				EnsureCapacity(count + 1);
			}
			items[count++] = item;
		}

		inline void Insert(int32_t index, const T& item)
		{
			gcix_assert(index <= count);

			if (count == capacity)
			{
				EnsureCapacity(count + 1);
			}

			for(int32_t i = count; i > index; i--)
			{
				items[i] = items[i-1];
			}
			items[index] = item;
			count++;
		}

		inline bool Find(const T& item, int32_t& i)
		{
			for(i = 0; i < count; i++)
			{
				if (items[i] == item)
				{
					return true;
				}
			}
			return false;
		}

		inline void Remove(int32_t index)
		{
			gcix_assert(index < count);
			for(int32_t i = index; i < (count-1); i++)
			{
				items[i] = items[i+1];
			}
			count--;
		}

		inline T &operator[](int32_t index) 
		{ 
			gcix_assert(index < count);
			return items[index]; 
		}
	private:
		void EnsureCapacity(int32_t minCapacity)
		{
			if (count >= minCapacity)
				return;
			SetCapacity(count ? count * 2 : 4);
		}
	};

	typedef List<void*> PointerList;

	class ListExtensions
	{
	public:
		template<typename T>
		static bool AddOrdered(List<T>& list, const T& item)
		{
			auto count = list.Count();
			if (count == 0 || item >= list[count-1])
			{
				if (count > 0 && item == list[count-1])
				{
					return false;
				}
				list.Add(item);
			}
			else
			{
				// Else insert the link at the correct index
				int32_t index = FindOrderedIndex(list, item);
				if (index < list.Count() && list[index] == item)
				{
					return false;
				}
				list.Insert(index, item);
			}
			return true;
		}

		template<typename T>
		static int32_t FindOrderedIndex(List<T>& list, const T& item)
		{
			if (list.Count() == 0) return 0;
			int32_t min = 0;
			int32_t max = list.Count() - 1;
			while (min < max)
			{
				int32_t midPoint = min + (max - min) / 2;
				auto it = list[midPoint];
				if (item == it)
				{
					return midPoint;
				}
				else if (item > it)
				{
					min = midPoint + 1;
				}
				else if (item < it)
				{
					if (midPoint == 0)
						break;
					max = midPoint - 1;
				}
			}
			return (int32_t)min;
		}

	private:
		ListExtensions() {}
	};


}