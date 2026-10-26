#pragma once

/* Contains definitions and implentation of image resource management for Nodus
 * This includes image data and handles, atlases, filepath -> handle mapping,
 * Image loading / unloading, atlas packing, image CPU/GPU memory management
 */

#include "nu_atlas.h"
#include <stddef.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "nu_atlas_builder.h"

// Image Handle Functions
static inline bool NU_Image_Is_Handle_Standalone(int h) { return (h & 0xFFFF) == 0xFFFF; }
static inline int NU_Image_Handle_To_Standalone_Index(int h) { return (h >> 16) & 0xFFFFu; }
static inline int NU_Image_Handle_To_Atlas_Index(int h) { return ((h >> 16) & 0xFFFFu) - 1; }
static inline int NU_Image_Handle_To_Atlas_Image_Index(int h) { return (h & 0xFFFFu) - 1; }
static inline int NU_Image_Create_Standalone_Handle(int resourceIndex) { return ((resourceIndex & 0xFFFFu) << 16) | 0xFFFFu; }
static inline int NU_Image_Create_Atlas_Handle(int atlasIndex, int imageIndex) { return (((atlasIndex + 1) & 0xFFFFu) << 16) | ((imageIndex + 1) & 0xFFFFu); }

// Represents and image that can exist on the GPU
typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkDescriptorSet descriptor;
    VkFormat format;
    u16 width;
    u16 height;
} ImageResource;

// Represents a region of an individual image that belongs to an atlas
typedef struct {
    float u0, v0;
    float u1, v1;
    u16 w, h;
} AtlasRegion;

// Contains "alas image regions" belonging to a shared image resource
typedef struct {
    AtlasBuilder builder;
    Array atlasRegions;     // Array of <AtlasRegion>
    int imageResourceIndex; // Index of an <ImageResource>
} Atlas;

// Entire image resource manager struct
typedef struct {
    LinearStringmap filepathToHandleMap;
    Array atlasses;
    Array imageResources;
} ImageResourceManager;

/* Creates an image resource that Vulkan can work with */
int ImageResource_Create(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    u32 width,
    u32 height,
    ImageResource* out)
{
    // Result
    *out = (ImageResource){0};

    // Create image
    out->width  = (u16)width;
    out->height = (u16)height;
    out->format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageCreateInfo imageInfo = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = out->format,
        .extent        = { width, height, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    if (vkCreateImage(device, &imageInfo, NULL, &out->image) != VK_SUCCESS) goto fail;

    // Get memory requirements
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device, out->image, &requirements);

    // Implement this helper using vkGetPhysicalDeviceMemoryProperties.
    u32 memoryTypeIndex;
    if (!FindMemoryType(
            physicalDevice,
            requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &memoryTypeIndex)) goto fail;

    // Allocate image memory
    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = requirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };
    if (vkAllocateMemory(device, &allocInfo, NULL, &out->memory) != VK_SUCCESS) goto fail;
    if (vkBindImageMemory(device, out->image, out->memory, 0) != VK_SUCCESS) goto fail;

    // Create a view
    VkImageViewCreateInfo viewInfo = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = resource->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = resource->format,
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    if (vkCreateImageView(device, &viewInfo, NULL, &resource->view) != VK_SUCCESS) goto fail

    // Blah blah blah
    // on and on it goes....

    // Success
    return 1;

fail:
    if (out->memory) vkFreeMemory(device, out->memory, NULL);
    if (out->image) vkDestroyImage(device, out->image, NULL);
    *out = (ImageResource){0};
    return 0;
}

/* Initialises an image resource manager and its underlying datastructures */
void ImageResourceManager_Init(ImageResourceManager* resourceManager)
{
    Array_Init(&resourceManager->atlasses, sizeof(Atlas), 8);
    Array_Init(&resourceManager->imageResources, sizeof(ImageResource), 64);
    LinearStringmap_Init(&resourceManager->filepathToHandleMap, sizeof(int), 20, 512);
    resourceManager->atlasBuilderCount = 0;
}

/* Frees datastructures and image memory belonging to an image resource manager */
void ImageResourceManager_Free(ImageResourceManager* resourceManager)
{
    // Free Images
    for (size_t i=0; i<Array_Length(resourceManager->imageResources); i++) {
        // Free image
        ImageResource* image = Array_Get(resourceManager->imageResources, i);
    }
    Array_Free(resourceManager->imageResources);

    // Free Atlases
    for (size_t i=0; i<Array_Length(resourceManager->atlasses); i++) {
        Atlas* atlas = Array_Get(resourceManager->atlasses, i);
        Array_Free(&atlas->atlasImages);
    }
    Array_Free(&resourceManager->atlasses);

    // Free Atlas Builders
    for (u8 i=0; i<resourceManager->atlasBuilderCount; i++) {
        AtlasBuilder_Free(&resourceManager->atlasBuilders[i]);
    }

    // Free filepath map
    LinearStringmap_Free(&resourceManager->filepathToHandleMap);
}

/* Creates image resources / atlases from any remaining "in-progress" atlas builds */
void ImageResourceManager_CreateAtlasImageResources(ImageResourceManager* resourceManager)
{
    for (size_t i=0; i<Array_Length(&resourceManager->atlasses); i++) {
        Atlas* atlas = Array_Get(&resourceManager->atlasses, i);

        // Create image resource
        ImageResource* imageResource = Array_PushEmpty(&resourceManager->imageResources);
        int resourceIndex = Array_Length(&resourceManager->imageResources) - 1;
        imageHandle = NU_Image_Create_Standalone_Handle(resourceIndex);
        ImageResource_Create(physicalDevice, device, w, h, imageResource);
        atlas->imageResourceIndex = resourceIndex;
    }
    resourceManager->atlasBuilderCount = 0;
}

/* Returns the width and height of an image, given an image handle */
void ImageResourceManager_GetImageSize(ImageResourceManager* resourceManager, int imageHandle, u16* w, u16* h)
{
    // Image is standalone
    if (NU_Image_Is_Handle_Standalone(imageHandle)) {
        int imageIndex = NU_Image_Handle_To_Standalone_Index(imageHandle);
        StandaloneImageInfo* info = Array_Get(&resourceManager->standaloneImageInfos, imageIndex);
        *w = info->w;
        *h = info->h;
    }
    // Image is atlased
    else {
        int atlasIndex = NU_Image_Handle_To_Atlas_Index(imageHandle);
        int imageIndex = NU_Image_Handle_To_Atlas_Image_Index(imageHandle);
        Atlas* atlas = Array_Get(&resourceManager->atlases, atlasIndex);
        AtlasImageInfo* img = Array_Get(&atlas->images, imageIndex);
        *w = img->w;
        *h = img->h;
    }
}

/* Returns image handle of a loaded image, given its filepath */
int ImageResourceManager_ImageHandleFromFilepath(ImageResourceManager* resourceManager, const char* filepath)
{
    void* found = LinearStringmap_Get(&resourceManager->filepathToHandleMap, filepath);
    if (found == NULL) return 0;
    return *(int*)found;
}

/* Loads an image from disk, returns image handle */
int ImageResourceManager_LoadImage(ImageResourceManager* resourceManager, const char* filepath)
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

        // Search the last 3 atlases to find one that the image can fit into
        int atlasIndex = -1;
        int start = Array_Length(&resourceManager->atlasses) - 3;
        if (start < 0) start = 0;
        for (int i=start; u<Array_Length(&resourceManager->atlasses); i++) {
            Atlas* candidate = Array_Get(&resourceManager->atlasses, i);
            if (AtlasBuilder_CanFit(&candidate->builder, w, h)) {
                atlas = candidate;
                atlasIndex = i;
                break;
            }
        }

        // No suitable atlas was found -> create a new one and a corresponding image resource
        if (atlasIndex == -1)
        {
            // Create a new atlas
            Atlas* newAtlas = Array_PushEmpty(&resourceManager->atlasses);
            Array_Init(&newAtlas->atlasRegions, sizeof(AtlasRegion), 16);
            AtlasBuilder_Init(&newAtlas->builder, atlasSize, atlasSize);
            newAtlas->imageResourceIndex = ...;
            atlasIndex = Array_Length(&resourceManager->atlasses) - 1;

            // Create an image resource
            ImageResource* resource = Array_Push(&resourceManager->imageResources);
            newAtlas->imageResourceIndex = Array_Length(&resourceManager->imageResources) - 1;
        }

        // Get atlas
        Atlas* atlas = Array_Get(&resourceManager->atlasses, atlasIndex);

        // Add image to atlas builder
        int regionX; int regionY
        AtlasBuilder_AddImage(&atlas->builder, buffer, w, h, &regionX, &regionY);
        stbi_image_free(buffer);

        // Create Atlas region
        AtlasRegion* newRegion = Array_PushEmpty(&atlas->atlasRegions);
        float inverseAtlasSize = 1.0f / (float)atlasSize;
        newRegion->u0 = (float)regionX * inverseAtlasSize;
        newRegion->v0 = (float)regionY * inverseAtlasSize;
        newRegion->u1 = (float)(regionX + w) * inverseAtlasSize;
        newRegion->v1 = (float)(regionY + h) * inverseAtlasSize;
        newRegion->w = w;
        newRegion->h = h;
        int imageIndex = Array_Length(&atlas->atlasRegions) - 1;

        // Create handle
        imageHandle = NU_Image_Create_Atlas_Handle(atlasIndex, imageIndex);
    }
    // Image is too large to be packed into an atlas
    else
    {
        ImageResource* imageResource = Array_PushEmpty(&resourceManager->imageResources);
        int resourceIndex = Array_Length(&resourceManager->imageResources) - 1;
        imageHandle = NU_Image_Create_Standalone_Handle(resourceIndex);
        ImageResource_Create(physicalDevice, device, w, h, imageResource);
    }

    // Add mapping from filepath to handle
    LinearStringmap_Add(&resourceManager->filepathToHandleMap, filepath, &imageHandle);

    // Success
    return imageHandle;
}

/* Appends the appropriate ImageRenderData to an array, given an image handle */
void ImageResourceManager_AddImageRenderData(ImageResourceManager* resourceManager, int imageHandle, ImageRenderData* renderData)
{
    // Image is standalone
    if (NU_Image_Is_Handle_Standalone(imageHandle)) {
        int imageIndex = NU_Image_Handle_To_Standalone_Index(imageHandle);
        StandaloneImageInfo* info = Array_Get(&resourceManager->standaloneImageInfos, imageIndex);
        StandaloneImageRenderData* sRenderData = Array_PushEmpty(&resourceManager->standaloneImageRenderDatas);
        renderData->u0 = 0;
        renderData->v0 = 0;
        renderData->u1 = 1;
        renderData->v1 = 1;
        sRenderData->glImageHandle = info->glImageHandle;
        sRenderData->renderData = *renderData;
    }
    // Image is atlased
    else {
        int atlasIndex = NU_Image_Handle_To_Atlas_Index(imageHandle);
        int imageIndex = NU_Image_Handle_To_Atlas_Image_Index(imageHandle);
        Atlas* atlas = Array_Get(&resourceManager->atlases, atlasIndex);
        AtlasImageInfo* img = Array_Get(&atlas->images, imageIndex);
        renderData->u0 = img->u0;
        renderData->v0 = img->v0;
        renderData->u1 = img->u1;
        renderData->v1 = img->v1;
        Array_Push(&atlas->renderDataArray, renderData);
    }
}

/* Loads an image from disk, small images are automatically packed into an atlas */
int ImageResourceLoader_LoadImage(ImageResourceLoader* loader, const char* filepath)
{
    int w, h, n;
    unsigned char* buffer = stbi_load(filepath, &w, &h, &n, 4); // Force RGBA
    if (!buffer) return 0;

    int imageHandle;

    // Too large to be added to an atlas -> keep as individual image
    if (w > 128 || h > 128)
    {
        // Upload to GPU
        GLuint handle;
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
        stbi_image_free(buffer); // Free CPU memory

        // Add handle to large image handles array
        StandaloneImageInfo* info = Array_PushEmpty(&loader->resourceManager->standaloneImageInfos);
        info->glImageHandle = handle;
        info->w = w;
        info->h = h;
        int index = loader->resourceManager->standaloneImageInfos.size - 1;

        // Create handle
        imageHandle = NU_Image_Create_Standalone_Handle(index);
    }
    // Pack image into an atlas
    else
    {
        u16 atlasSize = 512;

        // Search the last 4 atlases to find one that the image can fit into
        AtlasBuild* validAtlasBuild = NULL;
        Atlas* atlas = NULL;
        int atlasIndex = 0;
        int start = loader->atlasBuilds.size - 4;
        if (start < 0) start = 0;
        for (int i = start; i < loader->atlasBuilds.size; i++) {
            AtlasBuild* atlasBuild = Array_Get(&loader->atlasBuilds, i);
            if (AtlasBuild_CanFit(atlasBuild, w, h)) {
                validAtlasBuild = atlasBuild;
                atlas = Array_Get(&loader->resourceManager->atlases, i);
                atlasIndex = i;
                break;
            }
        }

        // Did not find a valid atlas build -> create one
        if (!validAtlasBuild) {
            validAtlasBuild = Array_PushEmpty(&loader->atlasBuilds);
            AtlasBuild_Init(validAtlasBuild, atlasSize, atlasSize); // example size

            // Create a corresponding ImageResourceManager Atlas
            atlas = Array_PushEmpty(&loader->resourceManager->atlases);
            Array_Init(&atlas->images, sizeof(AtlasImageInfo), 32);
            Array_Init(&atlas->renderDataArray, sizeof(ImageRenderData), 32);
            atlasIndex = loader->resourceManager->atlases.size - 1;
        }

        // Pack image into atlas build
        int x, y;
        AtlasBuild_AddImage(validAtlasBuild, buffer, w, h, &x, &y);
        stbi_image_free(buffer);

        // Add image to resource manager
        AtlasImageInfo* newImage = Array_PushEmpty(&atlas->images);
        newImage->u0 = (float)x / (float)atlasSize;
        newImage->v0 = (float)y / (float)atlasSize;
        newImage->u1 = (float)(x + w) / (float)atlasSize;
        newImage->v1 = (float)(y + h) / (float)atlasSize;
        newImage->w = w;
        newImage->h = h;
        int imageIndex = atlas->images.size - 1;

        // Create handle
        imageHandle = NU_Image_Create_Atlas_Handle(atlasIndex, imageIndex);
    }

    LinearStringmap_Set(&loader->imageFilepathToHandleMap, filepath, &imageHandle);
    return imageHandle;
}
