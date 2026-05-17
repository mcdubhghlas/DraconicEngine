#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

import core.memory;

TEST_CASE("Bump allocator provides distinct pointers on allocation")
{
	using namespace draco::memory;
	bump::BumpAllocator bumpAlloc;
	Allocator alloc;
	Slice a;
	Slice b;
	Error err;
	bump::init(&bumpAlloc, page::pageAllocator);
	bump::asAllocator(&alloc, &bumpAlloc);
	err = alloc.vtbl->alloc(alloc, &a, sizeof(int), alignof(int));
	REQUIRE(err == Error::Okay);
	REQUIRE(bumpAlloc.first != nullptr);
	// REQUIRE(bumpAlloc.first->next == nullptr);
	REQUIRE(bumpAlloc.first->size > bumpAlloc.allocated);
	err = alloc.vtbl->alloc(alloc, &b, sizeof(int), alignof(int));
	REQUIRE(err == Error::Okay);
	REQUIRE((((ptrdiff_t)b.data) - ((ptrdiff_t)a.data)) >= sizeof(int));
	bump::deinit(&bumpAlloc);
}

TEST_CASE("Bump allocator aligns pointers correctly")
{
	using namespace draco::memory;
	bump::BumpAllocator bumpAlloc;
	Allocator alloc;
	Slice a;
	Slice b;
	Error err;
	bump::init(&bumpAlloc, page::pageAllocator);
	bump::asAllocator(&alloc, &bumpAlloc);
	err = alloc.vtbl->alloc(alloc, &a, sizeof(uint8_t), 2);
	REQUIRE(err == Error::Okay);
	REQUIRE(bumpAlloc.first != nullptr);
	// REQUIRE(bumpAlloc.first->next == nullptr);
	REQUIRE(bumpAlloc.first->size > bumpAlloc.allocated);
	err = alloc.vtbl->alloc(alloc, &b, sizeof(uint8_t), 3);
	REQUIRE(err == Error::Okay);
	REQUIRE((((uintptr_t)a.data) & (2 - 1)) == 0);
	REQUIRE((((uintptr_t)b.data) & (3 - 1)) == 0);
	bump::deinit(&bumpAlloc);
}

TEST_CASE("Bump allocator data is well packed")
{
	struct Foo
	{
		uint32_t a;
		uint64_t b;
	};
	using namespace draco::memory;
	bump::BumpAllocator bumpAlloc;
	Allocator alloc;
	Slice aSlice;
	Slice bSlice;
	uint32_t *a;
	uint64_t *b;
	Error err;
	bump::init(&bumpAlloc, page::pageAllocator);
	bump::asAllocator(&alloc, &bumpAlloc);
	err = alloc.vtbl->alloc(alloc, &aSlice, sizeof(uint32_t), alignof(Foo));
	REQUIRE(err == Error::Okay);
	REQUIRE(bumpAlloc.first != nullptr);
	// REQUIRE(bumpAlloc.first->next == nullptr);
	REQUIRE(bumpAlloc.first->size > bumpAlloc.allocated);
	err = alloc.vtbl->alloc(alloc, &bSlice, sizeof(uint64_t), alignof(Foo));
	REQUIRE(err == Error::Okay);
	a = (uint32_t*)aSlice.data;
	b = (uint64_t*)bSlice.data;
	*a = 69;
	*b = 420;
	REQUIRE(((Foo*)bumpAlloc.first->data)->a == 69);
	REQUIRE(((Foo*)bumpAlloc.first->data)->b == 420);
	bump::deinit(&bumpAlloc);
}

TEST_CASE("Bump allocator allocates second page when available")
{
	using namespace draco::memory;
	bump::BumpAllocator bumpAlloc;
	Allocator alloc;
	Slice aSlice;
	Slice bSlice;
	Error err;
	bump::init(&bumpAlloc, page::pageAllocator);
	bump::asAllocator(&alloc, &bumpAlloc);
	err = alloc.vtbl->alloc(alloc, &aSlice, 8192, 1);
	REQUIRE(err == Error::Okay);
	err = alloc.vtbl->alloc(alloc, &bSlice, 8192, 1);
	REQUIRE(err == Error::Okay);
	REQUIRE(aSlice.data != bSlice.data);
	REQUIRE(aSlice.size == 8192);
	REQUIRE(bSlice.size == 8192);
	REQUIRE(bumpAlloc.first->next != nullptr);
	bump::deinit(&bumpAlloc);
}

TEST_CASE("Exact alignment")
{
	using namespace draco::memory;
	bump::BumpAllocator bumpAlloc;
	Allocator alloc;
	Slice aSlice;
	Slice bSlice;
	Slice cSlice;
	Error err;
	bump::init(&bumpAlloc, page::pageAllocator);
	bump::asAllocator(&alloc, &bumpAlloc);
	err = alloc.vtbl->alloc(alloc, &aSlice, 5, 1);
	REQUIRE(err == Error::Okay);
	err = alloc.vtbl->alloc(alloc, &bSlice, 8, 8);
	REQUIRE(err == Error::Okay);
	err = alloc.vtbl->alloc(alloc, &cSlice, 4, 4);
	REQUIRE(err == Error::Okay);
	REQUIRE(bumpAlloc.allocated == 20);
	bump::deinit(&bumpAlloc);
}
