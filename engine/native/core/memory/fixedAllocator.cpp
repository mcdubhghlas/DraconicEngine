module;

#include <bit>
#include <cassert>
#include <cstdint>

module core.memory.fixedAllocator;

namespace draco::memory::fixed
{
	void init(FixedAllocator *alloc, Slice block)
	{
		alloc->buffer = (uint8_t *)block.data;
		alloc->size = block.size;
		alloc->allocated = false;
	}

	Error alloc(
		Allocator alloc,
		Slice *dst,
		usize size,
		usize align
	)
	{
		FixedAllocator *allocData = (FixedAllocator*)alloc.allocatorData;
		usize alignMask = align - 1;
		usize alignedSize = allocData->size - (
			(align - (((uintptr_t)allocData->buffer) & alignMask))
			& alignMask
		);
		assert(std::popcount(align) == 1);
		if (allocData->allocated | (alignedSize < size))
		{
			return Error::OutOfMemory;
		}
		dst->data = (void *)(
			((uintptr_t)&(allocData->buffer[alignMask])) & ~alignMask
		);
		dst->size = alignedSize;
		allocData->allocated = true;
		return Error::Okay;
	}

	Error freeAll(Allocator alloc)
	{
		FixedAllocator *allocData = (FixedAllocator*)alloc.allocatorData;
		allocData->allocated = false;
		return Error::Okay;
	}
}
