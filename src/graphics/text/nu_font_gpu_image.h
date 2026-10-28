#pragma once

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
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
    vkCreateImage(device, &imageInfo, NULL, &image->image);

    // 3. Query memory requirements
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, image->image, &memReqs);

    // 4. Allocate suitable device-local memory
    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) return;
    vkAllocateMemory(device, &allocInfo, NULL, &image->memory);

    // 5. Bind memory
    vkBindImageMemory(device, image->image, image->memory, 0);

    // 6. Create VkImageView
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, NULL, &image->view);

    // 7. Upload pixels using a staging buffer
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
    void* data;
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

    // UNDEFINED -> TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image->image;
    barrier.subresourceRange = viewInfo.subresourceRange;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

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
    vkCmdCopyBufferToImage(cmd, stagingBuffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
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

    // 9. Update the descriptor set
    VkDescriptorImageInfo imageInfoDesk = {0};
    imageInfoDesk.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfoDesk.imageView = image->view;
    imageInfoDesk.sampler = image->sampler;
    VkWriteDescriptorSet descriptorWrite = {0};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = image->descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfoDesk;
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);

    // 10. Store dimensions and final layout
    image->mono = mono;
    image->width  = width;
    image->height = height;
    image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
    // (Handles are invalidated to VK_NULL_HANDLE in step 7, so a double-call
    // is harmless.)
    if (image->image == VK_NULL_HANDLE &&
        image->view  == VK_NULL_HANDLE &&
        image->memory == VK_NULL_HANDLE &&
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

    // 3. Free the descriptor set if individually allocated and permitted by its pool
    // Only valid if the set was allocated with VK_DESCRIPTOR_POOL_CREATE_FREE_
    // DESCRIPTOR_SET_BIT and is not still referenced by a pending
    // vkUpdateDescriptorSets or a bound command buffer. After the waitIdle
    // above there is no pending work, so the free is safe. If your pool was
    // created without the FREE flag, this call is a validation error — in
    // that case skip it and let the pool reclaim the set when it is reset/
    // destroyed. Make this conditional on how the set was allocated:
#if FONTIMAGE_DESCRIPTOR_SETS_ARE_FREEABLE
    if (image->descriptorSet != VK_NULL_HANDLE && image->descriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, image->descriptorPool, 1, &image->descriptorSet);
    }
#endif
    // If the set is not individually freeable, just drop the handle; the pool
    // owns its lifetime. Do NOT call vkFreeDescriptorSets in that case

    // 4. Destroy the image view.
    if (image->view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, image->view, NULL);
    }

    // 5. Destroy the image.
    if (image->image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image->image, NULL);
    }

    // 6. Free the image's device memory
    // Order matters: the memory must outlive the image bound to it, so this
    // comes strictly after vkDestroyImage
    if (image->memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, image->memory, NULL);
    }

    // 7. Reset the structure to zero and invalidate its handles.
    // The sampler (if owned by this struct) is intentionally *not* destroyed
    // here — see the note below. Zeroing the whole struct invalidates every
    // handle and scalar in one shot, which also makes a second call a no-op
    // via the guard in step 1
    memset(image, 0, sizeof(*image));
}
