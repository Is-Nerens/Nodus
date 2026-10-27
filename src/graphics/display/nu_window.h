#pragma once

int Window_CreateSwapchain(Window* window, DisplayManager* displayManager)
{
    VkDevice device = displayManager->device;

    // Wait for all rendering and presentation work to finish.
    if (vkDeviceWaitIdle(device) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkDeviceWaitIdle failed");
        return 0;
    }

    // Defer recreation while the window has no drawable area.
    int width, height;
    SDL_GetWindowSizeInPixels(window->sdlWin, &width, &height);
    if (width == 0 || height == 0) return 0;

    // Destroy old framebuffers and image views.
    for (u32 i=0; i<window->imageCount; i++) {
        if (window->framebuffers[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, window->framebuffers[i], NULL);
            window->framebuffers[i] = VK_NULL_HANDLE;
        }
        if (window->imageViews[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(device, window->imageViews[i], NULL);
            window->imageViews[i] = VK_NULL_HANDLE;
        }
    }

    // Destroy image-indexed presentation semaphores.
    for (u32 i = 0; i < window->imageCount; i++) {
        if (window->renderFinished[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, window->renderFinished[i], NULL);
            window->renderFinished[i] = VK_NULL_HANDLE;
        }
    }

    // Destroy the old swapchain.
    if (window->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, window->swapchain, NULL);
        window->swapchain = VK_NULL_HANDLE;
    }

    // Clear per-image handles so a failure leaves no stale state.
    for (u32 i=0; i<NU_MAX_SWAPCHAIN_IMAGES; i++) {
        window->images[i] = VK_NULL_HANDLE;
        window->imageViews[i] = VK_NULL_HANDLE;
        window->framebuffers[i] = VK_NULL_HANDLE;
    }
    window->imageCount = 0;

    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR caps;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        displayManager->physicalDevice,
        window->surface,
        &caps) != VK_SUCCESS) {
        return 0;
    }

    // Query supported surface formats.
    u32 formatCount = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
        displayManager->physicalDevice,
        window->surface,
        &formatCount,
        NULL) != VK_SUCCESS || formatCount == 0) {
        return 0;
    }

    // Select a format
    u32 fmtCount = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
        displayManager->physicalDevice,
        window->surface,
        &fmtCount,
        NULL) != VK_SUCCESS || fmtCount == 0) {
        return 0;
    }
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * fmtCount);
    if (!formats) return 0;
    VkResult formatResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
            displayManager->physicalDevice,
            window->surface,
            &fmtCount,
            formats);
    if (formatResult != VK_SUCCESS && formatResult != VK_INCOMPLETE) {
        free(formats);
        return 0;
    }
    VkSurfaceFormatKHR chosen = formats[0];
    for (u32 i=0; i<fmtCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = formats[i];
            break;
        }
    }
    free(formats);

    // The existing shared render pass uses the old format.
    if (window->swapchainFormat != VK_FORMAT_UNDEFINED &&
        chosen.format != window->swapchainFormat) {
        ErrorSystem_AddError(&GUI.errorSystem,
            "<Vulkan Error> Swapchain format changed; "
            "render pass and graphics pipelines need recreation");
        return 0;
    }

    // Commit the format before building anything that depends on it.
    window->swapchainFormat = chosen.format;

    // Lazily create the shared render pass with the chosen swapchain format.
    if (displayManager->sharedRenderPass == VK_NULL_HANDLE) {
        VkAttachmentDescription colorAttachment = {0};
        colorAttachment.format = chosen.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {0};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {0};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency = {0};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo rpci = {0};
        rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpci.attachmentCount = 1;
        rpci.pAttachments = &colorAttachment;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;
        rpci.dependencyCount = 1;
        rpci.pDependencies = &dependency;
        if (vkCreateRenderPass(device, &rpci, NULL, &displayManager->sharedRenderPass) != VK_SUCCESS) {
            ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateRenderPass failed");
            return 0;
        }
    }

    // Choose the extent
    VkExtent2D extent;
    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        extent = caps.currentExtent;
    } else {
        extent.width = (u32)width < caps.minImageExtent.width? caps.minImageExtent.width : ((u32)width > caps.maxImageExtent.width ? caps.maxImageExtent.width : (u32)width);
        extent.height = (u32)height < caps.minImageExtent.height ? caps.minImageExtent.height : ((u32)height > caps.maxImageExtent.height ? caps.maxImageExtent.height : (u32)height);
    }
    if (extent.width == 0 || extent.height == 0) {
        return 0;
    }
    window->swapchainExtent = extent;

    // Choose the minimum number of swapchain images to request
    u32 requestedImageCount = 2;
    if (requestedImageCount < caps.minImageCount) requestedImageCount = caps.minImageCount;
    if (caps.maxImageCount != 0 && requestedImageCount > caps.maxImageCount) {
        requestedImageCount = caps.maxImageCount;
    }

    // Create the swapchain
    VkSwapchainCreateInfoKHR sci = {0};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = window->surface;
    sci.minImageCount = requestedImageCount;
    sci.imageFormat = chosen.format;
    sci.imageColorSpace = chosen.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &sci, NULL, &swapchain) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateSwapchainKHR failed");
        return 0;
    }
    window->swapchain = swapchain;

    // Get the image count
    u32 actualImageCount = 0;
    VkResult result = vkGetSwapchainImagesKHR(device, window->swapchain, &actualImageCount, NULL);
    if (result != VK_SUCCESS || actualImageCount == 0 || actualImageCount > NU_MAX_SWAPCHAIN_IMAGES) {
        return 0;
    }
    result = vkGetSwapchainImagesKHR(device, window->swapchain, &actualImageCount, window->images);
    if (result != VK_SUCCESS) {
        return 0;
    }
    window->imageCount = actualImageCount;

    // Create image views
    for (u32 i=0; i<window->imageCount; i++) {
        VkImageViewCreateInfo vci = {0};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = window->images[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = chosen.format;
        vci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel = 0;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &vci, NULL, &window->imageViews[i]) != VK_SUCCESS) {
            return 0;
        }

        // Create frame buffers
        VkFramebufferCreateInfo fci = {0};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = displayManager->sharedRenderPass;
        fci.attachmentCount = 1;
        fci.pAttachments = &window->imageViews[i];
        fci.width = extent.width;
        fci.height = extent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(device, &fci, NULL, &window->framebuffers[i]) != VK_SUCCESS) {
            return 0;
        }
    }

    // Create per-swapchain-image presentation semaphores
    VkSemaphoreCreateInfo semci = {0};
    semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (u32 i=0; i<window->imageCount; i++) {
        if (vkCreateSemaphore(device, &semci, NULL, &window->renderFinished[i]) != VK_SUCCESS) {
            return 0;
        }
    }

    // Reset frame tracking
    window->currentFrame = 0;

    // Success
    return 1;
}

int Window_CreateVulkanDevices(Window* window, DisplayManager* displayManager)
{
    // Init Vulkan member variables
    window->surface = VK_NULL_HANDLE;
    window->swapchain = VK_NULL_HANDLE;
    window->swapchainFormat = VK_FORMAT_UNDEFINED;
    window->swapchainExtent = (VkExtent2D){0};
    window->renderPass = VK_NULL_HANDLE;
    window->commandPool = VK_NULL_HANDLE;
    for (u32 i=0; i<NU_MAX_SWAPCHAIN_IMAGES; i++) {
        window->images[i] = VK_NULL_HANDLE;
        window->imageViews[i] = VK_NULL_HANDLE;
        window->framebuffers[i] = VK_NULL_HANDLE;
        window->renderFinished[i] = VK_NULL_HANDLE;
    }
    for (u32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        window->imageAvailable[i] = VK_NULL_HANDLE;
        window->inFlightFences[i] = VK_NULL_HANDLE;
    }
    for (u32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        window->commandBuffers[i] = VK_NULL_HANDLE;
    }
    window->currentFrame = 0;
    window->imageCount = 0;
    window->imageIndex = 0;


    // Cache device
    VkDevice device = displayManager->device;

    // Create render surface
    if (!SDL_Vulkan_CreateSurface(window->sdlWin, displayManager->instance, NULL, &window->surface)) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> SDL_Vulkan_CreateSurface failed");
        return 0;
    }

    // Ensure graphics queue can present
    VkBool32 presentSupported = VK_FALSE;

    if (vkGetPhysicalDeviceSurfaceSupportKHR(
        displayManager->physicalDevice,
        displayManager->graphicsFamily,
        window->surface,
        &presentSupported) != VK_SUCCESS ||
        !presentSupported)
    {
        ErrorSystem_AddError(&GUI.errorSystem,"<Vulkan Error> graphics queue cannot present");
        return 0;
    }

    // Create swapchain
    if (!Window_CreateSwapchain(window, displayManager)) {
        return 0;
    }

    // Create frame synchronization objects
    VkSemaphoreCreateInfo semci = {0};
    semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fci = {0};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (u32 i = 0; i < NU_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semci, NULL, &window->imageAvailable[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fci, NULL, &window->inFlightFences[i]) != VK_SUCCESS)
        {
            ErrorSystem_AddError(&GUI.errorSystem,"<Vulkan Error> Failed to create frame sync objects");
            return 0;
        }
    }

    // Create command pool
    VkCommandPoolCreateInfo pci = {0};
    pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = displayManager->graphicsFamily;
    if (vkCreateCommandPool(device, &pci, NULL, &window->commandPool) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateCommandPool failed");
        return 0;
    }

    // Allocate command buffers
    VkCommandBufferAllocateInfo aci = {0};
    aci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aci.commandPool = window->commandPool;
    aci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aci.commandBufferCount = NU_FRAMES_IN_FLIGHT;
    if (vkAllocateCommandBuffers(device, &aci, window->commandBuffers) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkAllocateCommandBuffers failed");
        return 0;
    }

    // Set current frame
    window->currentFrame = 0;

    // Success
    return 1;
}

void Window_DestroyVulkanDevices(Window* window, DisplayManager* displayManager)
{
    VkDevice device = displayManager->device;
    if (device == VK_NULL_HANDLE) return;

    // 1. Command pool (frees the command buffers allocated from it)
    if (window->commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, window->commandPool, NULL);
        window->commandPool = VK_NULL_HANDLE;
    }

    // 2. Sync objects
    for (u32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        if (window->inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, window->inFlightFences[i], NULL);
            window->inFlightFences[i] = VK_NULL_HANDLE;
        }
        if (window->imageAvailable[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, window->imageAvailable[i], NULL);
            window->imageAvailable[i] = VK_NULL_HANDLE;
        }
    }
    for (u32 i=0; i<window->imageCount; i++) {
        if (window->renderFinished[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, window->renderFinished[i], NULL);
            window->renderFinished[i] = VK_NULL_HANDLE;
        }
    }

    // 3. Framebuffers
    for (u32 i=0; i<window->imageCount; i++) {
        if (window->framebuffers[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, window->framebuffers[i], NULL);
            window->framebuffers[i] = VK_NULL_HANDLE;
        }
    }

    // 4. Image views
    for (u32 i=0; i<window->imageCount; i++) {
        if (window->imageViews[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(device, window->imageViews[i], NULL);
            window->imageViews[i] = VK_NULL_HANDLE;
        }
    }

    // 5. Swapchain
    if (window->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, window->swapchain, NULL);
        window->swapchain = VK_NULL_HANDLE;
    }

    // 6. Surface — takes INSTANCE, not device
    if (window->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(displayManager->instance, window->surface, NULL);
        window->surface = VK_NULL_HANDLE;
    }

    // Reset counters so accidental reuse doesn't hit stale handles
    window->imageCount = 0;
    window->currentFrame = 0;
}

int Window_PrepareFrame(Window* window, DisplayManager* displayManager)
{
    VkDevice device = displayManager->device;

    // Wait until this frame slot is available.
    if (vkWaitForFences(
        device, 1,
        &window->inFlightFences[window->currentFrame],
        VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        return 0;
    }


    VkResult result = vkAcquireNextImageKHR(
        device,
        window->swapchain,
        UINT64_MAX,
        window->imageAvailable[window->currentFrame],
        VK_NULL_HANDLE,
        &window->imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        Window_CreateSwapchain(window, displayManager);
        return 0;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return 0;
    }

    // Begin recording the command buffer for this frame
    VkCommandBuffer cmd = window->commandBuffers[window->currentFrame];
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) return 0;

    // Begin render pass
    VkClearValue clearValue = {0};
    clearValue.color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 1.0f}};
    VkRenderPassBeginInfo renderInfo = {0};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderInfo.renderPass = displayManager->sharedRenderPass;
    renderInfo.framebuffer = window->framebuffers[window->imageIndex];
    renderInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderInfo.renderArea.extent = window->swapchainExtent;
    renderInfo.clearValueCount = 1;
    renderInfo.pClearValues = &clearValue;
    vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Success
    return 1;
}

int Window_Present(Window* window, DisplayManager* displayManager)
{
    // Finish recording commands
    VkCommandBuffer cmd = window->commandBuffers[window->currentFrame];
    vkCmdEndRenderPass(cmd);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) return 0;

    // Wait for the image-acquired semaphore
    VkSemaphore waitSemaphore = window->imageAvailable[window->currentFrame];

    // Signal when rendering to this swapchain image is finished.
    VkSemaphore signalSemaphore = window->renderFinished[window->imageIndex];
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // Reset fence
    if (vkResetFences(displayManager->device, 1, &window->inFlightFences[window->currentFrame]) != VK_SUCCESS) {
        return 0;
    }

    // Submit draw commands
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &waitSemaphore;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &signalSemaphore;
    if (vkQueueSubmit( displayManager->graphicsQueue, 1, &submitInfo, window->inFlightFences[window->currentFrame]) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkQueueSubmit failed");
        return 0;
    }

    // Present frame
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &window->renderFinished[window->imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &window->swapchain;
    presentInfo.pImageIndices = &window->imageIndex;
    VkResult result = vkQueuePresentKHR(displayManager->graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        Window_CreateSwapchain(window, displayManager);
        return 0;
    }
    if (result != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> Failed to swap window buffers");
        return 0;
    }

    window->currentFrame = (window->currentFrame + 1) % NU_FRAMES_IN_FLIGHT;

    // Success
    return 1;
}
