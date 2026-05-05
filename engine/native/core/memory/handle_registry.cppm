export module core.memory.handle_registry;

import core.memory.slot_array;
import core.memory.handle;

export namespace draco::core::memory
{
    // Manager layer so other subsystems don't touch raw storage logic
    
    template<typename T, typename Tag>
    class HandleRegistry
    {
    public:
        using Handle = Handle<Tag>;

        Handle create(const T& value)
        {
            return storage.create(value);
        }

        bool valid(Handle h) const
        {
            return storage.valid(h);
        }

        T* get(Handle h)
        {
            return storage.get(h);
        }

        const T* get(Handle h) const
        {
            return storage.get(h);
        }

        void destroy(Handle h)
        {
            storage.destroy(h);
        }

        SlotArray<T, Tag>& internal()
        {
            return storage;
        }

    private:
        SlotArray<T, Tag> storage;
    };
}