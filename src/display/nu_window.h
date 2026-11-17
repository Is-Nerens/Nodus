#pragma once

#include "nu_swapchain.h"
#include "vulkan/vulkan_core.h"

int Window_CreateVulkanDevices(Window* window, DisplayManager* displayManager)
{
    // Init Vulkan member variables
    window->commandPool = VK_NULL_HANDLE;
    for (uint32 i=0; i<NU_FRAMES_IN_FLIGHT; i++) {
        window->commandBuffers[i] = VK_NULL_HANDLE;
    }

    // Create surface from SDL window
    if (!Surface_Create(
        &window->surface,
        window->sdlWin,
        displayManager->instance,
        displayManager->physicalDevice,
        displayManager->graphicsFamily))
    {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> failed to create surface");
        return 0;
    }

    // Lazily create shared render passes
    if (displayManager->sharedRenderPass == VK_NULL_HANDLE) {
        {
            VkAttachmentDescription colorAttachment = {0};
            colorAttachment.format = window->surface.surfaceFormat.format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentDescription depthAttachment = {0};
            depthAttachment.format = VK_FORMAT_D32_SFLOAT;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
            VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            VkAttachmentReference depthRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

            VkSubpassDescription subpass = {0};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorRef;
            subpass.pDepthStencilAttachment = &depthRef;

            VkSubpassDependency dependency = {0};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo rpci = {0};
            rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            rpci.attachmentCount = 2;
            rpci.pAttachments = attachments;
            rpci.subpassCount = 1;
            rpci.pSubpasses = &subpass;
            rpci.dependencyCount = 1;
            rpci.pDependencies = &dependency;

            if (vkCreateRenderPass(displayManager->device, &rpci, NULL, &displayManager->sharedRenderPass) != VK_SUCCESS) {
                ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateRenderPass (shared) failed");
                return 0;
            }
        }

        // ---- sharedRenderPassMSAA4X: MSAA colour + MSAA depth + 1x resolve ----
        // This is the pass the canvas framebuffer binds. It resolves the 4x canvas
        // into a 1x sampleable image. There is no separate 2-attachment 4x pass.
        {
            VkAttachmentDescription attachments[3] = {0};

            // 0: MSAA colour (rendered into, then resolved away)
            attachments[0].format         = VK_FORMAT_B8G8R8A8_UNORM;
            attachments[0].samples        = VK_SAMPLE_COUNT_4_BIT;
            attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // 1: MSAA depth
            attachments[1].format         = VK_FORMAT_D32_SFLOAT;
            attachments[1].samples        = VK_SAMPLE_COUNT_4_BIT;
            attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // 2: resolved 1x colour -> sampled by UI
            attachments[2].format         = VK_FORMAT_B8G8R8A8_UNORM;
            attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[2].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentReference colorRef   = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            VkAttachmentReference resolveRef = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            VkAttachmentReference depthRef   = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

            VkSubpassDescription subpass = {0};
            subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount    = 1;
            subpass.pColorAttachments       = &colorRef;
            subpass.pResolveAttachments     = &resolveRef;
            subpass.pDepthStencilAttachment = &depthRef;

            VkSubpassDependency dependency = {0};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo rpci = {0};
            rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            rpci.attachmentCount = 3;
            rpci.pAttachments = attachments;
            rpci.subpassCount = 1;
            rpci.pSubpasses = &subpass;
            rpci.dependencyCount = 1;
            rpci.pDependencies = &dependency;

            if (vkCreateRenderPass(displayManager->device, &rpci, NULL, &displayManager->sharedRenderPassMSAA4X) != VK_SUCCESS) {
                ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateRenderPass (sharedRenderPassMSAA4X) failed");
                return 0;
            }
        }
    }

    // Get window width and height
    int width, height; SDL_GetWindowSizeInPixels(window->sdlWin, &width, &height);
    width = max(width, 1); height = max(height, 1);

    // Create swapchain
    if (!Swapchain_Create(&window->swapchain, displayManager->device)) {
        ErrorSystem_AddError(&GUI.errorSystem,"<Vulkan Error> Failed to create swapchain");
        return 0;
    }

    // Create swapchain images
    if (!Swapchain_CreateImages(
        &window->swapchain,
        displayManager->device,
        displayManager->physicalDevice,
        displayManager->sharedRenderPass,
        window->surface,
        width,
        height))
    {
        ErrorSystem_AddError(&GUI.errorSystem,"<Vulkan Error> Failed to create swapchain images");
        return 0;
    }

    // Create command pool
    VkCommandPoolCreateInfo pci = {0};
    pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = displayManager->graphicsFamily;
    if (vkCreateCommandPool(displayManager->device, &pci, NULL, &window->commandPool) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkCreateCommandPool failed");
        return 0;
    }

    // Allocate command buffers
    VkCommandBufferAllocateInfo aci = {0};
    aci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aci.commandPool = window->commandPool;
    aci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aci.commandBufferCount = NU_FRAMES_IN_FLIGHT;
    if (vkAllocateCommandBuffers(displayManager->device, &aci, window->commandBuffers) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vkAllocateCommandBuffers failed");
        return 0;
    }

    // Success
    return 1;
}

void Window_DestroyVulkanDevices(Window* window, DisplayManager* displayManager)
{
    // Destroy command pool (destroys command buffers)
    if (window->commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(displayManager->device, window->commandPool, NULL);

    // Destroy swapchain
    Swapchain_Destroy(&window->swapchain, displayManager->device);

    // Destroy surface
    if (window->surface.surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(displayManager->instance, window->surface.surface, NULL);
}

void Window_UpdateRenderScaleAndDisplaySize(Window* window)
{
    // Update display size and render scale
    int winPtsW, winPtsH;
    int winPxW, winPxH;
    SDL_GetWindowSize(window->sdlWin, &winPtsW, &winPtsH);
    SDL_GetWindowSizeInPixels(window->sdlWin, &winPxW, &winPxH);
    window->renderScale = (float)winPxW / (float)winPtsW;
    window->screenWidth = winPxW;
    window->screenHeight = winPxH;

    // Update refresh duration
    SDL_DisplayID displayID = SDL_GetDisplayForWindow(window->sdlWin);
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    if (mode && mode->refresh_rate_numerator > 0 && mode->refresh_rate_denominator > 0)
    {
        window->refreshDuration = 1000000000ULL * (uint64)mode->refresh_rate_denominator / (uint64)mode->refresh_rate_numerator;
        window->nextPresentTime = 0;
    }
}

void Window_DisplayChanged(Window* window, DisplayManager* displayManager)
{
    // Update display size and render scale
    Window_UpdateRenderScaleAndDisplaySize(window);
}

int Window_StartFrame(Window* window, DisplayManager* displayManager)
{
    VkDevice device = displayManager->device;

    // If the window was resized OR the swapchain was suboptimal last frame -> recreate
    int width, height;
    SDL_GetWindowSizeInPixels(window->sdlWin, &width, &height); width = max(width, 1); height = max(height, 1);
    if (width != Swapchain_Width(&window->swapchain) || height != Swapchain_Height(&window->swapchain) || window->swapchain.suboptimal)
    {
        if (!Swapchain_CreateImages(
            &window->swapchain,
            displayManager->device,
            displayManager->physicalDevice,
            displayManager->sharedRenderPass,
            window->surface,
            width, height))
        {
            return 0;
        }
    }

    // Acquire next image
    int acquireResult = Swapchain_AquireNextImage(&window->swapchain, displayManager->device);
    if (acquireResult == 2) {
        if (!Swapchain_CreateImages(
            &window->swapchain,
            displayManager->device,
            displayManager->physicalDevice,
            displayManager->sharedRenderPass,
            window->surface,
            width, height))
        {
            return 0;
        }
    }
    if (acquireResult == 2 || acquireResult == 0) return 0;

    // Start recording commands for this frame
    VkCommandBuffer cmd = window->commandBuffers[Swapchain_CurrentFrame(&window->swapchain)];
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) return 0;

    // Begin render pass
    VkClearValue clearValues[2] = {0};
    clearValues[0].color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = (VkClearDepthStencilValue){0.0f, 0};
    VkRenderPassBeginInfo renderInfo = {0};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderInfo.renderPass = displayManager->sharedRenderPass;
    renderInfo.framebuffer = Swapchain_CurrentFramebuffer(&window->swapchain);
    renderInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderInfo.renderArea.extent = window->swapchain.swapchainExtent;
    renderInfo.clearValueCount = 2;
    renderInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Success
    return 1;

}

inline VkCommandBuffer Window_CurrentCommandBuffer(Window* window)
{
    return window->commandBuffers[Swapchain_CurrentFrame(&window->swapchain)];
}

int Window_EndFrame(Window* window, DisplayManager* displayManager)
{
    // Finish recording commands
    VkCommandBuffer cmd = window->commandBuffers[Swapchain_CurrentFrame(&window->swapchain)];
    vkCmdEndRenderPass(cmd);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) return 0;

    // Prepare swapchain
    Swapchain_PrepareSubmission(&window->swapchain, displayManager->device);

    // Submit draw commands
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    VkSemaphore waitSemaphores[1];
    waitSemaphores[0] = Swapchain_CurrentImageSemaphore(&window->swapchain);
    submitInfo.pWaitSemaphores = &waitSemaphores[0];
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    VkSemaphore signalSemaphores[1];
    signalSemaphores[0] = Swapchain_CurrentRenderSemaphore(&window->swapchain);
    submitInfo.pSignalSemaphores = &signalSemaphores[0];
    if (vkQueueSubmit(displayManager->graphicsQueue, 1, &submitInfo, window->swapchain.inFlightFences[Swapchain_CurrentFrame(&window->swapchain)]) != VK_SUCCESS) {
        return 0; // Catastrophic -> quit
    }

    // Success
    return 1;
}

void Window_Present(Window* window, DisplayManager* displayManager)
{
    // Present frame
    VkPresentIdKHR presentIdInfo = {0};
    presentIdInfo.sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
    presentIdInfo.swapchainCount = 1;
    presentIdInfo.pPresentIds = &window->swapchain.presentId;
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    VkSemaphore waitSemaphores[1];
    waitSemaphores[0] = Swapchain_CurrentRenderSemaphore(&window->swapchain);
    presentInfo.pWaitSemaphores = &waitSemaphores[0];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &window->swapchain.swapchain;
    uint32 imageIndices[1];
    imageIndices[0] = Swapchain_CurrentImageIndex(&window->swapchain);
    presentInfo.pImageIndices = &imageIndices[0];
    presentInfo.pNext = displayManager->presentWaitSupported ? &presentIdInfo : NULL;
    VkResult result = vkQueuePresentKHR(displayManager->graphicsQueue, &presentInfo);

    // Advance frame
    Swapchain_AdvanceFrame(&window->swapchain);

    // Advance present ID
    window->swapchain.presentId++;

    // Window updated
    window->needsRepaint = true;
}

void Window_WaitForPresentation(Window* window, DisplayManager* displayManager, uint64 expirationTimeNS)
{
    displayManager->vkWaitForPresentKHR(
        displayManager->device,
        window->swapchain.swapchain,
        window->swapchain.presentId,
        expirationTimeNS
    );
}
