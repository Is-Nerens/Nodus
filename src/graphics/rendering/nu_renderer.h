#pragma once

// #include <rendering/gui/nu_mesh_generation.h>
// #include <rendering/nu_renderer_structures.h>
// #include <rendering/nu_shader.h>

void NU_DrawVertexRGB(
    NuWindow* window,
    DisplayManager* displayManager,
    Array* vertices,
    Array* indices)
{
    if (vertices->size == 0 || indices->size == 0) return;

    VkCommandBuffer cmd = window->activeCommandBuffer;

    // ---- Bind pipeline ----
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      displayManager->vertexRGBPipeline.pipeline);

    // ---- Dynamic viewport / scissor ----
    VkViewport vp = {
        0.0f, 0.0f,
        (float)window->swapchainExtent.width,
        (float)window->swapchainExtent.height,
        0.0f, 1.0f
    };
    VkRect2D sc = {{0, 0}, window->swapchainExtent};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    // ---- Push constants ----
    // 4 floats: screenW, screenH, offsetX, offsetY
    float push[4] = { (float)window->screenWidth, (float)window->screenHeight, 0.0f, 0.0f };
    vkCmdPushConstants(cmd, displayManager->vertexRGBPipeline.layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(push), push);

    // ---- Allocate vertex data from the arena ----
    VkDeviceSize vertBytes = vertices->size * vertices->elementSize;
    ArenaAllocGPU vertAlloc = ArenaAllocatorGPU_Alloc(&displayManager->gpuArenaAlloc, vertBytes, 16);
    if (vertAlloc.buffer == VK_NULL_HANDLE) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vertexRGB vertex arena alloc failed");
        return;
    }
    memcpy((char*)vertAlloc.mapped + vertAlloc.offset,
           vertices->data,
           (size_t)vertBytes);

    // ---- Allocate index data from the arena ----
    VkDeviceSize idxBytes = indices->size * indices->elementSize;
    ArenaAllocGPU idxAlloc = ArenaAllocatorGPU_Alloc(&displayManager->gpuArenaAlloc, idxBytes, 16);
    if (idxAlloc.buffer == VK_NULL_HANDLE) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> vertexRGB index arena alloc failed");
        return;
    }
    memcpy((char*)idxAlloc.mapped + idxAlloc.offset,
           indices->data,
           (size_t)idxBytes);

    // ---- Bind vertex + index buffers ----
    VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertAlloc.buffer, &vertAlloc.offset);
    vkCmdBindIndexBuffer(cmd, idxAlloc.buffer, idxAlloc.offset, VK_INDEX_TYPE_UINT32);

    // ---- Draw ----
    vkCmdDrawIndexed(cmd, (uint32_t)indices->size, 1, 0, 0, 0);
}

void NU_DrawClippedVertexRGB(
    NU_Window* window,
    DisplayManager* displayManager,
    Array* vertices,
    Array* indices,
    float clipTop, float clipBottom,
    float clipLeft, float clipRight)
{

}

void NU_DrawBorderRects(
    NuWindow* window,
    DisplayManager* displayManager,
    Array borderRects,
    float screenWidth,
    float screenHeight)
{
    if (borderRects.size == 0) return;

    VkCommandBuffer cmd = window->activeCommandBuffer;

    // ---- Bind pipeline ----
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      displayManager->borderRectPipeline.pipeline);

    // ---- Dynamic viewport / scissor (full window) ----
    VkViewport vp = {
        0.0f, 0.0f,
        (float)window->swapchainExtent.width,
        (float)window->swapchainExtent.height,
        0.0f, 1.0f
    };
    VkRect2D sc = {{0, 0}, window->swapchainExtent};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    // ---- Push constants: screen width / height ----
    float push[2] = { screenWidth, screenHeight };
    vkCmdPushConstants(cmd, displayManager->borderRectPipeline.layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(push), push);

    // ---- Allocate instance data from the arena ----
    //
    // The arena gives us a (VkBuffer, VkDeviceSize offset) pair that stays
    // valid until the slot is swapped back in N frames. We write directly
    // into the mapped pointer.
    VkDeviceSize bytes = borderRects.size * borderRects.elementSize;

    // Vertex attributes need 4-byte alignment at minimum, and some drivers
    // prefer 16. 16 is the safe choice and costs nothing.
    ArenaAllocGPU alloc = ArenaAllocatorGPU_Alloc(&displayManager->gpuArenaAlloc, bytes, 16);
    if (alloc.buffer == VK_NULL_HANDLE) {
        ErrorSystem_AddError(&GUI.errorSystem, "<Vulkan Error> border rect arena alloc failed");
        return;
    }

    // Find the mapped pointer for this (buffer, offset).
    //
    // The arena doesn't return the mapped pointer directly, only the buffer
    // and offset. You need a way to get from a VkBuffer back to its mapped
    // memory. Two options:
    //
    //   (a) add a `void* mapped` field to ArenaAllocGPU so the allocator
    //       returns it (recommended — one-line change to the arena).
    //
    //   (b) walk arena->slots[arena->currentSlot].chunks[] to find the
    //       chunk with matching VkBuffer, then use its `mapped`.
    //
    // Option (a) is much nicer. Assuming you've added `void* mapped` to
    // ArenaAllocGPU:
    memcpy((char*)alloc.mapped + alloc.offset, borderRects.data, (size_t)bytes);

    // ---- Bind quad + instance buffers ----
    VkBuffer     bufs[2]    = { quadBuffer, alloc.buffer };
    VkDeviceSize offsets[2] = { 0, alloc.offset };
    vkCmdBindVertexBuffers(cmd, 0, 2, bufs, offsets);

    // ---- Draw ----
    vkCmdDraw(cmd, 6, (uint32_t)borderRects.size, 0, 0);
}

void NU_DrawImage(
    NuWindow* window,
    DisplayManager* displayManager,
    ImageRenderData* imageRenderData)
{

}

void NU_DrawImages(
    NuWindow* window,
    DisplayManager* displayManager,
    Array imageRenderDatas,
    GLuint imageHandle)
{

}

void NU_DrawText(
    NuWindow* window,
    DisplayManager* displayManager,
    Array textRenderData,
    float offsetX,
    float offsetY)
{

}

void NU_DrawClippedText(
    NuWindow* window,
    DisplayManager* displayManager,
    Array textRenderData,
    float offsetX,
    float offsetY,
    float clipTop,
    float clipBottom,
    float clipLeft,
    flaot clipRight)
{

}
