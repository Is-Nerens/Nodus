#pragma once

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout setLayout;
    VkImageLayout layout;
    u32 width;
    u32 height;
    bool mono;
} FontImageGPU;

void FontImageGPU_Create(
    FontImageGPU* image,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 width, u32 height,
    const unsigned char* buffer,
    bool mono)
{
    // 1. Validate dimensions and pixel buffer
    if (width == 0 || height == 0 || buffer == NULL) return;

    // 1 channel (R8) for mono coverage atlases, 3 channels (RGB8) for colour
    const u32 channels  = mono ? 1u : 4u;
    const VkFormat format = mono ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;

    VkDeviceSize imageSize = (VkDeviceSize)width * height * channels;

    // Track which handles we've created so a failure midway through this
    // function can tear down exactly what it made and leave nothing behind
    VkImage        localImage  = VK_NULL_HANDLE;
    VkDeviceMemory localMemory = VK_NULL_HANDLE;
    VkImageView    localView   = VK_NULL_HANDLE;
    VkSampler      localSampler = VK_NULL_HANDLE;
    VkDescriptorPool localPool = VK_NULL_HANDLE;
    VkDescriptorSet  localSet  = VK_NULL_HANDLE;
    VkDescriptorSetLayout localSetLayout = VK_NULL_HANDLE;

    // Staging upload resources. Tracked at function scope so a failure
    // anywhere in the upload path (or later in this function) is torn down
    // by the same cleanup label that destroys the image/view/etc.
    VkBuffer       localStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory localStagingMemory = VK_NULL_HANDLE;
    VkCommandBuffer localCmd          = VK_NULL_HANDLE;
    void*          localStagingMapped = NULL;

    // 2. Create VkImage
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageInfo, NULL, &localImage) != VK_SUCCESS) goto cleanup;
    image->image = localImage;

    // 3. Query memory requirements
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, localImage, &memReqs);

    // 4. Allocate suitable device-local memory
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) goto cleanup;
    if (vkAllocateMemory(device, &allocInfo, NULL, &localMemory) != VK_SUCCESS) goto cleanup;
    image->memory = localMemory;

    // 5. Bind memory
    if (vkBindImageMemory(device, localImage, localMemory, 0) != VK_SUCCESS) goto cleanup;

    // 6. Create VkImageView
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = localImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, NULL, &localView) != VK_SUCCESS) goto cleanup;
    image->view = localView;

    // 7. Upload pixels using a staging buffer
    VkBufferCreateInfo stagingBufferInfo = {0};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &stagingBufferInfo, NULL, &localStagingBuffer) != VK_SUCCESS) goto cleanup;

    VkMemoryRequirements stagingMemReqs;
    vkGetBufferMemoryRequirements(device, localStagingBuffer, &stagingMemReqs);

    VkMemoryAllocateInfo stagingAllocInfo = {0};
    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocInfo.allocationSize = stagingMemReqs.size;
    stagingAllocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, stagingMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (stagingAllocInfo.memoryTypeIndex == UINT32_MAX) goto cleanup;
    if (vkAllocateMemory(device, &stagingAllocInfo, NULL, &localStagingMemory) != VK_SUCCESS) goto cleanup;
    if (vkBindBufferMemory(device, localStagingBuffer, localStagingMemory, 0) != VK_SUCCESS) goto cleanup;

    if (vkMapMemory(device, localStagingMemory, 0, imageSize, 0, &localStagingMapped) != VK_SUCCESS) goto cleanup;
    memcpy(localStagingMapped, buffer, (size_t)imageSize);
    vkUnmapMemory(device, localStagingMemory);
    localStagingMapped = NULL;

    VkCommandBufferAllocateInfo cmdAllocInfo = {0};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device, &cmdAllocInfo, &localCmd) != VK_SUCCESS) goto cleanup;

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(localCmd, &beginInfo) != VK_SUCCESS) goto cleanup;

    // UNDEFINED -> TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = localImage;
    barrier.subresourceRange = viewInfo.subresourceRange;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(localCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    // Copy buffer to image. bufferRowLength == 0 means "tightly packed at
    // width * channels bytes per row", which matches our staging data
    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){0, 0, 0};
    region.imageExtent = (VkExtent3D){width, height, 1};
    vkCmdCopyBufferToImage(localCmd, localStagingBuffer, localImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(localCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
    if (vkEndCommandBuffer(localCmd) != VK_SUCCESS) goto cleanup;

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &localCmd;
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) goto cleanup;
    if (vkQueueWaitIdle(graphicsQueue) != VK_SUCCESS) goto cleanup;

    vkFreeCommandBuffers(device, commandPool, 1, &localCmd);
    localCmd = VK_NULL_HANDLE;
    vkDestroyBuffer(device, localStagingBuffer, NULL);
    localStagingBuffer = VK_NULL_HANDLE;
    vkFreeMemory(device, localStagingMemory, NULL);
    localStagingMemory = VK_NULL_HANDLE;

    // 8. Create the sampler.
    // Linear filtering + CLAMP_TO_EDGE is the right default for a font
    // atlas. If neighbouring glyphs bleed into each other, switch to
    // VK_FILTER_NEAREST or add padding between glyphs in the atlas builder
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
    if (vkCreateSampler(device, &samplerInfo, NULL, &localSampler) != VK_SUCCESS) goto cleanup;
    image->sampler = localSampler;

    // 9. Create the descriptor set layout inline. This is the "no helper,
    // no parameter" choice: every FontImageGPU owns its own layout, and
    // NU_CreateMonoTextPipeline creates its own layout with the SAME
    // binding description. Per Vulkan spec, layouts with identical
    // (binding, type, count, stageFlags) are compatible, so a set
    // allocated from this layout is valid for a pipeline built with the
    // other one. The two descriptions MUST be kept in sync — see the
    // matching block in NU_CreateMonoTextPipeline
    VkDescriptorSetLayoutBinding layoutBinding = {0};
    layoutBinding.binding         = 0;
    layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 8;
    layoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &layoutBinding;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &localSetLayout) != VK_SUCCESS) goto cleanup;
    image->setLayout = localSetLayout;

    // 10. Create a per-atlas descriptor pool and allocate the font set.
    VkDescriptorPoolSize poolSize = {0};
    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 8;
    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets       = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    if (vkCreateDescriptorPool(device, &poolInfo, NULL, &localPool) != VK_SUCCESS) goto cleanup;
    image->descriptorPool = localPool;

    VkDescriptorSetAllocateInfo setAllocInfo = {0};
    setAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool     = localPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts        = &localSetLayout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &localSet) != VK_SUCCESS || localSet == VK_NULL_HANDLE) goto cleanup;
    image->descriptorSet = localSet;

    // 11. Write the descriptor set.
    // The shader declares sampler2D uFontTextures[8]; every element of the
    // array must be a valid descriptor at draw time per the Vulkan spec.
    // For now we only have one atlas, so point all 8 slots at it. When
    // multiple atlases exist, write distinct views per slot and set
    // GlyphRenderData.textureIndex accordingly
    VkDescriptorImageInfo imageInfoDesk[8];
    for (int i = 0; i < 8; i++) {
        imageInfoDesk[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfoDesk[i].imageView   = localView;
        imageInfoDesk[i].sampler     = localSampler;
    }
    VkWriteDescriptorSet descriptorWrite = {0};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = localSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 8;
    descriptorWrite.pImageInfo = imageInfoDesk;
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);

    // 12. Store dimensions and final layout
    image->mono = mono;
    image->width  = width;
    image->height = height;
    image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return;

cleanup:
    // Tear down whatever we managed to create, in reverse order of creation.
    // The handles on `image` have already been assigned for the ones that
    // succeeded, so a subsequent FontImageGPU_Destroy would also be safe.
    // Staging resources are destroyed first because they were created last
    if (localStagingMapped != NULL)    vkUnmapMemory(device, localStagingMemory);
    if (localCmd != VK_NULL_HANDLE)    vkFreeCommandBuffers(device, commandPool, 1, &localCmd);
    if (localStagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, localStagingBuffer, NULL);
    if (localStagingMemory != VK_NULL_HANDLE) vkFreeMemory(device, localStagingMemory, NULL);
    if (localPool != VK_NULL_HANDLE)      vkDestroyDescriptorPool(device, localPool, NULL);
    if (localSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, localSetLayout, NULL);
    if (localSampler != VK_NULL_HANDLE)   vkDestroySampler(device, localSampler, NULL);
    if (localView != VK_NULL_HANDLE)      vkDestroyImageView(device, localView, NULL);
    if (localImage != VK_NULL_HANDLE)     vkDestroyImage(device, localImage, NULL);
    if (localMemory != VK_NULL_HANDLE)    vkFreeMemory(device, localMemory, NULL);

    image->image          = VK_NULL_HANDLE;
    image->memory         = VK_NULL_HANDLE;
    image->view           = VK_NULL_HANDLE;
    image->sampler        = VK_NULL_HANDLE;
    image->descriptorPool = VK_NULL_HANDLE;
    image->descriptorSet  = VK_NULL_HANDLE;
    image->setLayout      = VK_NULL_HANDLE;
}

void FontImageGPU_Patch(
    FontImageGPU* image,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 x, u32 y, u32 width, u32 height,
    const unsigned char* buffer)
{
    // 1. Validate the image and patch bounds
    if (!image || !image->image || !buffer) return;
    if (width == 0 || height == 0) return;
    if (x >= image->width || y >= image->height) return;
    if (x + width  > image->width  || y + height > image->height) return;
    if (image->descriptorSet == VK_NULL_HANDLE) return;
    const VkDeviceSize bytesPerPixel = image->mono ? 1 : 4; // R8_UNORM or VK_FORMAT_R8G8B8A8_UNORM
    const VkDeviceSize patchSize = (VkDeviceSize)width * height * bytesPerPixel;

    // The image must be in SHADER_READ_ONLY_OPTIMAL at entry.
    // Its oldLayout tells us what to restore to after the patch; we always
    // come back to SHADER_READ_ONLY_OPTIMAL, but we need an accurate
    // oldLayout for the first barrier
    const VkImageLayout restoreLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const VkImageLayout currentLayout = image->layout; // should be SHADER_READ_ONLY_OPTIMAL

    // 2. Upload patch pixels to a staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkBufferCreateInfo stagingBufferInfo = {0};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = patchSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer);
    VkMemoryRequirements stagingMemReqs;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingMemReqs);
    VkMemoryAllocateInfo stagingAllocInfo = {0};
    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocInfo.allocationSize = stagingMemReqs.size;
    stagingAllocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, stagingMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &stagingAllocInfo, NULL, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);
    void* data = NULL;
    vkMapMemory(device, stagingBufferMemory, 0, patchSize, 0, &data);
    memcpy(data, buffer, (size_t)patchSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Begin recording.
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAllocInfo = {0};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // 3. Transition image to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = currentLayout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Coming from SHADER_READ_ONLY we only need to flush shader reads;
    // from UNDEFINED (first use) no source access is needed
    if (currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else {
        barrier.srcAccessMask = 0;
    }
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // prior consumers
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier);

    // 4. Copy the patch using vkCmdCopyBufferToImage
    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;    // tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){ (int32_t)x, (int32_t)y, 0 };
    region.imageExtent = (VkExtent3D){ width, height, 1 };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 5. Transition back to SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier postBarrier = {0};
    postBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    postBarrier.newLayout = restoreLayout;
    postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postBarrier.image = image->image;
    postBarrier.subresourceRange = barrier.subresourceRange;
    postBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    postBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &postBarrier);

    // End + submit
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);

    // 6. Retire staging resources after GPU completion
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);

    // Bookkeeping: layout is unchanged, but keep the field authoritative
    image->layout = restoreLayout;
}

void FontImageGPU_Resize(
    FontImageGPU* image,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 width, u32 height,
    const unsigned char* buffer)
{
    // 1. Validate the image, dimensions and pixel buffer
    if (!image || !buffer) return;
    if (width == 0 || height == 0) return;
    if (image->sampler == VK_NULL_HANDLE || image->descriptorSet == VK_NULL_HANDLE) return;

    // A no-op resize is common (caller rebuilt at the same size) — short-circuit.
    // If you also want to refresh contents at the same size, use FontImageGPU_Patch
    if (image->image != VK_NULL_HANDLE &&
        width == image->width && height == image->height)
    {
        // Still valid to overwrite contents wholesale; do it via the patch
        // path over the full extent to avoid churning handles
        FontImageGPU_Patch(
            image,
            device,
            physicalDevice,
            commandPool,
            graphicsQueue,
            0, 0, width, height, buffer);
        return;
    }

    // 1 channel (R8) for mono coverage atlases, 3 channels (RGB8) for colour.
    // Resize preserves the existing channel count; it does not switch format
    const u32 channels  = image->mono ? 1u : 4u;
    const VkFormat format = image->mono ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    const VkDeviceSize imageSize = (VkDeviceSize)width * height * channels;

    // 2. Create a replacement image with the new dimensions
    VkImage newImage = VK_NULL_HANDLE;
    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(device, &imageInfo, NULL, &newImage);

    // 3. Allocate and bind device memory for the replacement image
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, newImage, &memReqs);
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) return;
    VkDeviceMemory newMemory = VK_NULL_HANDLE;
    vkAllocateMemory(device, &allocInfo, NULL, &newMemory);
    vkBindImageMemory(device, newImage, newMemory, 0);

    // 4. Create an image view for the replacement image
    VkImageView newView = VK_NULL_HANDLE;
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = newImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, NULL, &newView);

    // 5. Upload the pixel buffer into the replacement image
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkBufferCreateInfo stagingBufferInfo = {0};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer);
    VkMemoryRequirements stagingMemReqs;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingMemReqs);
    VkMemoryAllocateInfo stagingAllocInfo = {0};
    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocInfo.allocationSize = stagingMemReqs.size;
    stagingAllocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, stagingMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &stagingAllocInfo, NULL, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void* data = NULL;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, buffer, (size_t)imageSize);
    vkUnmapMemory(device, stagingBufferMemory);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAllocInfo = {0};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = newImage;
    barrier.subresourceRange = viewInfo.subresourceRange;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier);

    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){0, 0, 0};
    region.imageExtent = (VkExtent3D){width, height, 1};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, newImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 6. Transition the replacement image to shader-readable layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);

    // 7. Update the descriptor to reference the replacement image view.
    // Sampler is unchanged; only the view (and thus the image) changes
    VkDescriptorImageInfo descriptorImageInfo = {0};
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = newView;
    descriptorImageInfo.sampler = image->sampler;
    VkWriteDescriptorSet descriptorWrite = {0};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = image->descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &descriptorImageInfo;
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);

    // 8. Ensure no pending GPU work still references the old resources.
    // endSingleTimeCommands already synchronized the upload, but the *old*
    // image may still be referenced by draws recorded in in-flight command
    // buffers from previous frames. vkDeviceWaitIdle is the blunt but safe
    // choice; a fence-per-frame scheme (wait on the last submit that sampled
    // the atlas) is cheaper if you have one
    vkDeviceWaitIdle(device);

    // 9. Destroy the old image view and image, then free its memory
    if (image->view != VK_NULL_HANDLE) vkDestroyImageView(device, image->view, NULL);
    if (image->image != VK_NULL_HANDLE) vkDestroyImage(device, image->image, NULL);
    if (image->memory != VK_NULL_HANDLE) vkFreeMemory(device, image->memory, NULL);

    // 10. Store the new image handles, dimensions and layout
    image->image  = newImage;
    image->memory = newMemory;
    image->view   = newView;
    image->width  = width;
    image->height = height;
    image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void FontImageGPU_Destroy(FontImageGPU* image, VkDevice device)
{
    // 1. Validate the image pointer
    if (!image) return;

    // Nothing to do if the object was already destroyed or never created
    // (Handles are invalidated to VK_NULL_HANDLE by the memset at the end,
    // so a double-call is harmless.)
    if (image->image == VK_NULL_HANDLE &&
        image->view  == VK_NULL_HANDLE &&
        image->memory == VK_NULL_HANDLE &&
        image->sampler == VK_NULL_HANDLE &&
        image->descriptorPool == VK_NULL_HANDLE &&
        image->descriptorSet == VK_NULL_HANDLE)
    {
        return;
    }

    // 2. Ensure no pending GPU work is using the image resources
    // A draw recorded in an in-flight command buffer may still sample this
    // atlas; destroying the view/image underneath it is a use-after-free on
    // the GPU. vkDeviceWaitIdle is the blunt-but-safe choice. If you have
    // per-frame fences, prefer waiting only on the last submission that could
    // have sampled this image (e.g. the current frame's fence)
    vkDeviceWaitIdle(device);

    // 3. Destroy the descriptor pool. This implicitly frees every set that
    // was allocated from it, so there is no need to call vkFreeDescriptorSets
    // (which would only be legal if the pool had been created with
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT and the set were not
    // still bound in a pending command buffer — neither of which we rely on).
    // Do this before tearing down the sampler/view/image so the set stops
    // referencing them first
    if (image->descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, image->descriptorPool, NULL);
    }
    image->descriptorSet = VK_NULL_HANDLE;

    // 3b. Destroy the set layout owned by this atlas. It is no longer
    // referenced now that the pool and set are gone. Other atlases have
    // their own compatible layouts, and the pipeline layout was built
    // from a separate compatible layout, so this is safe
    if (image->setLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, image->setLayout, NULL);
    }

    // 4. Destroy the sampler. It is owned by this struct (created in
    // FontImageGPU_Create) and is not referenced by anything else once the
    // pool above has been destroyed
    if (image->sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, image->sampler, NULL);
    }

    // 5. Destroy the image view.
    if (image->view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, image->view, NULL);
    }

    // 6. Destroy the image.
    if (image->image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image->image, NULL);
    }

    // 7. Free the image's device memory
    // Order matters: the memory must outlive the image bound to it, so this
    // comes strictly after vkDestroyImage
    if (image->memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, image->memory, NULL);
    }

    // 8. Reset the structure to zero and invalidate its handles.
    // Zeroing the whole struct invalidates every handle and scalar in one
    // shot, which also makes a second call a no-op via the guard in step 1
    memset(image, 0, sizeof(*image));
}
