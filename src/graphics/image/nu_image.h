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

    // Staging buffer
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

    // Record upload commands
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

    // Submit and wait
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

    // Create the view
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


/* Creates the shared Vulkan infrastructure for image group descriptor sets:
 * layout, pool, sampler, and a 1x1 white dummy image used to fill unused
 * slots in a partial last group. Call once, after DisplayManager has a
 * valid device, and before ImageResourceManager_BuildGroupDescriptorSets. */
int ImageResourceManager_CreateVulkanDevices(ImageResourceManager* resourceManager, DisplayManager* displayManager, u32 maxGroups)
{
    VkDevice device = displayManager->device;
    VkPhysicalDevice physicalDevice = displayManager->physicalDevice;

    // 1. Descriptor set layout: binding 0 = sampler2D uTextures[8], fragment stage
    VkDescriptorSetLayoutBinding texBinding = {0};
    texBinding.binding            = 0;
    texBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding.descriptorCount    = 8;
    texBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    texBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo lci = {0};
    lci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    lci.bindingCount = 1;
    lci.pBindings    = &texBinding;
    if (vkCreateDescriptorSetLayout(device, &lci, NULL, &resourceManager->setLayout) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> image descriptor set layout failed");
        return 0;
    }

    // 2. Descriptor pool: maxGroups sets × 8 combined-image-samplers each
    VkDescriptorPoolSize poolSize = {0};
    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = maxGroups * 8;

    VkDescriptorPoolCreateInfo pci = {0};
    pci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.maxSets       = maxGroups;
    pci.poolSizeCount = 1;
    pci.pPoolSizes    = &poolSize;
    if (vkCreateDescriptorPool(device, &pci, NULL, &resourceManager->pool) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device, resourceManager->setLayout, NULL);
        resourceManager->setLayout = VK_NULL_HANDLE;
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> image descriptor pool failed");
        return 0;
    }

    // 3. Shared sampler: linear + clamp-to-edge
    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter    = VK_FILTER_LINEAR;
    samplerInfo.minFilter    = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxLod       = 0.0f;
    samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    if (vkCreateSampler(device, &samplerInfo, NULL, &resourceManager->sampler) != VK_SUCCESS) {
        vkDestroyDescriptorPool(device, resourceManager->pool, NULL);
        vkDestroyDescriptorSetLayout(device, resourceManager->setLayout, NULL);
        resourceManager->pool = VK_NULL_HANDLE;
        resourceManager->setLayout = VK_NULL_HANDLE;
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> image sampler failed");
        return 0;
    }

    // 4. 1x1 white dummy image (fills unused slots in a partial last group)
    const u32 dw = 1, dh = 1;
    const unsigned char whitePixel[4] = { 255, 255, 255, 255 };

    VkImageCreateInfo ici = {0};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = VK_FORMAT_R8G8B8A8_UNORM;
    ici.extent.width  = dw;
    ici.extent.height = dh;
    ici.extent.depth  = 1;
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device, &ici, NULL, &resourceManager->dummyImage) != VK_SUCCESS) {
        vkDestroySampler(device, resourceManager->sampler, NULL);
        vkDestroyDescriptorPool(device, resourceManager->pool, NULL);
        vkDestroyDescriptorSetLayout(device, resourceManager->setLayout, NULL);
        resourceManager->sampler = VK_NULL_HANDLE;
        resourceManager->pool = VK_NULL_HANDLE;
        resourceManager->setLayout = VK_NULL_HANDLE;
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> image dummy image failed");
        return 0;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, resourceManager->dummyImage, &memReqs);

    VkMemoryAllocateInfo mai = {0};
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize  = memReqs.size;
    mai.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mai.memoryTypeIndex == UINT32_MAX ||
        vkAllocateMemory(device, &mai, NULL, &resourceManager->dummyMemory) != VK_SUCCESS) {
        vkDestroyImage(device, resourceManager->dummyImage, NULL);
        vkDestroySampler(device, resourceManager->sampler, NULL);
        vkDestroyDescriptorPool(device, resourceManager->pool, NULL);
        vkDestroyDescriptorSetLayout(device, resourceManager->setLayout, NULL);
        resourceManager->dummyImage = VK_NULL_HANDLE;
        resourceManager->sampler = VK_NULL_HANDLE;
        resourceManager->pool = VK_NULL_HANDLE;
        resourceManager->setLayout = VK_NULL_HANDLE;
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> image dummy memory failed");
        return 0;
    }
    vkBindImageMemory(device, resourceManager->dummyImage, resourceManager->dummyMemory, 0);

    // Upload the white pixel via staging + one-time command buffer
    VkBuffer       stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkBufferCreateInfo bci = {0};
    bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size        = 4;
    bci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &bci, NULL, &stagingBuffer);

    VkMemoryRequirements bufReqs;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &bufReqs);
    VkMemoryAllocateInfo smai = {0};
    smai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    smai.allocationSize  = bufReqs.size;
    smai.memoryTypeIndex = FindMemoryType(physicalDevice, bufReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &smai, NULL, &stagingMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    void* mapped = NULL;
    vkMapMemory(device, stagingMemory, 0, 4, 0, &mapped);
    memcpy(mapped, whitePixel, 4);
    vkUnmapMemory(device, stagingMemory);

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo cbai = {0};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = displayManager->commandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &cbai, &cmd);

    VkCommandBufferBeginInfo cbbi = {0};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    VkImageMemoryBarrier b = {0};
    b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image               = resourceManager->dummyImage;
    b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.baseMipLevel   = 0;
    b.subresourceRange.levelCount     = 1;
    b.subresourceRange.baseArrayLayer = 0;
    b.subresourceRange.layerCount     = 1;
    b.srcAccessMask = 0;
    b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &b);

    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageExtent = (VkExtent3D){ dw, dh, 1 };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, resourceManager->dummyImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    b.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &b);

    vkEndCommandBuffer(cmd);
    VkSubmitInfo si = {0};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    vkQueueSubmit(displayManager->graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(displayManager->graphicsQueue);
    vkFreeCommandBuffers(device, displayManager->commandPool, 1, &cmd);
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingMemory, NULL);

    // Image view for the dummy
    VkImageViewCreateInfo vci = {0};
    vci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.image    = resourceManager->dummyImage;
    vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vci.format   = VK_FORMAT_R8G8B8A8_UNORM;
    vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = 1;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = 1;
    if (vkCreateImageView(device, &vci, NULL, &resourceManager->dummyView) != VK_SUCCESS) {
        vkDestroyImage(device, resourceManager->dummyImage, NULL);
        vkFreeMemory(device, resourceManager->dummyMemory, NULL);
        vkDestroySampler(device, resourceManager->sampler, NULL);
        vkDestroyDescriptorPool(device, resourceManager->pool, NULL);
        vkDestroyDescriptorSetLayout(device, resourceManager->setLayout, NULL);
        resourceManager->dummyImage = VK_NULL_HANDLE;
        resourceManager->dummyMemory = VK_NULL_HANDLE;
        resourceManager->sampler = VK_NULL_HANDLE;
        resourceManager->pool = VK_NULL_HANDLE;
        resourceManager->setLayout = VK_NULL_HANDLE;
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> image dummy view failed");
        return 0;
    }

    return 1;
}

/* Initialises an image resource manager and its underlying datastructures */
void ImageResourceManager_Init(ImageResourceManager* resourceManager, DisplayManager* displayManager)
{
    resourceManager->buildersCreated = 0;
    Array_Init(&resourceManager->groupDescriptorSets, sizeof(VkDescriptorSet), 8);
    Array_Init(&resourceManager->imageViews, sizeof(ImageView), 128);
    Array_Init(&resourceManager->imageResources, sizeof(ImageResource), 64);
    LinearStringmap_Init(&resourceManager->filepathToHandleMap, sizeof(int), 20, 512);
    ImageResourceManager_CreateVulkanDevices(resourceManager, displayManager, 8);
}

/* Frees datastructures and image memory belonging to an image resource manager */
void ImageResourceManager_Free(ImageResourceManager* resourceManager, DisplayManager* displayManager)
{
    VkDevice device = displayManager->device;

    // Destroy descriptor pool
    if (resourceManager->pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, resourceManager->pool, NULL);
        resourceManager->pool = VK_NULL_HANDLE;
    }

    // Free group descriptor set array
    Array_Free(&resourceManager->groupDescriptorSets);

    // Destroy set layout
    if (resourceManager->setLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, resourceManager->setLayout, NULL);
        resourceManager->setLayout = VK_NULL_HANDLE;
    }

    // The sampler is no longer referenced once the descriptor sets are gone.
    if (resourceManager->sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, resourceManager->sampler, NULL);
        resourceManager->sampler = VK_NULL_HANDLE;
    }

    // The 1x1 white dummy image: view, then image, then memory (in that
    // order — memory must outlive the image bound to it).
    if (resourceManager->dummyView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, resourceManager->dummyView, NULL);
        resourceManager->dummyView = VK_NULL_HANDLE;
    }
    if (resourceManager->dummyImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, resourceManager->dummyImage, NULL);
        resourceManager->dummyImage = VK_NULL_HANDLE;
    }
    if (resourceManager->dummyMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, resourceManager->dummyMemory, NULL);
        resourceManager->dummyMemory = VK_NULL_HANDLE;
    }

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

void ImageResourceManager_BuildGroupDescriptorSets(ImageResourceManager* resourceManager, DisplayManager* displayManager)
{
    u32 total = (u32)Array_Length(&resourceManager->imageResources);
    if (total == 0) return;

    VkDevice device = displayManager->device;

    for (u32 i=0; i<total; i+= 8) {

        // Create a descriptor set for this group
        VkDescriptorSetAllocateInfo allocInfo = {0};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = resourceManager->pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &resourceManager->setLayout;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            return;
        }

        // Fill 8 slots. Slots past the real image count use the dummy view,
        // so every element of sampler2D uTextures[8] is valid at draw time.
        VkDescriptorImageInfo info[8];
        for (u32 slot=0; slot<8; slot++) {
            info[slot].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            info[slot].sampler = resourceManager->sampler;
            u32 imageIndex = i + slot;
            if (imageIndex < total) {
                ImageResource* resource = Array_Get(&resourceManager->imageResources, imageIndex);
                info[slot].imageView = resource->view;
            } else {
                info[slot].imageView = resourceManager->dummyView;
            }
        }

        // Write descriptor sets
        VkWriteDescriptorSet write = {0};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = descriptorSet;
        write.dstBinding      = 0;
        write.dstArrayElement = 0;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 8;
        write.pImageInfo      = info;
        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

        // Add group descripto set
        Array_Push(&resourceManager->groupDescriptorSets, &descriptorSet);
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
                ImageResource_Create(displayManager, mostFullBuilder->w, mostFullBuilder->h, mostFullBuilder->buffer, imageResource);

                // Set and clear
                builder = mostFullBuilder;
                AtlasBuilder_Clear(mostFullBuilder);

                // Create a new empty image resource struct for the recycled builder
                Array_PushEmpty(&resourceManager->imageResources);
                int imageResourceIndex = Array_Length(&resourceManager->imageResources) - 1;
                builder->imageResourceIndex = imageResourceIndex;
            }
        }

        // Blit image onto builder atlas and compute UVs
        u16 x, y;
        AtlasBuilder_AddImage(builder, buffer, w, h, &x, &y);
        ImageView* imageView = Array_PushEmpty(&resourceManager->imageViews);
        float atlasW = (float)builder->w;
        float atlasH = (float)builder->h;
        imageView->u0 = (float)x / atlasW;
        imageView->v0 = (float)y / atlasH;
        imageView->u1 = (float)(x + w) / atlasW;
        imageView->v1 = (float)(y + h) / atlasH;
        imageView->w = w;
        imageView->h = h;
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
        imageView->u0 = 0.0f;
        imageView->v0 = 0.0f;
        imageView->u1 = 1.0f;
        imageView->v1 = 1.0f;
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
