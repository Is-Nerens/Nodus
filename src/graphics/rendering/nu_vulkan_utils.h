#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

u32 FindMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (u32 i=0; i<memProperties.memoryTypeCount; i++)
    {
        // Is this memory type allowed by the requirements?
        if ((typeFilter & (1u << i)) == 0) continue;

        // Does it support every requested property?
        if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // No suitable memory type was found.
    return UINT32_MAX;
}
