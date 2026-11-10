#pragma once

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkImageLayout currentLayout;
    uint16 width;
    uint16 height;
} CanvasRenderTarget;

CanvasRenderTarget_Create(DisplayManager* displayManager, CanvasRenderTarget* renderTarget)
{

}
