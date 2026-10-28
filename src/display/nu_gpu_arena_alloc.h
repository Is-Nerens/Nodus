#pragma once
#include <string.h>

#define ARENA_GPU_SLOTS              3
#define ARENA_MAX_CHUNKS             16
#define ARENA_INITIAL_CHUNK          (1024 * 1024) // 1MB
#define ARENA_CHUNK_IDLE_THRESHOLD   20

typedef struct ArenaChunkGPU {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped;
    VkDeviceSize capacity;
    VkDeviceSize offset;
    u32 idleFrames;
} ArenaChunkGPU;

typedef struct ArenaFrameGPU {
    ArenaChunkGPU chunks[ARENA_MAX_CHUNKS];
    u32 chunkCount;
    u32 currentChunk;
} ArenaFrameGPU;

typedef struct ArenaAllocatorGPU {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkDevice device;
    u32 typeIndex;
    u32 currentSlot;
    ArenaFrameGPU slots[ARENA_GPU_SLOTS];
} ArenaAllocatorGPU;

typedef struct ArenaAllocGPU {
    VkBuffer buffer;
    VkDeviceSize offset;
    void* mapped;
} ArenaAllocGPU;

static void ArenaChunkGPU_Destroy(ArenaChunkGPU* chunk, VkDevice device)
{
    if (chunk->mapped) {
        vkUnmapMemory(device, chunk->memory);
        chunk->mapped = NULL;
    }
    if (chunk->buffer) {
        vkDestroyBuffer(device, chunk->buffer, NULL);
        chunk->buffer = VK_NULL_HANDLE;
    }
    if (chunk->memory) {
        vkFreeMemory(device, chunk->memory, NULL);
        chunk->memory = VK_NULL_HANDLE;
    }
    chunk->capacity = 0;
    chunk->offset   = 0;
    chunk->idleFrames = 0;
}

static int ArenaChunkGPU_Create(ArenaChunkGPU* chunk, VkDevice device, u32 typeIndex, VkDeviceSize size)
{
    // Init chunk
    chunk->buffer = VK_NULL_HANDLE;
    chunk->memory = VK_NULL_HANDLE;
    chunk->mapped = NULL;
    chunk->capacity = size;
    chunk->offset = 0;
    chunk->idleFrames = 0;

    // Create buffer
    VkBufferCreateInfo bci = {0};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size  = size;
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bci, NULL, &chunk->buffer) != VK_SUCCESS) {
        return 0;
    }

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, chunk->buffer, &req);

    // Allocate memory
    VkMemoryAllocateInfo mai = {0};
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize  = req.size;
    mai.memoryTypeIndex = typeIndex;
    if (vkAllocateMemory(device, &mai, NULL, &chunk->memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, chunk->buffer, NULL);
        chunk->buffer = VK_NULL_HANDLE;
        return 0;
    }

    // Map memory
    if (vkBindBufferMemory(device, chunk->buffer, chunk->memory, 0) != VK_SUCCESS) {
        vkFreeMemory(device, chunk->memory, NULL);
        vkDestroyBuffer(device, chunk->buffer, NULL);
        chunk->memory = VK_NULL_HANDLE;
        chunk->buffer = VK_NULL_HANDLE;
        return 0;
    }
    if (vkMapMemory(device, chunk->memory, 0, VK_WHOLE_SIZE, 0, &chunk->mapped) != VK_SUCCESS) {
        vkFreeMemory(device, chunk->memory, NULL);
        vkDestroyBuffer(device, chunk->buffer, NULL);
        chunk->memory = VK_NULL_HANDLE;
        chunk->buffer = VK_NULL_HANDLE;
        return 0;
    }

    // Success
    return 1;
}

static void ArenaAllocatorGPU_Destroy(ArenaAllocatorGPU* alloc)
{
    for (u32 i=0; i<ARENA_GPU_SLOTS; i++) {
        ArenaFrameGPU* frame = &alloc->slots[i];
        for (u32 c=0; c<frame->chunkCount; c++) {
            ArenaChunkGPU_Destroy(&frame->chunks[c], alloc->device);
        }
        frame->chunkCount = 0;
        frame->currentChunk = 0;
    }
}

static int ArenaAllocatorGPU_Init(ArenaAllocatorGPU* alloc, VkDevice device, VkPhysicalDevice physicalDevice)
{
    alloc->device = device;

    // Zero init chunk slots
    memset(alloc->slots, 0, sizeof(alloc->slots));

    // Get memory type index
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &alloc->memoryProperties);
    alloc->typeIndex = UINT32_MAX;
    for (u32 i=0; i<alloc->memoryProperties.memoryTypeCount; i++) {
        if ((alloc->memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (alloc->memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            alloc->typeIndex = i;
            break;
        }
    }
    if (alloc->typeIndex == UINT32_MAX) return 0;

    // Allocate first swappable chunks
    alloc->currentSlot = 0;
    for (u32 i=0; i<ARENA_GPU_SLOTS; i++) {
        ArenaFrameGPU* frame = &alloc->slots[i];
        if (!ArenaChunkGPU_Create(&frame->chunks[0], device, alloc->typeIndex, ARENA_INITIAL_CHUNK)) {
            ArenaAllocatorGPU_Destroy(alloc);
            return 0;
        }
        frame->chunkCount = 1;
    }
    return 1;
}

static void ArenaAllocatorGPU_FreeIdleChunks(ArenaAllocatorGPU* alloc)
{
    for (u32 s=0; s<ARENA_GPU_SLOTS; s++) {
        ArenaFrameGPU* frame = &alloc->slots[s];
        for (u32 i=1; i<frame->chunkCount; ) {
            if (frame->chunks[i].idleFrames >= ARENA_CHUNK_IDLE_THRESHOLD) {
                ArenaChunkGPU_Destroy(&frame->chunks[i], alloc->device);
                if (frame->currentChunk > i) frame->currentChunk--;
                for (u32 j=i; j<frame->chunkCount-1; j++) {
                    frame->chunks[j] = frame->chunks[j + 1];
                }
                frame->chunkCount--;
            } else {
                i++;
            }
        }
    }
}

static void ArenaAllocatorGPU_SwapBuffers(ArenaAllocatorGPU* alloc)
{
    // Increment idle counters for the slot we're leaving
    ArenaFrameGPU* leaving = &alloc->slots[alloc->currentSlot];
    for (u32 i=0; i<leaving->chunkCount; i++) {
        leaving->chunks[i].idleFrames++;
    }

    // Advance
    alloc->currentSlot = (alloc->currentSlot + 1) % ARENA_GPU_SLOTS;

    // Reset the new current slot's bump pointers
    ArenaFrameGPU* entering = &alloc->slots[alloc->currentSlot];
    entering->currentChunk = 0;
    for (u32 i = 0; i < entering->chunkCount; i++) {
        entering->chunks[i].offset = 0;
    }
}

static ArenaAllocGPU ArenaAllocatorGPU_Alloc(ArenaAllocatorGPU* alloc, VkDeviceSize size, VkDeviceSize align)
{
    ArenaFrameGPU* frame = &alloc->slots[alloc->currentSlot];
    ArenaChunkGPU* chunk = &frame->chunks[frame->currentChunk];
    VkDeviceSize aligned = (chunk->offset + align - 1) & ~(align - 1);

    if (aligned + size > chunk->capacity) {
        // Advance to next chunk; create one if needed
        frame->currentChunk++;
        if (frame->currentChunk >= frame->chunkCount) {

            // No more chunk slots -> fail
            if (frame->chunkCount >= ARENA_MAX_CHUNKS) {
                frame->currentChunk--;
                ArenaAllocGPU fail = { VK_NULL_HANDLE, 0, NULL };
                return fail;
            }

            // New chunks grow geometrically from the last one
            VkDeviceSize lastCap = frame->chunks[frame->chunkCount - 1].capacity;
            VkDeviceSize newSize = lastCap * 2;
            if (newSize < size) newSize = size * 2;
            if (!ArenaChunkGPU_Create(&frame->chunks[frame->chunkCount], alloc->device, alloc->typeIndex, newSize)) {
                // Creation failed — step back and return a null allocation
                frame->currentChunk--;
                ArenaAllocGPU fail = { VK_NULL_HANDLE, 0, NULL };
                return fail;
            }
            frame->chunkCount++;
        }
        chunk = &frame->chunks[frame->currentChunk];
        chunk->offset = 0;
        aligned = 0;
    }

    ArenaAllocGPU result;
    result.buffer = chunk->buffer;
    result.offset = aligned;
    result.mapped = chunk->mapped;
    chunk->offset = aligned + size;
    chunk->idleFrames = 0;
    return result;
}
