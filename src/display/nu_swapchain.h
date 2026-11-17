#pragma once

#define NU_FRAMES_IN_FLIGHT 2
#define NU_MAX_SWAPCHAIN_IMAGES 4

typedef struct {
    VkSwapchainKHR  swapchain;
    VkFormat        swapchainFormat;
    VkExtent2D      swapchainExtent;
    uint32          imageCount;
    VkImage         images[NU_MAX_SWAPCHAIN_IMAGES];
    VkImageView     imageViews[NU_MAX_SWAPCHAIN_IMAGES];
    VkFramebuffer   framebuffers[NU_MAX_SWAPCHAIN_IMAGES];
    VkImage         depthImage;
    VkDeviceMemory  depthImageMemory;
    VkImageView     depthImageView;
    VkSemaphore     imageAvailable[NU_FRAMES_IN_FLIGHT];
    VkFence         inFlightFences[NU_FRAMES_IN_FLIGHT];
    VkSemaphore     renderFinished[NU_MAX_SWAPCHAIN_IMAGES];
    uint32          currentFrame;
    uint32          imageIndex;
    uint64          presentId;
    bool            suboptimal;
} Swapchain;

static int Swapchain_CreateSemaphores(Swapchain* swapchain, VkDevice device)
{
    VkSemaphoreCreateInfo semci = {0};
    semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semci, NULL, &swapchain->imageAvailable[i]) != VK_SUCCESS) return 0;
    }
    for (uint32 i=0; i<NU_MAX_SWAPCHAIN_IMAGES; i++) {
        if (vkCreateSemaphore(device, &semci, NULL, &swapchain->renderFinished[i]) != VK_SUCCESS)  return 0;
    }
    return 1;
}

static void Swapchain_DestroySemaphores(Swapchain* swapchain, VkDevice device)
{
    for (uint32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        if (swapchain->imageAvailable[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(device, swapchain->imageAvailable[i], NULL);
        swapchain->imageAvailable[i] = VK_NULL_HANDLE;
    }
    for (uint32 i=0; i<NU_MAX_SWAPCHAIN_IMAGES; i++) {
        if (swapchain->renderFinished[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(device, swapchain->renderFinished[i], NULL);
        swapchain->renderFinished[i] = VK_NULL_HANDLE;
    }
}

int Swapchain_Create(Swapchain* swapchain, VkDevice device)
{
    swapchain->swapchain = VK_NULL_HANDLE;
    swapchain->swapchainFormat = VK_FORMAT_UNDEFINED;
    swapchain->swapchainExtent = (VkExtent2D){0};

    for (uint32 i=0; i<NU_MAX_SWAPCHAIN_IMAGES; i++)
    {
        swapchain->images[i] = VK_NULL_HANDLE;
        swapchain->imageViews[i] = VK_NULL_HANDLE;
        swapchain->framebuffers[i] = VK_NULL_HANDLE;
        swapchain->renderFinished[i] = VK_NULL_HANDLE;
    }

    for (uint32 i=0; i<NU_FRAMES_IN_FLIGHT; i++)
    {
        swapchain->imageAvailable[i] = VK_NULL_HANDLE;
        swapchain->inFlightFences[i] = VK_NULL_HANDLE;
    }

    swapchain->depthImage = VK_NULL_HANDLE;
    swapchain->depthImageMemory = VK_NULL_HANDLE;
    swapchain->depthImageView = VK_NULL_HANDLE;
    swapchain->currentFrame = 0;
    swapchain->imageCount = 0;
    swapchain->imageIndex = 0;
    swapchain->presentId = 0;
    swapchain->suboptimal = false;

    // Create fences
    VkFenceCreateInfo fci = {0};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (uint32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(device, &fci, NULL, &swapchain->inFlightFences[i]) != VK_SUCCESS) {
            return 0;
        }
    }

    // Create semaphores
    if (!Swapchain_CreateSemaphores(swapchain, device)) return 0;

    // Success
    return 1;
}

void Swapchain_DestroyImages(Swapchain* swapchain, VkDevice device)
{
    // Destroy framebuffers and image views
    for (uint32 i=0; i<swapchain->imageCount; i++) {
        if (swapchain->framebuffers[i] != VK_NULL_HANDLE) vkDestroyFramebuffer(device, swapchain->framebuffers[i], NULL);
        if (swapchain->imageViews[i] != VK_NULL_HANDLE) vkDestroyImageView(device, swapchain->imageViews[i], NULL);
        swapchain->framebuffers[i] = VK_NULL_HANDLE;
        swapchain->imageViews[i] = VK_NULL_HANDLE;
        swapchain->images[i] = VK_NULL_HANDLE;
    }
    swapchain->imageCount = 0;

    // Destroy depth buffer
    if (swapchain->depthImageView != VK_NULL_HANDLE) vkDestroyImageView(device, swapchain->depthImageView, NULL);
    if (swapchain->depthImage != VK_NULL_HANDLE) vkDestroyImage(device, swapchain->depthImage, NULL);
    if (swapchain->depthImageMemory != VK_NULL_HANDLE) vkFreeMemory(device, swapchain->depthImageMemory, NULL);
    swapchain->depthImage = VK_NULL_HANDLE;
    swapchain->depthImageView = VK_NULL_HANDLE;
    swapchain->depthImageMemory = VK_NULL_HANDLE;

    // Destroy the old swapchain
    if (swapchain->swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, swapchain->swapchain, NULL);
    swapchain->swapchain = VK_NULL_HANDLE;
}

int Swapchain_CreateImages(
    Swapchain* swapchain,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkRenderPass renderPass,
    Surface surface,
    int width,
    int height)
{
    if (vkDeviceWaitIdle(device) != VK_SUCCESS) return 0; // Catastrophic error

    Swapchain_DestroyImages(swapchain, device);

    // Recreate semaphores
    Swapchain_DestroySemaphores(swapchain, device);
    if (!Swapchain_CreateSemaphores(swapchain, device)) return 0;
    swapchain->currentFrame = 0;

    // Commit the format
    swapchain->swapchainFormat = surface.surfaceFormat.format;

    // Choose the extent
    VkSurfaceCapabilitiesKHR caps;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,
        surface.surface,
        &caps) != VK_SUCCESS)
    {
        return 0;
    }
    VkExtent2D extent;
    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        extent = caps.currentExtent;
    } else {
        extent.width = (uint32)width < caps.minImageExtent.width? caps.minImageExtent.width : ((uint32)width > caps.maxImageExtent.width ? caps.maxImageExtent.width : (uint32)width);
        extent.height = (uint32)height < caps.minImageExtent.height ? caps.minImageExtent.height : ((uint32)height > caps.maxImageExtent.height ? caps.maxImageExtent.height : (uint32)height);
    }
    if (extent.width == 0 || extent.height == 0) {
        return 0;
    }
    swapchain->swapchainExtent = extent;

    // Create the depth buffer
    VkImageCreateInfo dci = {0};
    dci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    dci.imageType = VK_IMAGE_TYPE_2D;
    dci.format = VK_FORMAT_D32_SFLOAT;
    dci.extent.width = extent.width;
    dci.extent.height = extent.height;
    dci.extent.depth = 1;
    dci.mipLevels = 1;
    dci.arrayLayers = 1;
    dci.samples = VK_SAMPLE_COUNT_1_BIT;
    dci.tiling = VK_IMAGE_TILING_OPTIMAL;
    dci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    dci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    dci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device, &dci, NULL, &swapchain->depthImage) != VK_SUCCESS) {
        return 0;
    }
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, swapchain->depthImage, &memReq);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    uint32 typeIndex = 0;
    for (uint32 i=0; i<memProps.memoryTypeCount; i++) {
        if ((memReq.memoryTypeBits & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            typeIndex = i;
            break;
        }
    }
    VkMemoryAllocateInfo mai = {0};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memReq.size;
    mai.memoryTypeIndex = typeIndex;
    if (vkAllocateMemory(device, &mai, NULL, &swapchain->depthImageMemory) != VK_SUCCESS) {
        return 0;
    }
    vkBindImageMemory(device, swapchain->depthImage, swapchain->depthImageMemory, 0);
    VkImageViewCreateInfo dvci = {0};
    dvci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    dvci.image = swapchain->depthImage;
    dvci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    dvci.format = VK_FORMAT_D32_SFLOAT;
    dvci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    dvci.subresourceRange.baseMipLevel = 0;
    dvci.subresourceRange.levelCount = 1;
    dvci.subresourceRange.baseArrayLayer = 0;
    dvci.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &dvci, NULL, &swapchain->depthImageView) != VK_SUCCESS) {
        return 0;
    }

    // Choose the minimum number of swapchain images to request
    uint32 requestedImageCount = 2;
    if (requestedImageCount < caps.minImageCount) requestedImageCount = caps.minImageCount;
    if (caps.maxImageCount != 0 && requestedImageCount > caps.maxImageCount) {
        requestedImageCount = caps.maxImageCount;
    }

    // Create the swapchain
    VkSwapchainCreateInfoKHR sci = {0};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = surface.surface;
    sci.minImageCount = requestedImageCount;
    sci.imageFormat = surface.surfaceFormat.format;
    sci.imageColorSpace = surface.surfaceFormat.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &sci, NULL, &swapchain->swapchain) != VK_SUCCESS) {
        return 0;
    }

    // Get the image count
    uint32 actualImageCount = 0;
    VkResult result = vkGetSwapchainImagesKHR(device, swapchain->swapchain, &actualImageCount, NULL);
    if (result != VK_SUCCESS || actualImageCount == 0 || actualImageCount > NU_MAX_SWAPCHAIN_IMAGES) return 0;
    result = vkGetSwapchainImagesKHR(device, swapchain->swapchain, &actualImageCount, swapchain->images);
    if (result != VK_SUCCESS) return 0;
    swapchain->imageCount = actualImageCount;

    // Create image views
    for (uint32 i=0; i<swapchain->imageCount; i++)
    {
        VkImageViewCreateInfo vci = {0};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = swapchain->images[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = surface.surfaceFormat.format;
        vci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel = 0;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &vci, NULL, &swapchain->imageViews[i]) != VK_SUCCESS) {
            return 0;
        }

        // Create frame buffers
        VkImageView fbAttachments[2] = { swapchain->imageViews[i], swapchain->depthImageView };
        VkFramebufferCreateInfo fci = {0};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = renderPass;
        fci.attachmentCount = 2;
        fci.pAttachments = fbAttachments;
        fci.width = extent.width;
        fci.height = extent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(device, &fci, NULL, &swapchain->framebuffers[i]) != VK_SUCCESS) {
            return 0;
        }
    }

    // Success
    return 1;
}

void Swapchain_Destroy(Swapchain* swapchain, VkDevice device)
{
    // Destroy frences
    for (uint32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        if (swapchain->inFlightFences[i] != VK_NULL_HANDLE) vkDestroyFence(device, swapchain->inFlightFences[i], NULL);
        swapchain->inFlightFences[i] = VK_NULL_HANDLE;
    }

    // Destroy semaphores
    Swapchain_DestroySemaphores(swapchain, device);

    // Destroy Images
    Swapchain_DestroyImages(swapchain, device);
}

int Swapchain_AquireNextImage(Swapchain* swapchain, VkDevice device)
{
    // Wait until frame is available
    if (vkWaitForFences(
        device, 1,
        &swapchain->inFlightFences[swapchain->currentFrame],
        VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        return 0;
    }

    // Acquire next image
    VkResult result = vkAcquireNextImageKHR(
        device,
        swapchain->swapchain,
        UINT64_MAX,
        swapchain->imageAvailable[swapchain->currentFrame],
        VK_NULL_HANDLE,
        &swapchain->imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return 2;
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        swapchain->suboptimal = true;
        return 3;
    }
    if (result != VK_SUCCESS) return 0;

    // Success
    return 1;
}

int Swapchain_PrepareSubmission(Swapchain* swapchain, VkDevice device)
{
    // Wait for the image-acquired semaphore
    VkSemaphore waitSemaphore = swapchain->imageAvailable[swapchain->currentFrame];

    // Signal when rendering to this swapchain image is finished.
    VkSemaphore signalSemaphore = swapchain->renderFinished[swapchain->imageIndex];
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // Reset fence
    if (vkResetFences(device, 1, &swapchain->inFlightFences[swapchain->currentFrame]) != VK_SUCCESS) {
        return 0;
    }

    // Success
    return 1;
}

inline void Swapchain_AdvanceFrame(Swapchain* swapchain)
{
    swapchain->currentFrame = (swapchain->currentFrame + 1) % NU_FRAMES_IN_FLIGHT;

}

inline uint32 Swapchain_CurrentFrame(Swapchain* swapchain)
{
    return swapchain->currentFrame;
}

inline uint32 Swapchain_CurrentImageIndex(Swapchain* swapchain)
{
    return swapchain->imageIndex;
}

inline VkFramebuffer Swapchain_CurrentFramebuffer(Swapchain* swapchain)
{
    return swapchain->framebuffers[swapchain->imageIndex];
}

inline VkSemaphore Swapchain_CurrentRenderSemaphore(Swapchain* swapchain)
{
    return swapchain->renderFinished[swapchain->imageIndex];
}

inline VkSemaphore Swapchain_CurrentImageSemaphore(Swapchain* swapchain)
{
    return swapchain->imageAvailable[swapchain->currentFrame];
}

inline float Swapchain_Width(Swapchain* swapchain)
{
    return (float)swapchain->swapchainExtent.width;
}

inline float Swapchain_Height(Swapchain* swapchain)
{
    return (float)swapchain->swapchainExtent.height;
}
