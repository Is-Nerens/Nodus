#pragma once


#include "nu_gpu_arena_alloc.h"
#include "nu_surface.h"
#include "nu_swapchain.h"
#define NU_RECYCLED_WINDOWS_CAPACITY 8

typedef struct NuWindowDrawlist
{
    Array drawNodes;
    Array clippedDrawNodes;
    Array canvasNodes;
} NuWindowDrawlist;

typedef struct {

    SDL_Window* sdlWin;

    Surface surface;
    Swapchain swapchain;

    VkCommandPool   commandPool;
    VkCommandBuffer commandBuffers[NU_FRAMES_IN_FLIGHT];
    uint64 nextPresentTime;
    uint64 refreshDuration;

    // Draw info
    NuWindowDrawlist drawlist;
    Node* windowNode;
    int displayID;
    int screenWidth;
    int screenHeight;
    float renderScale;
    bool repaint;
    bool waitingToPresent;
} Window;

#include "nu_windows.h"

struct DisplayManagerPipelines
{
    NuPipeline borderRectPipeline;
    NuPipeline imagePipeline;
    NuPipeline monoTextPipeline;
    NuPipeline clippedMonoTextPipeline;
    NuPipeline subpixelTextPipeline;
    NuPipeline clippedSubpixelTextPipeline;
    NuPipeline vertexRGBPipeline;
    NuPipeline borderRectPipelineMSAA4X;
    NuPipeline imagePipelineMSAA4X;
    NuPipeline monoTextPipelineMSAA4X;
    NuPipeline clippedMonoTextPipelineMSAA4X;
    NuPipeline subpixelTextPipelineMSAA4X;
    NuPipeline clippedSubpixelTextPipelineMSAA4X;
    NuPipeline vertexRGBPipelineMSAA4X;
};

typedef struct DisplayManager
{
    // Vulkan stuff
    VkInstance            instance;
    VkPhysicalDevice      physicalDevice;
    VkDevice              device;
    VkQueue               graphicsQueue;
    VkCommandPool         commandPool;
    uint32                graphicsFamily;
    VkRenderPass          sharedRenderPass;
    VkRenderPass          sharedRenderPassMSAA4X;
    VkDescriptorSetLayout fontSetLayout;   // binding 0, COMBINED_IMAGE_SAMPLER, count 1, fragment
    VkDescriptorPool      fontPool;
    PFN_vkWaitForPresentKHR vkWaitForPresentKHR;
    VkBool32 presentWaitSupported;

    struct DisplayManagerPipelines pipelines;

    // GPU arena allocator
    ArenaAllocatorGPU gpuArenaAlloc;

    // Nodus stuff
    Windows windows;
    SDL_Window* recycledSDLWindows[NU_RECYCLED_WINDOWS_CAPACITY];
    int recycledSDLWindowCount;
    Array absoluteRootNodes;
    Hashmap clipMap;
    Node* hoveredWindowNode;
    SDL_Window* lastClickedWindow;
    int8 rootWindowID;
    bool uiChanged;
    uint8 numWindowsWaitingToPresent;
} DisplayManager;
