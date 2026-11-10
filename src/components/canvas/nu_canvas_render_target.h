#pragma once

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkImageLayout currentLayout;
    uint16 width;
    uint16 height;

    // MSAA colour attachment
    VkImage msaaColorImage;
    VkDeviceMemory msaaColorMemory;
    VkImageView msaaColorView;

    // MSAA depth attachment
    VkImage msaaDepthImage;
    VkDeviceMemory msaaDepthMemory;
    VkImageView msaaDepthView;

    // Binds { msaaColorView, msaaDepthView, view } into the shared canvas pass
    VkFramebuffer framebuffer;

    // Set 0 for the image pipeline: uTextures[8], allocated from the image
    // resource manager's shared pool. Slot 0 = view, slots 1-7 = dummy.
    VkDescriptorSet descriptorSet;
} CanvasRenderTarget;

// Helper: create a single attachment (image + memory + view).
// Caller must zero-initialise outputs. On failure, all outputs are cleaned and
// zeroed, so partial state is never left behind.
static int CanvasRenderTarget_CreateAttachment(
    DisplayManager* displayManager,
    VkImage* image,
    VkDeviceMemory* memory,
    VkImageView* view,
    VkFormat format,
    VkImageUsageFlags usage,
    VkSampleCountFlagBits samples,
    VkImageAspectFlags aspect,
    uint32 width,
    uint32 height)
{
    // Create image
    VkImageCreateInfo imageInfo = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = { width, height, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = samples,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    if (vkCreateImage(displayManager->device, &imageInfo, NULL, image) != VK_SUCCESS) goto fail;

    // Allocate memory
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(displayManager->device, *image, &requirements);

    uint32 memoryTypeIndex = FindMemoryType(
        displayManager->physicalDevice,
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    if (memoryTypeIndex == UINT32_MAX) goto fail;

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = requirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };
    if (vkAllocateMemory(displayManager->device, &allocInfo, NULL, memory) != VK_SUCCESS) goto fail;
    if (vkBindImageMemory(displayManager->device, *image, *memory, 0) != VK_SUCCESS) goto fail;

    // Create view
    VkImageViewCreateInfo viewInfo = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = *image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = format,
        .subresourceRange = {
            .aspectMask     = aspect,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    if (vkCreateImageView(displayManager->device, &viewInfo, NULL, view) != VK_SUCCESS) goto fail;
    return 1;

fail:
    if (*view)   vkDestroyImageView(displayManager->device, *view, NULL);
    if (*image)  vkDestroyImage(displayManager->device, *image, NULL);
    if (*memory) vkFreeMemory(displayManager->device, *memory, NULL);
    *view   = VK_NULL_HANDLE;
    *image  = VK_NULL_HANDLE;
    *memory = VK_NULL_HANDLE;
    return 0;
}

// (Re)create the framebuffer that binds the canvas attachments into the shared
// canvas pass. Dimensions must be the current logical extent — on shrink-reuse
// resize the images stay larger, but the framebuffer must match the new extent.
static int CanvasRenderTarget_CreateFramebuffer(
    DisplayManager* displayManager,
    CanvasRenderTarget* renderTarget)
{
    if (renderTarget->framebuffer)
        vkDestroyFramebuffer(displayManager->device, renderTarget->framebuffer, NULL);

    VkImageView attachments[3] = {
        renderTarget->msaaColorView,
        renderTarget->msaaDepthView,
        renderTarget->view
    };

    VkFramebufferCreateInfo fbInfo = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = displayManager->sharedRenderPassMSAA4X,
        .attachmentCount = 3,
        .pAttachments    = attachments,
        .width           = renderTarget->width,
        .height          = renderTarget->height,
        .layers          = 1
    };
    return vkCreateFramebuffer(displayManager->device, &fbInfo, NULL, &renderTarget->framebuffer) == VK_SUCCESS;
}

// Allocate + write the per-canvas descriptor set from the image resource
// manager's shared pool/layout. Slot 0 is the resolved canvas view; slots 1-7
// point at the manager's 1x1 dummy so every slot is a valid write even though
// the shader only samples index 0.
static int CanvasRenderTarget_CreateDescriptorSet(
    DisplayManager* displayManager,
    ImageResourceManager* imageResourceManager,
    CanvasRenderTarget* renderTarget)
{
    VkDescriptorSetAllocateInfo alloc = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = imageResourceManager->pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &imageResourceManager->setLayout
    };
    if (vkAllocateDescriptorSets(displayManager->device, &alloc, &renderTarget->descriptorSet) != VK_SUCCESS)
        return 0;

    VkDescriptorImageInfo infos[8];
    infos[0].sampler     = imageResourceManager->sampler;
    infos[0].imageView   = renderTarget->view;
    infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    for (uint32 i = 1; i < 8; i++) {
        infos[i].sampler     = imageResourceManager->sampler;
        infos[i].imageView   = imageResourceManager->dummyView;
        infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = renderTarget->descriptorSet,
        .dstBinding      = 0,
        .descriptorCount = 8,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo      = infos
    };
    vkUpdateDescriptorSets(displayManager->device, 1, &write, 0, NULL);
    return 1;
}

// Tear down every GPU object owned by the render texture and zero it.
// The descriptor set is returned to the shared pool (requires
// VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT on that pool).
static void CanvasRenderTarget_Destroy(
    DisplayManager* displayManager,
    ImageResourceManager* imageResourceManager,
    CanvasRenderTarget* renderTarget)
{
    if (renderTarget->descriptorSet && imageResourceManager)
        vkFreeDescriptorSets(displayManager->device, imageResourceManager->pool,
                             1, &renderTarget->descriptorSet);
    if (renderTarget->framebuffer)     vkDestroyFramebuffer(displayManager->device, renderTarget->framebuffer, NULL);
    if (renderTarget->msaaDepthView)   vkDestroyImageView(displayManager->device, renderTarget->msaaDepthView, NULL);
    if (renderTarget->msaaDepthImage)  vkDestroyImage(displayManager->device, renderTarget->msaaDepthImage, NULL);
    if (renderTarget->msaaDepthMemory) vkFreeMemory(displayManager->device, renderTarget->msaaDepthMemory, NULL);
    if (renderTarget->msaaColorView)   vkDestroyImageView(displayManager->device, renderTarget->msaaColorView, NULL);
    if (renderTarget->msaaColorImage)  vkDestroyImage(displayManager->device, renderTarget->msaaColorImage, NULL);
    if (renderTarget->msaaColorMemory) vkFreeMemory(displayManager->device, renderTarget->msaaColorMemory, NULL);
    if (renderTarget->view)            vkDestroyImageView(displayManager->device, renderTarget->view, NULL);
    if (renderTarget->image)           vkDestroyImage(displayManager->device, renderTarget->image, NULL);
    if (renderTarget->memory)          vkFreeMemory(displayManager->device, renderTarget->memory, NULL);
    *renderTarget = (CanvasRenderTarget){0};
}

// Create a canvas render target: a resolved, sampled colour image plus an MSAA
// colour+depth pair used only inside the canvas pass. No alpha in the resolved
// image — it is cleared to the containing canvas's background colour, so the
// blend is a plain copy.
static int CanvasRenderTarget_Create(
    DisplayManager* displayManager,
    ImageResourceManager* imageResourceManager,
    CanvasRenderTarget* renderTarget,
    uint32 width,
    uint32 height)
{
    *renderTarget = (CanvasRenderTarget){0};

    // B8G8R8A8 is the only guaranteed colour-renderable format without alpha
    // on most drivers; if you can guarantee VK_FORMAT_B8G8R8A8_SRGB support,
    // use that instead for correct sRGB sampling.
    const VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

    // Resolved colour image (blit target, sampled by UI)
    renderTarget->width  = (uint16)width;
    renderTarget->height = (uint16)height;
    if (!CanvasRenderTarget_CreateAttachment(
        displayManager,
        &renderTarget->image,
        &renderTarget->memory,
        &renderTarget->view,
        format,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        width, height))
    {
        goto fail;
    }
    renderTarget->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // MSAA colour attachment (canvas pass only; never sampled)
    if (!CanvasRenderTarget_CreateAttachment(
        displayManager,
        &renderTarget->msaaColorImage,
        &renderTarget->msaaColorMemory,
        &renderTarget->msaaColorView,
        format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        width, height))
    {
        goto fail;
    }

    // MSAA depth attachment
    if (!CanvasRenderTarget_CreateAttachment(
        displayManager,
        &renderTarget->msaaDepthImage,
        &renderTarget->msaaDepthMemory,
        &renderTarget->msaaDepthView,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        width, height))
    {
        goto fail;
    }

    // Framebuffer binding the three views into the shared 4x pass
    if (!CanvasRenderTarget_CreateFramebuffer(displayManager, renderTarget))
        goto fail;

    // Descriptor set from the image resource manager's shared pool/layout
    if (!CanvasRenderTarget_CreateDescriptorSet(displayManager, imageResourceManager, renderTarget))
        goto fail;

    return 1;

fail:
    CanvasRenderTarget_Destroy(displayManager, imageResourceManager, renderTarget);
    return 0;
}

// Resize the canvas render target in place.
//
// Strategy:
//   - Shrinking (both dimensions <= current): REUSE all existing images.
//     The resolve blit targets the lower-left subregion, so no allocation
//     churn. The framebuffer still has to be recreated because its extent must
//     match the new logical extent, but no images are touched.
//   - Any growth: destroy and recreate. Vulkan requires the render area at
//     vkCmdBeginRenderPass to equal the attachment extent exactly, so a larger
//     extent cannot reuse a smaller allocation. Content is lost.
static int CanvasRenderTarget_Resize(
    DisplayManager* displayManager,
    ImageResourceManager* imageResourceManager,
    CanvasRenderTarget* renderTarget,
    uint32 width,
    uint32 height)
{
    if (width == 0 || height == 0 || width > UINT16_MAX || height > UINT16_MAX) return 0;

    if (vkDeviceWaitIdle(displayManager->device) != VK_SUCCESS) return 0;

    // Shrink-only: reuse images, recreate framebuffer at the new extent.
    // Descriptor set is untouched: the view is the same image view.
    if (width <= renderTarget->width && height <= renderTarget->height)
    {
        renderTarget->width  = (uint16)width;
        renderTarget->height = (uint16)height;
        return CanvasRenderTarget_CreateFramebuffer(displayManager, renderTarget);
    }

    // Grow (or mixed): recreate at the new extent. Content is lost.
    CanvasRenderTarget_Destroy(displayManager, imageResourceManager, renderTarget);
    return CanvasRenderTarget_Create(displayManager, imageResourceManager, renderTarget, width, height);
}

// Rapidly clear the canvas: the resolved colour image is cleared to `color`
// (alpha ignored) with vkCmdClearColorImage on a one-shot command buffer, so
// it runs entirely on the GPU — no CPU work proportional to canvas size.
//
// Leaves the resolved image in SHADER_READ_ONLY_OPTIMAL, ready to sample in the
// UI pass. The MSAA colour and depth images are not touched: the canvas render
// pass clears both via loadOp = CLEAR on every begin.
static int CanvasRenderTarget_Clear(
    DisplayManager* displayManager,
    CanvasRenderTarget* renderTarget,
    NuColor color)
{
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkFence fence       = VK_NULL_HANDLE;
    int ok = 0;

    VkCommandBufferAllocateInfo cmdAlloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = displayManager->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    if (vkAllocateCommandBuffers(displayManager->device, &cmdAlloc, &cmd) != VK_SUCCESS) goto fail;

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Transition resolved image -> TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier toDst = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = 0,
        .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout           = renderTarget->currentLayout,
        .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = renderTarget->image,
        .subresourceRange    = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1, .layerCount = 1
        }
    };
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 0, NULL, 1, &toDst);

    // Clear resolved colour (alpha ignored)
    VkClearColorValue clearColor = {
        .float32 = {
            (float)color.r / 255.0f,
            (float)color.g / 255.0f,
            (float)color.b / 255.0f,
            1.0f
        }
    };
    VkImageSubresourceRange colorRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1, .layerCount = 1
    };
    vkCmdClearColorImage(cmd, renderTarget->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &colorRange);

    // Resolved -> SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier toShader = toDst;
    toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    toShader.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShader.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, NULL, 0, NULL, 1, &toShader);

    vkEndCommandBuffer(cmd);

    // Submit and wait
    VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    if (vkCreateFence(displayManager->device, &fenceInfo, NULL, &fence) != VK_SUCCESS) goto fail;

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd
    };
    if (vkQueueSubmit(displayManager->graphicsQueue, 1, &submit, fence) != VK_SUCCESS) goto fail;
    vkWaitForFences(displayManager->device, 1, &fence, VK_TRUE, UINT64_MAX);

    renderTarget->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ok = 1;

fail:
    if (fence) vkDestroyFence(displayManager->device, fence, NULL);
    if (cmd)   vkFreeCommandBuffers(displayManager->device, displayManager->commandPool, 1, &cmd);
    return ok;
}
