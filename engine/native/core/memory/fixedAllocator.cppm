module;

#include <cassert>
#include <cstdint>

export module core.memory.fixedAllocator;
export import core.memory.allocator;
export import core.memory.slice;
export import core.stdtypes;

export namespace draco::memory
{
	namespace fixed
	{
		struct FixedAllocator
		{
			uint8_t *buffer;
			usize size;
			bool allocated;
		};

		void init(FixedAllocator *alloc, Slice block);

		Error alloc(Allocator alloc, Slice *dst, usize size, usize align);

		Error freeAll(Allocator alloc);

		AllocatorVTbl fixedAllocatorVtbl = {
			.alloc = alloc,
			.free = nilFree,
			.freeAll = freeAll,
		};

		inline void asAllocator(Allocator *dst, FixedAllocator *alloc)
		{
			asAllocatorVoid(dst, (void*)alloc, &fixedAllocatorVtbl);
		}
	}
}
