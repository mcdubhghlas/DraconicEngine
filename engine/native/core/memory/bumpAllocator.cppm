module;

#include <algorithm>
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
		using namespace std;
		struct Node;

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
		)
		{
			memset(alloc, 0, sizeof(BumpAllocator));
			alloc->base = baseAlloc;
			alloc->minAllocRequest = minAllocRequest;
		}

		void deinit(BumpAllocator *alloc)
		{
			Node *lastNode;
			Node *node = alloc->first;
			while (node != nullptr)
			{
				lastNode = node;
				node = node->next;
				alloc->base.vtbl->free(
					alloc->base,
					{
						.data = (void*)lastNode,
						.size = lastNode->size + sizeof(Node),
				   	}
				);
			}
		}

		Error alloc(
			Allocator alloc,
			Slice *dst,
			size_t size,
			size_t align
		)
		{
			Error err;
			BumpAllocator *allocData = (BumpAllocator *)alloc.allocatorData;
			uintptr_t alignMask = align - 1;
			Node **lastNode;
			Node **node = &(allocData->first);
			size_t pos = allocData->allocated;
			size_t oldPos = pos;
			size_t reqSize = size;
			Slice newBlock;
			uintptr_t currentPtr;
			lastNode = node;
			while (((*node) != nullptr) & (pos > 0))
			{
				oldPos = pos;
				pos -= std::min((*node)->size, pos);
				lastNode = node;
				node = &((*node)->next);
			}
			assert(pos == 0); // fraudulent mark provided
			currentPtr = (uintptr_t)&((*lastNode)->data[pos]);
			reqSize += ((reqSize + currentPtr - 1) & alignMask);
			if (!(*lastNode) || (reqSize > ((*lastNode)->size - oldPos)))
			{
				reqSize = (sizeof(Node) + size) & ~alignMask;
				err = allocData->base.vtbl->alloc(
					allocData->base,
					&newBlock,
					std::max(allocData->minAllocRequest, reqSize),
					std::max(alignof(Node), align)
				);
				if (err != Error::Okay)
				{
					return Error::OutOfMemory;
				}
				(*node) = (Node *)newBlock.data;
				(*node)->size = newBlock.size - sizeof(Node);
				pos = 0;
				lastNode = node;
			}
			currentPtr = ((uintptr_t)&((*lastNode)->data[oldPos]));
			reqSize = (size + ((currentPtr - 1) & alignMask)) & ~alignMask;
			currentPtr = (currentPtr + alignMask) & ~alignMask;
			allocData->allocated += reqSize;
			dst->data = (void*)currentPtr;
			dst->size = size;
			return Error::Okay;
		}

		Error freeAll(Allocator alloc)
		{
			BumpAllocator *allocData = (BumpAllocator *)alloc.allocatorData;
			allocData->allocated = 0;
			return Error::Okay;
		}

		AllocatorVTbl bumpAllocatorVtbl = {
			.alloc = alloc,
			.free = nilFree,
			.freeAll = freeAll,
		};

		size_t saveMark(BumpAllocator *self)
		{
			return self->allocated;
		}

		void resumeMark(BumpAllocator *self, size_t mark)
		{
			self->allocated = mark;
		}

		inline void asAllocator(Allocator *dst, BumpAllocator *alloc)
		{
			asAllocatorVoid(dst, (void*)alloc, &bumpAllocatorVtbl);
		}
	}
}
