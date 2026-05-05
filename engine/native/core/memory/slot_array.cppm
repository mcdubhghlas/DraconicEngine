module;

#include <cstdint>

export module core.memory.slot_array;

import std;
import core.memory.handle;

export namespace draco::core::memory
{
    template<typename T>
    struct Slot
    {
        T value{};
        uint16_t generation = 0;
        bool alive = false;
    };

    template<typename T, typename Tag>
    class SlotArray
    {
    public:
        using Handle = Handle<Tag>;

        Handle create(const T& value)
        {
            uint16_t idx;

            if (!free_list.empty())
            {
                idx = free_list.back();
                free_list.pop_back();
            }
            else
            {
                idx = static_cast<uint16_t>(slots.size());
                slots.push_back({});
            }

            Slot<T>& slot = slots[idx];

            slot.value = value;
            slot.alive = true;

            return Handle::make(idx, slot.generation);
        }

        bool valid(Handle h) const
        {
            uint16_t i = h.index();

            return i < slots.size()
                && slots[i].alive
                && slots[i].generation == h.generation();
        }

        T* get(Handle h)
        {
            if (!valid(h))
                return nullptr;

            return &slots[h.index()].value;
        }

        const T* get(Handle h) const
        {
            if (!valid(h))
                return nullptr;

            return &slots[h.index()].value;
        }

        void destroy(Handle h)
        {
            if (!valid(h))
                return;

            auto& s = slots[h.index()];

            s.alive = false;
            s.generation++;     // Invalidate all old handles
            free_list.push_back(h.index());
        }

        std::vector<Slot<T>>& raw()
        {
            return slots;
        }

    private:
        std::vector<Slot<T>> slots;
        std::vector<uint16_t> free_list;
    };
}