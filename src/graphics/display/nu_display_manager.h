#pragma once

#include <graphics/display/nu_display_structs.h>
#include <graphics/display/nu_gpu_arena_alloc.h>
#include <graphics/display/nu_window.h>

int DisplayManager_CreateVulkanDevices(DisplayManager* displayManager)
{
    // Application info
    VkApplicationInfo app = {0};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Nodus";
    app.apiVersion = VK_API_VERSION_1_2;

    // Create instance info
    VkInstanceCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;

    // Create VK Instance
    VkInstance instance = VK_NULL_HANDLE;
    VkResult r = vkCreateInstance(&ci, NULL, &instance);
    if (r != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> Failed to create a Vulkan instance");
        return 0;
    }

    // Get device count
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    VkPhysicalDevice devices[8];
    if (deviceCount > 8) deviceCount = 8;
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    for (uint32_t i=0; i<deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
    }

    // Choose a physical device
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    u32 graphicsFamily = 0;
    for (u32 i=0; i<deviceCount; i++) {
        u32 familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &familyCount, NULL);
        VkQueueFamilyProperties families[16];
        if (familyCount > 16) familyCount = 16;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &familyCount, families);
        for (u32 f=0; f<familyCount; f++) {
            if (families[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                physicalDevice = devices[i];
                graphicsFamily = f;
                break;
            }
        }
        if (physicalDevice != VK_NULL_HANDLE) break;
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> Could not find a physical device");
        return 0;
    }

    // Create a logical device
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {0};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = graphicsFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* deviceExts[2];
    u32 deviceExtCount = 0;
    deviceExts[deviceExtCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    #ifdef PLATFORM_MACOS
    deviceExts[deviceExtCount++] = "VK_KHR_portability_subset";
    #endif

    VkPhysicalDeviceFeatures features = {0};

    // Device create info
    VkDeviceCreateInfo dci = {0};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = deviceExtCount;
    dci.ppEnabledExtensionNames = deviceExts;
    dci.pEnabledFeatures = &features;

    // Create device
    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(physicalDevice, &dci, NULL, &device) != VK_SUCCESS) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> Failed to create a device");
        return 0;
    }
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);

    // Store everything inside vkState
    displayManager->instance = instance;
    displayManager->physicalDevice = physicalDevice;
    displayManager->device = device;
    displayManager->graphicsQueue = graphicsQueue;
    displayManager->graphicsFamily = graphicsFamily;
    displayManager->sharedRenderPass = NULL; // Will be created lazily when the first window is created

    // Success
    return 1;
}

void DisplayManager_DestroyVulkanDevices(DisplayManager* displayManager)
{
    if (displayManager->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(displayManager->device);
        vkDestroyDevice(displayManager->device, NULL);
        displayManager->device = VK_NULL_HANDLE;
    }
    if (displayManager->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(displayManager->instance, NULL);
        displayManager->instance = VK_NULL_HANDLE;
    }
    if (displayManager->sharedRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(displayManager->device, displayManager->sharedRenderPass, NULL);
        displayManager->sharedRenderPass = VK_NULL_HANDLE;
    }
    displayManager->physicalDevice = VK_NULL_HANDLE;
    displayManager->graphicsQueue = VK_NULL_HANDLE;
    displayManager->graphicsFamily = 0;
}

int DisplayManager_Init(DisplayManager* displayManager)
{
    // Init datastructures
    displayManager->recycledSDLWindowCount = 0;
    displayManager->lastClickedWindow = NULL;
    displayManager->windows = Container_Create(sizeof(NU_Window));
    Array_Init(&displayManager->absoluteRootNodes, sizeof(Node*), 8);
    Hashmap_Init(&displayManager->clipMap, sizeof(Node*), sizeof(NU_ClipBounds), 16);

    // Create vulkan devices
    if (!DisplayManager_CreateVulkanDevices(displayManager)) {
        return 0;
    }

    // Init GPU memory allocator
    ArenaAllocatorGPU_Init(&displayManager->gpuArenaAlloc,
        displayManager->device, displayManager->physicalDevice
    );

    // Create quad buffer
    NU_CreateQuadBuffer(displayManager->physicalDevice, displayManager->device);

    // Create render pipelines
    NU_CreateVertexRGBPipeline(displayManager,
        &displayManager->vertexRGBPipeline,
        false);

    // Success
    return 1;
}

void DisplayManager_Free(DisplayManager* displayManager)
{
    // 1. Wait for the GPU to finish everything
    if (displayManager->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(displayManager->device);
    }

    // 2. Destroy GPU arena allocator
    ArenaAllocatorGPU_Destroy(&displayManager->gpuArenaAlloc);

    // 3. Destroy render pipelines
    NU_DestroyPipeline(&displayManager->vertexRGBPipeline);

    // 4. Destroy quad buffer
    NU_DestroyQuadBuffer(displayManager->device);

    // 5. Destroy per-window Vulkan objects, then SDL window and free memory
    for (uint32_t i=0; i<displayManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&displayManager->windows, i);
        NuWindow_DestroyVulkanDevices(win, displayManager);
        if (win->window) SDL_DestroyWindow(win->window);
        Array_Free(&win->drawlist.drawNodes);
        Array_Free(&win->drawlist.clippedDrawNodes);
        Array_Free(&win->drawlist.canvasNodes);
    }

    // 6. Destroy recycled SDL windows
    for (int i=0; i<displayManager->recycledSDLWindowCount; i++) {
        SDL_DestroyWindow(displayManager->recycledSDLWindows[i]);
    }
    displayManager->recycledSDLWindowCount = 0;

    // 7. Destroy display manager vulkan devices
    DisplayManager_DestroyVulkanDevices(displayManager);

    // 8. Free display manager allocations
    Container_Free(&displayManager->windows);
    Array_Free(&displayManager->absoluteRootNodes);
    Hashmap_Free(&displayManager->clipMap);
    displayManager->hoveredWindowNode = NULL;
}

void DisplayManager_UpdateWindowRenderScale(DisplayManager* displayManager, int windowID)
{
    NU_Window* win = Container_Get(&displayManager->windows, windowID);
    int winPtsW, winPtsH;
    int winPxW, winPxH;
    SDL_GetWindowSize(win->window, &winPtsW, &winPtsH);
    SDL_GetWindowSizeInPixels(win->window, &winPxW, &winPxH);
    win->renderScale = (float)winPxW / (float)winPtsW;
}

void DisplayManager_UpdateWindowScreenDimensions(DisplayManager* displayManager, int windowID)
{
    NU_Window* win = Container_Get(&displayManager->windows, windowID);
    SDL_DisplayID displayID = SDL_GetDisplayForWindow(win->window);
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    if (mode != NULL) {
        win->screenWidth = mode->w * win->renderScale;
        win->screenHeight = mode->h * win->renderScale;
    }
}

void DisplayManager_CreateWindow(DisplayManager* displayManager, Node* node, const char* title, int width, int height)
{
    // Create / recycle SDL window
    SDL_Window* sdlWin = NULL;
    if (displayManager->recycledSDLWindowCount > 0) {
        sdlWin = displayManager->recycledSDLWindows[displayManager->recycledSDLWindowCount-1];
    }
    else {
        sdlWin = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    }

    // Create a new NU_Window
    NU_Window window;
    window.sdlWin = sdlWin;
    window.screenWidth = -1;
    window.screenHeight = -1;
    window.renderScale = -1;

    // Create a swapchain, framebuffer and renderpass
    NuWindow_CreateVulkanDevices(&window, displayManager);

    // Init drawlist
    Array_Init(&window.drawlist.drawNodes, sizeof(Node*), 512);
    Array_Init(&window.drawlist.clippedDrawNodes, sizeof(Node*), 64);
    Array_Init(&window.drawlist.canvasNodes, sizeof(Node*), 16);

    // Add window do display manager
    node->windowID = Container_Add(&displayManager->windows, &window);
    DisplayManager_UpdateWindowRenderScale(displayManager, node->windowID);
    DisplayManager_UpdateWindowScreenDimensions(displayManager, node->windowID);
}

void DisplayManager_DestroyWindow(DisplayManager* displayManager, Node* node)
{
    NU_Window* nuWin = Container_Get(&displayManager->windows, node->windowID);

    // Set last clicked window to NULL if it was destroyed
    if (nuWin->window == displayManager->lastClickedWindow) {
        displayManager->lastClickedWindow = NULL;
    }

    // Recycle window if possible
    if (displayManager->recycledSDLWindows.size < RECYCLED_WINDOWS_CAPACITY) {
        Array_Push(&displayManager->recycledSDLWindows, &nuWin->window);
    }
    else {
        SDL_DestroyWindow(nuWin->window);
    }

    // Destroy vulkan devices
    NuWindow_DestroyVulkanDevices(nuWin, displayManager);

    // Free window allocations
    Array_Free(&nuWin->drawlist.drawNodes);
    Array_Free(&nuWin->drawlist.clippedDrawNodes);
    Array_Free(&nuWin->drawlist.canvasNodes);

    // Remove window from display manager
    Container_Remove(&displayManager->windows, node->windowID);
}






int GetFrametime()
{
    int frameTimeMs = 16;
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
    if (mode && mode->refresh_rate > 0) {
        frameTimeMs = 1000 / mode->refresh_rate;
    }
    return frameTimeMs;
}

inline NU_Window* GetNodeWindow(DisplayManager* displayManager, Node* node)
{
    return Container_Get(&displayManager->windows, node->windowID);
}

SDL_Window* GetSDL_Window(DisplayManager* displayManager, int windowID)
{
    NU_Window* win = Container_Get(&displayManager->windows, windowID);
    return win->window;
}

Node* GetWindowNodeFromSDL_Window(DisplayManager* displayManager, SDL_Window* window)
{
    for (int i=0; i<displayManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&displayManager->windows, i);
        if (win->window == window) {
            return win->windowNode;
        }
    }
    return NULL;
}

NU_WindowDrawlist* GetDrawlist(DisplayManager* displayManager, int windowID)
{
    NU_Window* win = Container_Get(&displayManager->windows, windowID);
    return &win->drawlist;
}

void AssignRootWindow(DisplayManager* displayManager, Node* rootNode)
{
    SDL_Window* window = GetSDL_Window(displayManager, displayManager->rootWindowID);
    SDL_ShowWindow(window);

    int winW, winH;
    SDL_GetWindowSizeInPixels(window, &winW, &winH);
    rootNode->public.generic.width = (float)winW;
    rootNode->public.generic.height = (float)winH;
    rootNode->public.generic.minWidth = winW;
    rootNode->public.generic.maxWidth = winW;
    rootNode->public.generic.minHeight = winH;
    rootNode->public.generic.maxHeight = winH;
    rootNode->windowID = displayManager->rootWindowID;
    NU_Window* rootWin = Container_Get(&displayManager->windows, rootNode->windowID);
    rootWin->windowNode = rootNode;

    DisplayManager_UpdateWindowRenderScale(displayManager, rootNode->windowID);
    DisplayManager_UpdateWindowScreenDimensions(displayManager, rootNode->windowID);

    // Initialise drawlist
    NU_WindowDrawlist* list = GetDrawlist(displayManager, displayManager->rootWindowID);
    Array_Init(&list->drawNodes, sizeof(Node*), 512);
    Array_Init(&list->clippedDrawNodes, sizeof(Node*), 64);
    Array_Init(&list->canvasNodes, sizeof(Node*), 16);

    NU_Draw_Init();
}

void GetLocalMouseCoords(DisplayManager* displayManager, float* outX, float* outY)
{
    NU_Window* win = Container_Get(&displayManager->windows, displayManager->hoveredWindowNode->windowID);
    float globalPtsX, globalPtsY;
    int windowPtsX, windowPtsY;
    SDL_GetGlobalMouseState(&globalPtsX, &globalPtsY);
    SDL_GetWindowPosition(win->window, &windowPtsX, &windowPtsY);
    *outX = (globalPtsX - windowPtsX) * win->renderScale;
    *outY = (globalPtsY - windowPtsY) * win->renderScale;
}

void DisplayManager_SetHoveredWindow(DisplayManager* displayManager, SDL_Window* window)
{
    for (int i=0; i<displayManager->windows.size; i++) {
        NU_Window* win = Container_GetAt(&displayManager->windows, i);
        if (win->window == window) {
            displayManager->hoveredWindowNode = win->windowNode;
            return;
        }
    }
    displayManager->hoveredWindowNode = NULL;
}

inline void WindowBeginFrame(SDL_Window* window)
{
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    glViewport(0, 0, w, h); glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

inline void SetNodeDrawlist_Draw(DisplayManager* displayManager, Node* node)
{
    NU_WindowDrawlist* list = GetDrawlist(displayManager, node->windowID);
    Array_Push(&list->drawNodes, &node);
}

inline void SetNodeDrawlist_Clipped(DisplayManager* displayManager, Node* node)
{
    NU_WindowDrawlist* list = GetDrawlist(displayManager, node->windowID);
    Array_Push(&list->clippedDrawNodes, &node);
}

inline void SetNodeDrawlist_Canvas(DisplayManager* displayManager, Node* node)
{
    NU_WindowDrawlist* list = GetDrawlist(displayManager, node->windowID);
    Array_Push(&list->canvasNodes, &node);
}
