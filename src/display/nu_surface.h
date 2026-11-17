#pragma once

typedef struct {
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
} Surface;

int Surface_Create(
    Surface* surface,
    SDL_Window* sdlWindow,
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    uint32 graphicsFamily)
{
    if (!SDL_Vulkan_CreateSurface(sdlWindow, instance, NULL, &surface->surface)) {
        return 0;
    }

    // Ensure graphics queue can present
    VkBool32 presentSupported = VK_FALSE;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice,
        graphicsFamily,
        surface->surface,
        &presentSupported) != VK_SUCCESS ||
        !presentSupported)
    {
        return 0;
    }

    // Query surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface->surface,
        &caps) != VK_SUCCESS) {
        return 0;
    }

    // Query supported surface formats
    uint32 formatCount = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface->surface,
        &formatCount,
        NULL) != VK_SUCCESS || formatCount == 0) {
        return 0;
    }

    // Select a format
    uint32 fmtCount = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface->surface,
        &fmtCount,
        NULL) != VK_SUCCESS || fmtCount == 0) {
        return 0;
    }
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * fmtCount);
    if (!formats) return 0;
    VkResult formatResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface->surface,
            &fmtCount,
            formats);
    if (formatResult != VK_SUCCESS && formatResult != VK_INCOMPLETE) {
        free(formats);
        return 0;
    }
    VkSurfaceFormatKHR chosenSurfaceFormat = formats[0];
    for (uint32 i=0; i<fmtCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenSurfaceFormat = formats[i];
            break;
        }
    }
    free(formats);

    // Commit format
    surface->surfaceFormat = chosenSurfaceFormat;

    // Success
    return 1;
}
