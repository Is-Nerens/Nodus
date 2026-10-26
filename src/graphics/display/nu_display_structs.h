#pragma once

#include <graphics/display/nu_gpu_arena_alloc_structs.h>
#define RECYCLED_WINDOWS_CAPACITY 8

typedef struct DisplayManager
{
    // Vulkan stuff
    VkInstance       instance;
    VkPhysicalDevice physicalDevice;
    VkDevice         device;
    VkQueue          graphicsQueue;
    u32              graphicsFamily;
    VkRenderPass     sharedRenderPass;

    // GPU arena allocator
    ArenaAllocatorGPU gpuArenaAlloc;

    // Render pipelines
    NuPipeline borderRectPipeline;
    NuPipeline imagePipeline;
    NuPipeline clippedImagePipeline;
    NuPipeline monoTextPipeline;
    NuPipeline clippedMonoTextPipeline;
    NuPipeline subpixelTextPipeline;
    NuPipeline clippedSubpixelTextPipeline;
    NuPipeline vertexRGBPipeline;


    // Nodus stuff
    Container windows;
    SDL_Window* recycledSDLWindows[RECYCLED_WINDOWS_CAPACITY];
    int recycledSDLWindowCount;
    Array absoluteRootNodes;
    Hashmap clipMap;
    Node* hoveredWindowNode;
    SDL_Window* lastClickedWindow;
    int rootWindowID;
} DisplayManager;

typedef struct NuWindowDrawlist
{
    Array drawNodes;
    Array clippedDrawNodes;
    Array canvasNodes;
} NuWindowDrawlist;

typedef struct NuWindow
{
    SDL_Window* sdlWin;

    // Swapchain
    VkSurfaceKHR   surface;
    VkSwapchainKHR swapchain;
    VkFormat       swapchainFormat;
    VkExtent2D     swapchainExtent;
    u32            imageCount;
    VkImage        images[4];
    VkImageView    imageViews[4];
    VkFramebuffer  framebuffers[4];
    VkRenderPass   renderPass;

    // Sync
    VkSemaphore imageAvailable[NU_FRAMES_IN_FLIGHT];
    VkFence     inFlightFences[NU_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinished[4];

    // Command buffers
    VkCommandPool   commandPool;
    VkCommandBuffer commandBuffers[NU_FRAMES_IN_FLIGHT];
    u32             currentFrame;

    // Draw info
    NuWindowDrawlist drawlist;
    Node* windowNode;
    int screenWidth;
    int screenHeight;
    float renderScale;
} NuWindow;
