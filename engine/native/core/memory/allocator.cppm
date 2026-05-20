module;

export module core.memory.allocator;
export import core.memory.slice;
export import core.stdtypes;

export namespace draco::memory
{
	enum class Error
	{
		Okay,
		OutOfMemory,
		NotImplemented,
		IllegalAddressRange,
		Other, // This one shouldn't be needed. If you see it returned, make a
			   // new error.
	};

	struct AllocatorVTbl;

	struct Allocator
	{
		AllocatorVTbl *vtbl;
		void *allocatorData;
	};

	struct AllocatorVTbl
	{
		using AllocFn = Error (*)(
			Allocator alloc,
			Slice *dst,
			usize size,
			usize align
		);
		using FreeFn = Error (*)(Allocator alloc, Slice block);
		using FreeAllFn = Error (*)(Allocator alloc);
		AllocFn alloc;
		FreeFn free;
		FreeAllFn freeAll;
	};

	Error nilAlloc(
		Allocator alloc,
		Slice *dst,
		usize size,
		usize align
	);

	Error nilFree(Allocator alloc, Slice block);

	Error nilFreeAll(Allocator alloc);

	void asAllocatorVoid(Allocator *dst, rawptr alloc, AllocatorVTbl *vtbl);
}
