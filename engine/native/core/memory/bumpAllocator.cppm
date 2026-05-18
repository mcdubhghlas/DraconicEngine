module;

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

export module core.memory.bumpAllocator;
export import core.memory.allocator;
export import core.memory.slice;

export namespace draco::memory
{
	namespace bump
	{
		struct Node
		{
			Node *next;
			size_t size;
			uint8_t data[];
		};

		struct BumpAllocator
		{
			Allocator base;
			Node *first;
			size_t minAllocRequest;
			size_t allocated;
		};

		void init(
			BumpAllocator *alloc,
			Allocator baseAlloc,
			// one page by default on unix-like systems
			size_t minAllocRequest = (1 << 12)
		);

		void deinit(BumpAllocator *alloc);

		Error alloc(Allocator alloc, Slice *dst, size_t size, size_t align);

		Error freeAll(Allocator alloc);

		AllocatorVTbl bumpAllocatorVtbl = {
			.alloc = alloc,
			.free = nilFree,
			.freeAll = freeAll,
		};

		size_t saveMark(BumpAllocator *self);

		void resumeMark(BumpAllocator *self, size_t mark);

		inline void asAllocator(Allocator *dst, BumpAllocator *alloc)
		{
			asAllocatorVoid(dst, (void*)alloc, &bumpAllocatorVtbl);
		}
	}
}
