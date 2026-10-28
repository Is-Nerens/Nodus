#pragma once
#include <string.h>

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

typedef struct ArenaAllocatorGPU {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkDevice device;
    u32 typeIndex;
    ArenaChunkGPU chunks[ARENA_MAX_CHUNKS];
    u32 chunkCount;
    u32 currentChunk;
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

    // Bind and map memory
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
    for (u32 c=0; c<alloc->chunkCount; c++) {
        ArenaChunkGPU_Destroy(&alloc->chunks[c], alloc->device);
    }
    alloc->chunkCount = 0;
    alloc->currentChunk = 0;
}

static int ArenaAllocatorGPU_Init(ArenaAllocatorGPU* alloc, VkDevice device, VkPhysicalDevice physicalDevice)
{
    alloc->device = device;

    // Zero init chunk slots
    memset(alloc->chunks, 0, sizeof(alloc->chunks));
    alloc->chunkCount = 0;
    alloc->currentChunk = 0;

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

    // Allocate the first chunk
    if (!ArenaChunkGPU_Create(&alloc->chunks[0], device, alloc->typeIndex, ARENA_INITIAL_CHUNK)) {
        ArenaAllocatorGPU_Destroy(alloc);
        return 0;
    }
    alloc->chunkCount = 1;
    return 1;
}

static void ArenaAllocatorGPU_Clear(ArenaAllocatorGPU* alloc)
{
    // Wait for all previous GPU work to finish. This single wait is what
    // makes the arena safe to reset every frame without any per-slot or
    // per-chunk tracking. Everything after this call is free to overwrite
    // existing chunks, append new ones, or destroy idle ones.
    vkDeviceWaitIdle(alloc->device);

    // Increment idle counters for every chunk. Chunks that were used this
    // frame (i.e. touched by Alloc before this Clear ran) get reset to 0 in
    // Alloc, so this only counts chunks that went untouched
    for (u32 i=0; i<alloc->chunkCount; i++) {
        alloc->chunks[i].idleFrames++;
    }

    // Destroy chunks that have been idle for too long. Skip index 0 (the
    // primary chunk) because Alloc always starts there. Compact the array
    // so all live chunks are contiguous
    u32 write = 1;
    for (u32 read=1; read<alloc->chunkCount; read++) {
        if (alloc->chunks[read].idleFrames >= ARENA_CHUNK_IDLE_THRESHOLD) {
            ArenaChunkGPU_Destroy(&alloc->chunks[read], alloc->device);
        } else {
            if (write != read) {
                alloc->chunks[write] = alloc->chunks[read];
            }
            write++;
        }
    }
    alloc->chunkCount = write;

    // Reset bump pointers
    alloc->currentChunk = 0;
    for (u32 i=0; i<alloc->chunkCount; i++) {
        alloc->chunks[i].offset = 0;
    }
}

static ArenaAllocGPU ArenaAllocatorGPU_Alloc(ArenaAllocatorGPU* alloc, VkDeviceSize size, VkDeviceSize align)
{
    ArenaChunkGPU* chunk = &alloc->chunks[alloc->currentChunk];
    VkDeviceSize aligned = (chunk->offset + align - 1) & ~(align - 1);

    if (aligned + size > chunk->capacity) {
        // Advance to next chunk; create one if needed
        alloc->currentChunk++;
        if (alloc->currentChunk >= alloc->chunkCount) {

            // No more chunk slots -> fail
            if (alloc->chunkCount >= ARENA_MAX_CHUNKS) {
                alloc->currentChunk--;
                ArenaAllocGPU fail = { VK_NULL_HANDLE, 0, NULL };
                return fail;
            }

            // New chunks grow geometrically from the last one
            VkDeviceSize lastCap = alloc->chunks[alloc->chunkCount - 1].capacity;
            VkDeviceSize newSize = lastCap * 2;
            if (newSize < size) newSize = size * 2;
            if (!ArenaChunkGPU_Create(&alloc->chunks[alloc->chunkCount], alloc->device, alloc->typeIndex, newSize)) {
                // Creation failed — step back and return a null allocation
                alloc->currentChunk--;
                ArenaAllocGPU fail = { VK_NULL_HANDLE, 0, NULL };
                return fail;
            }
            alloc->chunkCount++;
        }
        chunk = &alloc->chunks[alloc->currentChunk];
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
