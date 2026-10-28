#pragma once


#include "nu_gpu_arena_alloc.h"
#define NU_RECYCLED_WINDOWS_CAPACITY 8
#define NU_FRAMES_IN_FLIGHT 2
#define NU_MAX_SWAPCHAIN_IMAGES 4

typedef struct DisplayManager
{
    // Vulkan stuff
    VkInstance       instance;
    VkPhysicalDevice physicalDevice;
    VkDevice         device;
    VkQueue          graphicsQueue;
    VkCommandPool    commandPool;
    u32              graphicsFamily;
    VkRenderPass     sharedRenderPass;

    // GPU arena allocator
    ArenaAllocatorGPU gpuArenaAlloc;

    // Render pipelines
    NuPipeline borderRectPipeline;
    NuPipeline imagePipeline;
    NuPipeline monoTextPipeline;
    NuPipeline clippedMonoTextPipeline;
    NuPipeline subpixelTextPipeline;
    NuPipeline clippedSubpixelTextPipeline;
    NuPipeline vertexRGBPipeline;


    // Nodus stuff
    Container windows;
    SDL_Window* recycledSDLWindows[NU_RECYCLED_WINDOWS_CAPACITY];
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

typedef struct {

    SDL_Window* sdlWin;

    // Swapchain
    VkSurfaceKHR   surface;
    VkSwapchainKHR swapchain;
    VkFormat       swapchainFormat;
    VkExtent2D     swapchainExtent;
    u32            imageCount;
    VkImage        images[NU_MAX_SWAPCHAIN_IMAGES];
    VkImageView    imageViews[NU_MAX_SWAPCHAIN_IMAGES];
    VkFramebuffer  framebuffers[NU_MAX_SWAPCHAIN_IMAGES];
    VkRenderPass   renderPass;

    // Sync
    VkSemaphore imageAvailable[NU_FRAMES_IN_FLIGHT];
    VkFence     inFlightFences[NU_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinished[NU_FRAMES_IN_FLIGHT];

    // Command buffers
    VkCommandPool   commandPool;
    VkCommandBuffer commandBuffers[NU_FRAMES_IN_FLIGHT];
    u32 currentFrame;
    u32 imageIndex;

    // Draw info
    NuWindowDrawlist drawlist;
    Node* windowNode;
    int screenWidth;
    int screenHeight;
    float renderScale;
} Window;
