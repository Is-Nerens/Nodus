#pragma once

#define NU_FRAMES_IN_FLIGHT 2

int NuWindow_CreateVulkanDevices(NuWindow* window, DisplayManager* displayManager)
{
    // Create render surface
    if (!SDL_Vulkan_CreateSurface(window->sdlWin, displayManager->instance, NULL, &window->surface)) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> SDL_Vulkan_CreateSurface failed");
        return 0;
    }

    // Ensure graphics queue can present
    VkBool32 presentSupported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(displayManager->physicalDevice, displayManager->graphicsFamily, window->surface, &presentSupported);
    if (!presentSupported) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> graphics queue cannot present");
        return 0;
    }

    // Query surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(displayManager->physicalDevice, window->surface, &caps);
    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(displayManager->physicalDevice, window->surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[32];
    if (formatCount > 32) formatCount = 32;
    vkGetPhysicalDeviceSurfaceFormatsKHR(displayManager->physicalDevice, window->surface, &formatCount, formats);
    u32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(displayManager->physicalDevice, window->surface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[8];
    if (presentModeCount > 8) presentModeCount = 8;
    vkGetPhysicalDeviceSurfacePresentModesKHR(displayManager->physicalDevice, window->surface, &presentModeCount, presentModes);

    // Choose surface format
    VkSurfaceFormatKHR chosen = formats[0];
    for (u32 i=0; i<formatCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = formats[i];
            break;
        }
    }
    window->swapchainFormat = chosen.format;

    // Choose present mode
    VkExtent2D extent;
    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        extent = caps.currentExtent;
    } else {
        int w, h;
        SDL_GetWindowSizeInPixels(window->sdlWin, &w, &h);
        extent.width  = w < caps.minImageExtent.width  ? caps.minImageExtent.width  : (w > caps.maxImageExtent.width  ? caps.maxImageExtent.width  : w);
        extent.height = h < caps.minImageExtent.height ? caps.minImageExtent.height : (h > caps.maxImageExtent.height ? caps.maxImageExtent.height : h);
    }
    window->swapchainExtent = extent;

    // Get image count
    u32 imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    // Create a swapchain
    VkSwapchainCreateInfoKHR sci = {0};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = window->surface;
    sci.minImageCount = imageCount;
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
    if (vkCreateSwapchainKHR(displayManager->device, &sci, NULL, &window->swapchain) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateSwapchainKHR failed");
        return 0;
    }

    // Get images
    vkGetSwapchainImagesKHR(displayManager->device, window->swapchain, &window->imageCount, NULL);
    if (window->imageCount > 4) window->imageCount = 4;
    vkGetSwapchainImagesKHR(displayManager->device, window->swapchain, &window->imageCount, window->images);

    // Create image views
    for (u32 i=0; i<window->imageCount; i++) {
        VkImageViewCreateInfo vci = {0};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = window->images[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = window->swapchainFormat;
        vci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel = 0;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount = 1;
        if (vkCreateImageView(displayManager->device, &vci, NULL, &window->imageViews[i]) != VK_SUCCESS) {
            ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateImageView failed");
            return 0;
        }
    }

    // Create the shared render pass only when the first window is made
    if (displayManager->sharedRenderPass == VK_NULL_HANDLE) {

        // Colour attachment
        VkAttachmentDescription color = {0};
        color.format = window->swapchainFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Attachment reference
        VkAttachmentReference colorRef = {0};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Subpass
        VkSubpassDescription subpass = {0};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        // Subpass dependency
        VkSubpassDependency dep = {0};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask = 0;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Create render pass
        VkRenderPassCreateInfo rci = {0};
        rci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rci.attachmentCount = 1;
        rci.pAttachments = &color;
        rci.subpassCount = 1;
        rci.pSubpasses = &subpass;
        rci.dependencyCount = 1;
        rci.pDependencies = &dep;
        if (vkCreateRenderPass(displayManager->device, &rci, NULL, &displayManager->sharedRenderPass) != VK_SUCCESS) {
            ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateRenderPass failed");
            return 0;
        }
    }

    // Create frame buffers
    for (u32 i=0; i< window->imageCount; i++) {
        VkFramebufferCreateInfo fci = {0};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = displayManager->sharedRenderPass;
        fci.attachmentCount = 1;
        fci.pAttachments = &window->imageViews[i];
        fci.width = window->swapchainExtent.width;
        fci.height = window->swapchainExtent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(displayManager->device, &fci, NULL, &window->framebuffers[i]) != VK_SUCCESS) {
            ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateFramebuffer failed");
            return 0;
        }
    }

    // Create sync objects
    VkSemaphoreCreateInfo semci = {0};
    semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fci = {0};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (u32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(displayManager->device, &semci, NULL, &window->imageAvailable[i]);
        vkCreateFence(displayManager->device, &fci, NULL, &window->inFlightFences[i]);
    }
    for (u32 i=0; i<window->imageCount; i++) {
        vkCreateSemaphore(displayManager->device, &semci, NULL, &window->renderFinished[i]);
    }

    // Create command buffers
    VkCommandPoolCreateInfo pci = {0};
    pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = displayManager->graphicsFamily;
    vkCreateCommandPool(displayManager->device, &pci, NULL, &window->commandPool);
    VkCommandBufferAllocateInfo aci = {0};
    aci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aci.commandPool = window->commandPool;
    aci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aci.commandBufferCount = NU_FRAMES_IN_FLIGHT;
    vkAllocateCommandBuffers(displayManager->device, &aci, window->commandBuffers);
}

void NuWindow_DestroyVulkanDevices(NuWindow* window, DisplayManager* displayManager)
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
