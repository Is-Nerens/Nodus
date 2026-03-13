#pragma once

#include <stdlib.h>

typedef struct Container {
    void* data;
    int* ids;
    int* indices;
    int* slots;
    int size;
    int capacity;
    int freeHead;
    size_t elementSize;
} Container;

static inline Container Container_Create(size_t elementSize)
{
    const int initialCapacity = 8;

    Container container = {0};
    container.elementSize = elementSize;
    container.size = 0;
    container.capacity = initialCapacity;
    container.freeHead = -1;

    // allocate storage
    container.data = malloc(initialCapacity * elementSize);
    container.ids = malloc(initialCapacity * sizeof(int));
    container.indices = malloc(initialCapacity * sizeof(int));
    container.slots = malloc(initialCapacity * sizeof(int));

    // initialize slots
    for (int i = 0; i < initialCapacity; ++i) {
        container.ids[i] = 0;
        container.indices[i] = -1;
        container.slots[i] = -1;
    }

    return container;
}

static inline void Container_Free(Container* container)
{
    if (!container) return;
    free(container->data);
    free(container->ids);
    free(container->indices);
    free(container->slots);
    *container = (Container){0};
}

static inline void Container_Resize(Container* container, int newCapacity)
{
    container->data = realloc(container->data, newCapacity * container->elementSize);
    container->ids = realloc(container->ids, newCapacity * sizeof(int));
    container->indices = realloc(container->indices, newCapacity * sizeof(int));
    container->slots = realloc(container->slots, newCapacity * sizeof(int));

    for (int i = container->capacity; i < newCapacity; ++i) {
        container->ids[i] = 0;
        container->indices[i] = -1;
        container->slots[i] = -1;
    }

    container->capacity = newCapacity;
}

static inline int Container_Add(Container* container, void* item)
{
    if (container->size >= container->capacity) Container_Resize(container, container->capacity ? container->capacity * 2 : 8);

    int slot;
    if (container->freeHead != -1) {
        slot = container->freeHead;
        container->freeHead = container->indices[slot]; // next free slot
    }
    else {
        slot = container->size;
    }

    int index = container->size;

    // Copy element directly into data array
    memcpy((char*)container->data + index * container->elementSize, item, container->elementSize);

    container->indices[slot] = index;
    container->slots[index] = slot;

    container->size++;

    int id = (container->ids[slot] << 16) | slot;
    return id;
}

static inline void Container_Remove(Container* container, int id)
{
    int slot = id & 0xFFFF;
    int gen  = (id >> 16) & 0xFFFF;

    if (slot >= container->capacity || container->ids[slot] != gen) return;

    int index = container->indices[slot];
    int lastIndex = container->size - 1;

    if (index != lastIndex)
    {
        // move last element into removed spot
        memcpy((char*)container->data + index * container->elementSize,
               (char*)container->data + lastIndex * container->elementSize,
               container->elementSize);

        int lastSlot = container->slots[lastIndex];
        container->indices[lastSlot] = index;
        container->slots[index] = lastSlot;
    }

    container->size--;
    container->ids[slot]++;
    container->indices[slot] = container->freeHead;
    container->freeHead = slot;
}

static inline void* Container_Get(Container* container, int id)
{
    int slot = id & 0xFFFF;
    int gen  = (id >> 16) & 0xFFFF;
    if (slot >= container->capacity || container->ids[slot] != gen) return NULL;
    int index = container->indices[slot];
    return (char*)container->data + index * container->elementSize;
}

static inline void* Container_GetAt(Container* container, int i)
{
    if (!container) return NULL;
    if (i < 0 || i >= container->size) return NULL; // bounds check
    return (char*)container->data + i * container->elementSize;
}

static inline int Container_IdAt(Container* container, int i)
{
    if (!container) return -1;
    if (i < 0 || i >= container->size) return -1;

    int slot = container->slots[i];       // get the slot for this index
    int gen  = container->ids[slot];      // get the current generation
    int id   = (gen << 16) | slot;        // reconstruct full ID
    return id;
}