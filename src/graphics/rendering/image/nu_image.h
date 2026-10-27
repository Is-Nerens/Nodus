#pragma once

/* Contains definitions and implentation of image resource management for Nodus
 * This includes image data and handles, atlases, filepath -> handle mapping,
 * Image loading / unloading, atlas packing, image CPU/GPU memory management
 */
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stddef.h>
#include "stb_image.h"
#include "nu_atlas_builder.h"

// Represents and image that can exist on the GPU
typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkDescriptorSet descriptor;
    VkImageLayout currentLayout;
    u16 width;
    u16 height;
} ImageResource;

/* Represents a region of an image resource
 * An image resource may have 1 or many views (if the resource is an atlas)
 */
typedef struct {
    u16 x, y, w, h;
    int imageResourceIndex;
} ImageView;

// Entire image resource manager struct
typedef struct {
    LinearStringmap filepathToHandleMap;
    AtlasBuilder builders[3]; // Builders pack small images into image resources at startup
    int buildersCreated;
    Array imageViews;
    Array imageResources;
} ImageResourceManager;

/* Creates an image resource that Vulkan can work with */
int ImageResource_Create(
    DisplayManager* displayManager,
    u32 width,
    u32 height,
    const unsigned char* buffer,
    ImageResource* imageResource)
{
    // Result
    *imageResource = (ImageResource){0};

    const VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    const VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;

    VkBuffer       stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkCommandBuffer cmd          = VK_NULL_HANDLE;
    VkFence        fence        = VK_NULL_HANDLE;

    // Create image
    imageResource->width  = (u16)width;
    imageResource->height = (u16)height;
    VkImageCreateInfo imageInfo = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = { width, height, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    if (vkCreateImage(displayManager->device, &imageInfo, NULL, &imageResource->image) != VK_SUCCESS) goto fail;

    // Get memory requirements
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(displayManager->device, imageResource->image, &requirements);

    u32 memoryTypeIndex = FindMemoryType(displayManager->physicalDevice, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryTypeIndex == UINT32_MAX) goto fail;

    // Allocate image memory
    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = requirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };
    if (vkAllocateMemory(displayManager->device, &allocInfo, NULL, &imageResource->memory) != VK_SUCCESS) goto fail;
    if (vkBindImageMemory(displayManager->device, imageResource->image, imageResource->memory, 0) != VK_SUCCESS) goto fail;

    // --- Staging buffer ---------------------------------------------------
    VkBufferCreateInfo bufferInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = imageSize,
        .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    if (vkCreateBuffer(displayManager->device, &bufferInfo, NULL, &stagingBuffer) != VK_SUCCESS) goto fail;

    VkMemoryRequirements bufReqs;
    vkGetBufferMemoryRequirements(displayManager->device, stagingBuffer, &bufReqs);

    u32 stagingTypeIndex = FindMemoryType(
        displayManager->physicalDevice,bufReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if (stagingTypeIndex == UINT32_MAX) goto fail_staging;

    VkMemoryAllocateInfo stagingAlloc = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = bufReqs.size,
        .memoryTypeIndex = stagingTypeIndex
    };
    if (vkAllocateMemory(displayManager->device, &stagingAlloc, NULL, &stagingMemory) != VK_SUCCESS) goto fail_staging;
    if (vkBindBufferMemory(displayManager->device, stagingBuffer, stagingMemory, 0) != VK_SUCCESS) goto fail_staging;

    void* mapped = NULL;
    if (vkMapMemory(displayManager->device, stagingMemory, 0, imageSize, 0, &mapped) != VK_SUCCESS) goto fail_staging;
    memcpy(mapped, buffer, (size_t)imageSize);
    vkUnmapMemory(displayManager->device, stagingMemory);

    // --- Record upload commands ------------------------------------------
    VkCommandBufferAllocateInfo cmdAlloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = displayManager->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    if (vkAllocateCommandBuffers(displayManager->device, &cmdAlloc, &cmd) != VK_SUCCESS) goto fail_staging;

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cmd, &beginInfo);

    // UNDEFINED -> TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier toTransfer = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = 0,
        .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = imageResource->image,
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1, &toTransfer);

    VkBufferImageCopy region = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 }
    };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, imageResource->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier toShader = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = imageResource->image,
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, NULL, 0, NULL, 1, &toShader);

    vkEndCommandBuffer(cmd);

    // --- Submit and wait --------------------------------------------------
    VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    if (vkCreateFence(displayManager->device, &fenceInfo, NULL, &fence) != VK_SUCCESS) goto fail_cmd;

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd
    };
    if (vkQueueSubmit(displayManager->graphicsQueue, 1, &submit, fence) != VK_SUCCESS) goto fail_fence;
    vkWaitForFences(displayManager->device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(displayManager->device, fence, NULL);
    vkFreeCommandBuffers(displayManager->device, displayManager->commandPool, 1, &cmd);
    vkDestroyBuffer(displayManager->device, stagingBuffer, NULL);
    vkFreeMemory(displayManager->device, stagingMemory, NULL);

    // --- Create the view --------------------------------------------------
    VkImageViewCreateInfo viewInfo = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = imageResource->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = format,
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    if (vkCreateImageView(displayManager->device, &viewInfo, NULL, &imageResource->view) != VK_SUCCESS) goto fail;

    imageResource->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return 1;

fail_fence:
    vkDestroyFence(displayManager->device, fence, NULL);
fail_cmd:
    vkFreeCommandBuffers(displayManager->device, displayManager->commandPool, 1, &cmd);
fail_staging:
    if (stagingBuffer) vkDestroyBuffer(displayManager->device, stagingBuffer, NULL);
    if (stagingMemory) vkFreeMemory(displayManager->device, stagingMemory, NULL);
fail:
    if (imageResource->view) vkDestroyImageView(displayManager->device, imageResource->view, NULL);
    if (imageResource->image) vkDestroyImage(displayManager->device, imageResource->image, NULL);
    if (imageResource->memory) vkFreeMemory(displayManager->device, imageResource->memory, NULL);
    *imageResource = (ImageResource){0};
    return 0;
}

/* Initialises an image resource manager and its underlying datastructures */
void ImageResourceManager_Init(ImageResourceManager* resourceManager)
{
    resourceManager->buildersCreated = 0;
    Array_Init(&resourceManager->imageViews, sizeof(ImageView), 128);
    Array_Init(&resourceManager->imageResources, sizeof(ImageResource), 64);
    LinearStringmap_Init(&resourceManager->filepathToHandleMap, sizeof(int), 20, 512);
}

/* Frees datastructures and image memory belonging to an image resource manager */
void ImageResourceManager_Free(ImageResourceManager* resourceManager)
{
    // Free Image Resources
    for (size_t i=0; i<Array_Length(&resourceManager->imageResources); i++) {
        ImageResource* image = Array_Get(&resourceManager->imageResources, i);
    }
    Array_Free(&resourceManager->imageResources);

    // Free Image Views
    Array_Free(&resourceManager->imageViews);

    // Free Atlas Builders
    for (int i=0; i<resourceManager->buildersCreated; i++) {
        AtlasBuilder_Free(&resourceManager->builders[i]);
    }

    // Free filepath map
    LinearStringmap_Free(&resourceManager->filepathToHandleMap);
}

/* Creates image resources / atlases from any remaining "in-progress" atlas builds */
void ImageResourceManager_CreateImageResourcesFromPartialAtlasBuilds(ImageResourceManager* resourceManager, DisplayManager* displayManager)
{
    for (int i=0; i<resourceManager->buildersCreated; i++) {
        AtlasBuilder* builder = &resourceManager->builders[i];

        // Builder is empty -> skip
        if (builder->imageResourceIndex == -1) continue;

        // Create image resource
        ImageResource* imageResource = Array_Get(&resourceManager->imageResources, builder->imageResourceIndex);
        ImageResource_Create(displayManager, builder->w, builder->h, builder->buffer, imageResource);

        // Clear builder
        AtlasBuilder_Clear(builder);
    }
}

/* Returns the width and height of an image, given an image handle */
void ImageResourceManager_GetImageSize(ImageResourceManager* resourceManager, int imageHandle, u16* w, u16* h)
{
    ImageView* imageView = Array_Get(&resourceManager->imageViews, imageHandle - 1);
    *w = imageView->w;
    *h = imageView->h;
}

/* Returns image handle of a loaded image, given its filepath */
int ImageResourceManager_ImageHandleFromFilepath(ImageResourceManager* resourceManager, const char* filepath)
{
    void* found = LinearStringmap_Get(&resourceManager->filepathToHandleMap, filepath);
    if (found == NULL) return 0;
    return *(int*)found;
}

/* Loads an image from disk, small images are packed into atlasses. returns image handle */

int ImageResourceManager_LoadImage(ImageResourceManager* resourceManager, DisplayManager* displayManager, const char* filepath)
{
    // Load from disk using STB_Image
    int w, h, n;
    unsigned char* buffer = stbi_load(filepath, &w, &h, &n, 4); // Force RGBA
    if (!buffer) return 0;

    int imageHandle;

    // If image is small -> pack into an atlas builder
    if (w <= 128 && h <= 128)
    {
        int atlasSize = 256;

        // Search atlas builders to find one that the image can fit into
        AtlasBuilder* builder = NULL;
        for (int i=0; i<resourceManager->buildersCreated; i++) {
            AtlasBuilder* candidateBuilder = &resourceManager->builders[i];
            if (AtlasBuilder_CanFit(candidateBuilder, w, h)) {
                builder = candidateBuilder;
                break;
            }
        }

        // Did not find a builder
        if (!builder)
        {
            // Less than 3 builders have been created -> create a new one lazily
            if (resourceManager->buildersCreated < 3)
            {
                // Create builder
                builder = &resourceManager->builders[resourceManager->buildersCreated];
                AtlasBuilder_Init(builder, 256, 256);

                // Create a new empty image resource struct for the new builder
                Array_PushEmpty(&resourceManager->imageResources);
                int imageResourceIndex = Array_Length(&resourceManager->imageResources) - 1;
                builder->imageResourceIndex = imageResourceIndex;

                // Increment
                resourceManager->buildersCreated++;
            }
            // Commit the most full builder atlas, clear and reuse it
            else
            {
                // Find the most full atlas builder
                AtlasBuilder* mostFullBuilder = &resourceManager->builders[0];
                u16 maxShelfHeight = mostFullBuilder->shelfHeight;
                for (int i=1; i<resourceManager->buildersCreated; i++) {
                    AtlasBuilder* candidateBuilder = &resourceManager->builders[i];
                    if (candidateBuilder->shelfHeight > maxShelfHeight) {
                        maxShelfHeight = candidateBuilder->shelfHeight;
                        mostFullBuilder = candidateBuilder;
                    }
                }

                // Create the image resource that the most full builder was working on
                ImageResource* imageResource = Array_Get(&resourceManager->imageResources, mostFullBuilder->imageResourceIndex);
                ImageResource_Create(displayManager, w, h, mostFullBuilder->buffer, imageResource);

                // Set and clear
                builder = mostFullBuilder;
                AtlasBuilder_Clear(mostFullBuilder);

                // Create a new empty image resource struct for the recycled builder
                Array_PushEmpty(&resourceManager->imageResources);
                int imageResourceIndex = Array_Length(&resourceManager->imageResources) - 1;
                builder->imageResourceIndex = imageResourceIndex;
            }
        }

        // Blit image onto builder atlas and create image view
        u16 x, y;
        AtlasBuilder_AddImage(builder, buffer, w, h, &x, &y);
        ImageView* imageView = Array_PushEmpty(&resourceManager->imageViews);
        imageView->x = x;
        imageView->y = y;
        imageView->w = w;
        imageView->h = y;
        imageView->imageResourceIndex = builder->imageResourceIndex;

        // Get image handle
        imageHandle = Array_Length(&resourceManager->imageViews);
    }
    // Image is large
    else
    {
        // Create an image resource
        ImageResource* imageResource = Array_PushEmpty(&resourceManager->imageResources);
        int resourceIndex = Array_Length(&resourceManager->imageResources) - 1;
        ImageResource_Create(displayManager, w, h, buffer, imageResource);

        // Create an image view
        ImageView* imageView = Array_PushEmpty(&resourceManager->imageViews);
        imageView->x = 0;
        imageView->y = 0;
        imageView->w = w;
        imageView->h = h;
        imageView->imageResourceIndex = resourceIndex;

        // Get image handle
        imageHandle = Array_Length(&resourceManager->imageViews);
    }

    // Add mapping from filepath to handle
    LinearStringmap_Set(&resourceManager->filepathToHandleMap, filepath, &imageHandle);

    // Success
    return imageHandle;
}

inline int ImageResourceManager_GetImageHandle(ImageResourceManager* resourceManager, const char* filepath)
{
    void* found = LinearStringmap_Get(&resourceManager->filepathToHandleMap, filepath);
    if (found == NULL) return 0;
    return *(int*)found;
}

/*  */
inline ImageResource* ImageResourceManager_GetImageResource(ImageResourceManager* resourceManager, int imageHandle)
{
    ImageView* imageView = Array_Get(&resourceManager->imageViews, imageHandle - 1);
    ImageResource* imageResource = Array_Get(&resourceManager->imageResources, imageView->imageResourceIndex);
    return imageResource;
}

inline ImageView* ImageResourceManager_GetImageView(ImageResourceManager* resourceManager, int imageHandle)
{
    ImageView* imageView = Array_Get(&resourceManager->imageViews, imageHandle - 1);
    return imageView;
}
