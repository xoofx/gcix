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
#include "ObjectAddress.h"
#include "BlockData.h"

namespace gcix
{
	/**
	Use to mark object and recursively walk through the object graph to mark all objects.
	*/
	class Marker
	{
	public:
		/**
		Visits and mark recursively the specified object.
		@param object A reference to a managed object.
		*/
		static inline void Mark(ObjectAddress* object)
		{
			VisitorContext context = { Marker::Mark };
			Mark(object, &context);
		}

	private:
		Marker() {}

		static /* gcix_noinline */ void gcix_fastcall Mark(ObjectAddress* object, VisitorContext* context)
		{
			// If object is null or already marked, return immediately
			// We are not using any lock when performing IsMarked()/Mark() as immix is optimistically marking objects
			// This avoid a high cost when marking object, as concurrent marking should not happen very often on the same 
			// object at the same time
			if (object == nullptr || object->IsMarked())
			{
				return;
			}
			// Mark the object
			object->Mark();

#if (GCIX_ENABLE_INNER_OBJECT == 1)
			// Handle if it is a inner object, getting the parent object
			if (object->IsInnerObject())
			{
				// idem, check if marked/return or mark and process
				object = ((InnerObjectAddress*)object)->Parent();
				if (object->IsMarked())
				{
					return;
				}
				object->Mark();
			}
#endif

			// If this is a standard object, we need to mark the block.
			if (object->IsStandardObject())
			{
				auto standardObject = (StandardObjectAddress*)object;
				auto blockData = BlockData::FromObject(standardObject);
				blockData->MarkLines(standardObject);
			}

			// Gets the visitor attached to this object
			auto visitor = object->GetVisitor();

			// If there is no visitor, this is a pointer free object, so we can return immediately
			if (visitor == nullptr)
			{
				return;
			}

			auto inlineVisitor = (intptr_t)visitor; 
			if (inlineVisitor & 1)
			{
				inlineVisitor /= 2;
				void** userObject = (void**)object->ToUserObject();
				for(int i = 0; i < inlineVisitor; i++)
				{
					userObject++;
					Mark(ObjectAddress::FromUserObject(*userObject), context);
				}
			}
			else
			{
				// Visit recursively objects
				visitor(object, context);
			}
		}
	};
}