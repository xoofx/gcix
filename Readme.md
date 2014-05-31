# gcix

**gcix** provides a cross-platform implementation of the [Immix garbage collector](cs.anu.edu.au/techreports/2007/TR-CS-07-04.pdf)

## Why another GC?

The only GC that is providing a VM/language agnostic API is currently provided by **boehm gc** (see [bdwgc](https://github.com/ivmai/bdwgc/)). There is currently no Immix GC implementation in C++ that can be used/integrated as easily as the boehm gc. There are some Immix gc implementations around. but they are often tightly integrated into a specific VM/language like [Haxe](http://gamehaxe.com/2009/08/17/switched-to-immix-for-internal-garbage-collection/), [Rubinius](http://rubini.us/2013/06/22/concurrent-garbage-collection/)...etc

## Goals

- Immix GC as a service
- Provide a "reference" implementation in C++11 of the Immix GC and eventually [RC Immix](http://users.cecs.anu.edu.au/~steveb/downloads/pdf/rcix-oopsla-2013.pdf)
- Provide a VM/language agnostic garbage collector using latest research (Immix/RC)
- Support for Sticky Immix (semi-generational)
- Support for interior pointers
- Precise pointers scanning: semi-conservative on stack and precise on heap.
- Several unit tests to ensure the GC is correctly implemented and tracking regressions (*Note: unit tests are currently broken. Changes on latest internals not backported to tests*)
- When enough mature make a join-venture with [SharpLang](https://github.com/xen2/SharpLang) project 


## Current state

The project is currently under development and is in a very *early alpha* state (and well, no development on it for the past month!).

What has been done so far:

- A `GlobalAllocator`, providing a chunk allocator (a chunk is composed of several Immix blocks), 
- A `ThreadLocalAlloactor`, to be used by mutator GC (i.e. the program)
- Basic multithreading support (`GlobalAllocator` is thread safe)
- Naive marking and collector


What is under development:

- Implements marking by using SSB (Sequential Store Buffers)
- Start to work on implementing a basic write barrier infrastructure

## Help the project

If you are interested, don't hesitate to contact me via the issues tab or alexandre_mutel at live.com 
  

## License
BSD-Clause2 license
