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

#include <stdint.h>

#include "gtest/gtest_prod.h"

#ifdef _WIN32
#define GCIX_PLATFORM_WINDOWS 1
#endif

// TODO Make it compiler dependent
#define gcix_thread_local  __declspec(thread)
#define gcix_noinline __declspec(noinline)
#define gcix_fastcall __fastcall

#ifndef GCIX_ENABLE_INNER_OBJECT
/** Allows inner object. Default is true. */
#define GCIX_ENABLE_INNER_OBJECT 1
#endif

#ifndef GCIX_OFFSET_TO_VISITOR_FROM_VTBL
//(-sizeof(void*))
/** Offset from the VTBL/Class descriptor to get a pointer to the @see VisitorContext */
#define GCIX_OFFSET_TO_VISITOR_FROM_VTBL 0 
#endif

#ifndef GCIX_OBJECT_HEADER_ADDITIONAL_OFFSET 
#define GCIX_OBJECT_HEADER_ADDITIONAL_OFFSET (0)
#endif

#ifdef _DEBUG
#define GCIX_ENABLE_ASSERT
#endif

#ifdef GCIX_ENABLE_ASSERT
#include <assert.h>
/**
Assert macro
@param expression to assert
*/
#define gcix_assert(expression) assert(expression)
#else
#define gcix_assert(expression)
#endif

#define gcix_disable_new_delete_operator() \
static void* operator new (size_t size) = delete; \
static void operator delete (void *p) = delete; \

/** 
Macro used to declare a scoped enum which supports flags
*/
#define gcix_enum_flags(T,TInteger) \
enum class T : TInteger; \
inline T operator    &  (T x,   T y) { return (T)((TInteger)x & (TInteger)y); } \
inline T operator    |  (T x,   T y) { return (T)((TInteger)x | (TInteger)y); } \
inline T operator    ^  (T x,   T y) { return (T)((TInteger)x ^ (TInteger)y); } \
inline bool operator == (T x, int y) { return (TInteger)x == (TInteger)y; } \
inline bool operator != (T x, int y) { return (TInteger)x != (TInteger)y; } \
inline bool operator == (int y, T x) { return (TInteger)x == (TInteger)y; } \
inline bool operator != (int y, T x) { return (TInteger)x != (TInteger)y; } \
inline T operator    ~  (T x)        { return (T)~(TInteger)x; } \
inline T& operator   &= (T& x,  T y) { x = x & y;  return x; } \
inline T& operator   |= (T& x,  T y) { x = x | y;  return x; } \
inline T& operator   ^= (T& x,  T y) { x = x ^ y;  return x; } \
enum class T : TInteger





