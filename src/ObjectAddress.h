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

#include "Constants.h"
#include "ObjectFlags.h"
#include "ObjectConstants.h"
#include "ObjectType.h"

namespace gcix
{
	struct ObjectAddress;
	struct VisitorContext;

	/**
	Delegate used to visit objects (e.g while marking)
	*/
	typedef void (gcix_fastcall *ObjectVisitorDelegate)(ObjectAddress* object, VisitorContext* context);

	struct VisitorContext
	{
		ObjectVisitorDelegate Visitor;
	};

	/**
	Accessor to an allocated object by gcix
	*/
	struct ObjectAddress
	{
		uint32_t ObjectFlags;

		/** 
		Indicates whether the object is marked by the collector 
		*/
		inline bool IsMarked() const
		{
			return (int32_t)ObjectFlags < 0;
		}

		/**
		Indicates whether the object is marked by the collector
		*/
		inline bool IsStickyLogged() const
		{
			return (((uint8_t*)this)[4] & ObjectFlags::StickyLogHigh) != 0;
		}

		/** 
		UnMark this object.
		*/
		inline void UnMark()
		{
			((uint8_t*)this)[4] &= ~ObjectFlags::MarkedHigh;
		}

		/** 
		Mark this object.
		*/
		inline void Mark()
		{
			((uint8_t*)this)[4] |= ObjectFlags::MarkedHigh;
		}

		/** 
		Type of an object 
		*/
		inline ObjectType Type() const
		{
			return (ObjectType)(ObjectFlags & ObjectFlags::ObjectTypeMask);
		}

		/** 
		Indicates whether the object is a medium or small object (allocated in blocks). If true, the SizeForMediumObject() 
		is valid 
		*/
		inline bool IsStandardObject() const
		{
			return Type() == ObjectType::Standard;
		}

		/** 
		Indicates whether the object is a large object. If true, the SizeForLargeObject() is valid 
		*/
		inline bool IsLargeObject() const
		{
			return Type() == ObjectType::Large;
		}

#if (GCIX_ENABLE_INNER_OBJECT == 1)
		/** 
		Indicates whether the object is an inner object. If true, the InnerObjectOffset() is valid 
		*/
		inline bool IsInnerObject() const
		{
			return Type() == ObjectType::Inner;
		}
#endif

		/** 
		Indicates whether the object is being forwarded. If true, the property ForwardAddress() is valid 
		*/
		inline bool IsForward() const
		{
			return ObjectFlags == (ObjectFlags::Marked | (uint32_t)ObjectType::Forward);
		}

		/**
		Gets a pointer to the beginning of the user object.
		*/
		inline void* ToUserObject() const
		{
			return (void*)((intptr_t)this + ObjectConstants::HeaderTotalSizeInBytes);
		}

		inline void* GetClassDescriptor() const
		{
			return *(void**)ToUserObject();
		}

		inline void SetClassDescriptor(void* vtbl)
		{
			*(void**)ToUserObject() = vtbl;
		}

		/**
		Gets the visitor associated with this object
		*/
		ObjectVisitorDelegate GetVisitor() const
		{
			return *(ObjectVisitorDelegate*)((intptr_t)GetClassDescriptor() + ObjectConstants::OffsetToVisitorFromVTBL);
		}

		/**
		Gets the @see ObjectAddress from a user object reference
		*/
		static inline ObjectAddress* FromUserObject(void* object)
		{
			return object == nullptr ? nullptr : (ObjectAddress*)((intptr_t)object - ObjectConstants::HeaderTotalSizeInBytes);
		}
	};

	/**
	A standard object allocated into a @see BlockData
	*/
	struct StandardObjectAddress : ObjectAddress
	{
		/** 
		Initiailize this object as a StandardObject 
		@param size Size of the object. Must be <= @see ObjectConstants::MaxObjectSizePerBlock
		*/
		inline void Initialize(size_t size)
		{
			ObjectFlags = (uint32_t)ObjectType::Standard | size;
		}

		/** 
		Size of this standard object
		*/
		inline uint32_t Size() const
		{
			gcix_assert(IsStandardObject());
			if (ObjectFlags == 0) return 0;
			auto objectSize = (ObjectFlags & ObjectFlags::SizeMask) + ObjectConstants::HeaderTotalSizeInBytes;
			return objectSize;
		}

		/** 
		Returns the address of the next medium object
		*/
		inline StandardObjectAddress* NextObject() const
		{
			gcix_assert(IsStandardObject());

			auto size = Size();
			if (size == 0) return nullptr;
			return (StandardObjectAddress*)((intptr_t)this + size);
		}

		/** 
		Determines whether the specified pointer is an interior pointer
		*/
		inline static bool IsInteriorPointerOrNext(StandardObjectAddress*& object, void* ptr)
		{
			gcix_assert(object->IsStandardObject());

			// Go to next object
			auto nextObject = object->NextObject();

			if (ptr >= object && ptr < nextObject)
			{
				return true;
			}
			else if (nextObject == nullptr || ptr < nextObject)
			{
				// If there is no other objects, or the next object after the pointer we are checking, return null
				object = nullptr;
			}
			else
			{
				object = nextObject;
			}
			return false;
		}
	};

	/**
	A large object with size larger than @see ObjectConstants::MaxObjectSizePerBlock
	*/
	struct LargeObjectAddress : ObjectAddress
	{
		/** 
		Initialize this instance as a LargeObject
		@param size size of this large object
		*/
		inline void Initialize(size_t size)
		{
			ObjectFlags = (uint32_t)ObjectType::Large | ((size / 4) & ObjectFlags::LargeSizeAndInnerObjectOffsetMask);
		}

		/** 
		Determines whether this instance contains the specified interior pointer 
		*/
		inline bool Contains(void* ptr)
		{
			auto size = Size();
			return ptr >= this && ptr < (void*)((intptr_t)this + size);
		}

		/** 
		Size of this large object 
		*/
		inline uint32_t Size() const
		{
			gcix_assert(IsLargeObject());

			auto objectSize = ((ObjectFlags & ObjectFlags::LargeSizeAndInnerObjectOffsetMask) << 2);				
			return objectSize;
		}
	};

#if (GCIX_ENABLE_INNER_OBJECT == 1)
	/**
	A inner object
	*/
	struct InnerObjectAddress : ObjectAddress
	{
		/** 
		Initialize this instance as a inner object
		@param offset relative to the beginning of the parent object
		*/
		inline void Initialize(uint32_t offset)
		{
			ObjectFlags = (uint32_t)ObjectType::Inner | (offset & ObjectFlags::LargeSizeAndInnerObjectOffsetMask);
		}

		/** 
		Gets the parent object of this inner object.
		*/
		inline ObjectAddress* Parent() const
		{
			gcix_assert(IsInnerObject());
			return (ObjectAddress*)((intptr_t)this - (ObjectFlags & ObjectFlags::LargeSizeAndInnerObjectOffsetMask));
		}
	};
#endif

	/**
	An object fowrarded to another object.
	*/
	struct ForwardObjectAddress : ObjectAddress
	{
		/** 
		Initialize this object with a forward reference
		@param size size of this large object
		*/
		inline void Initialize(ObjectAddress* newObject)
		{
			ObjectFlags = (ObjectFlags::Marked | (uint32_t)ObjectType::Forward);
			ForwardAddress() = newObject;
		}

		/**
		Returns the Forward address for this object 
		*/
		inline ObjectAddress*& ForwardAddress() const
		{
			gcix_assert(IsForward());
			return ((ObjectAddress**)this)[1];
		}
	};
}