#pragma once

#include "nu_canvas_render_target.h"

#define NORMALIZE(dx, dy) do { float32 len = sqrtf((dx)*(dx) + (dy)*(dy)); if (len > 0.0001f) { dx /= len; dy /= len; } } while(0)

typedef struct {
    Vertex_RGB_List vertices;
    Index_List indices;
    uint32 zValuesRetargeted; // Number of vertices with finalised z values
} CanvasMeshLayer;

typedef struct {
    Array textRenderData;
    uint32 zValuesRetargeted; // Number of array elements with finalised z values
    int fontIdx;
} CanvasTextLayer;

typedef struct {

    // Layers
    CanvasMeshLayer meshLayer;
    Array textLayers;

    // Node state
    float32 canvasWidth;
    float32 canvasHeight;
    Node* node;

    // Context state
    uint32 selectedTextLayerIdx; // If this == -1 then we are working on the mesh layer
    bool isMeshLayer;
    float32 zLayer;

    // Render target
    CanvasRenderTarget renderTarget;
    bool renderUpdated;
} NU_Canvas_Context;

void Canvas_InitContext(Node* node, NU_Canvas_Context* ctx, DisplayManager* displayManager, ImageResourceManager* imageResourceManager)
{
    // Init node state
    ctx->node = node;
    ctx->canvasWidth = NODE_INNER_WIDTH(node);
    ctx->canvasHeight = NODE_INNER_HEIGHT(node);
    ctx->canvasWidth = max(ctx->canvasWidth, 50);
    ctx->canvasHeight = max(ctx->canvasHeight, 50);

    // Init context state
    ctx->isMeshLayer = true;
    ctx->selectedTextLayerIdx = 0;
    ctx->zLayer = 1.0f;

    // Initialise mesh layer
    Vertex_RGB_List_Init(&ctx->meshLayer.vertices, 1024);
    Index_List_Init(&ctx->meshLayer.indices, 2048);
    ctx->meshLayer.zValuesRetargeted = 0;

    // Initialise default text layer
    Array_Init(&ctx->textLayers, sizeof(CanvasTextLayer), 4);
    CanvasTextLayer* defaultTextLayer = Array_PushEmpty(&ctx->textLayers);
    defaultTextLayer->fontIdx = 0;
    defaultTextLayer->zValuesRetargeted = 0;
    Array_Init(&defaultTextLayer->textRenderData, sizeof(GlyphRenderData), 256);

    // Create a render target
    CanvasRenderTarget_Create(
        displayManager,
        imageResourceManager,
        &ctx->renderTarget,
        ctx->canvasWidth,
        ctx->canvasHeight
    );

    // Must be rendered
    ctx->renderUpdated = false;
}

void Canvas_FreeContext(NU_Canvas_Context* ctx, DisplayManager* displayManager, ImageResourceManager* imageResourceManager)
{
    // Free mesh layer
    Vertex_RGB_List_Free(&ctx->meshLayer.vertices);
    Index_List_Free(&ctx->meshLayer.indices);

    // Free text layers
    for (uint32 i=0; i<Array_Length(&ctx->textLayers); i++) {
        CanvasTextLayer* textLayer = Array_Get(&ctx->textLayers, i);
        Array_Free(&textLayer->textRenderData);
    }
    Array_Free(&ctx->textLayers);

    // Destroy render target
    CanvasRenderTarget_Destroy(displayManager, imageResourceManager, &ctx->renderTarget);
}

void Canvas_ClearContext(NU_Canvas_Context* ctx, DisplayManager* displayManager, ImageResourceManager* imageResourceManager)
{
    // Clear mesh layer
    Vertex_RGB_List_Clear(&ctx->meshLayer.vertices);
    Index_List_Clear(&ctx->meshLayer.indices);
    ctx->meshLayer.zValuesRetargeted = 0;

    // Clear text layers
    for (uint32 i=0; i<Array_Length(&ctx->textLayers); i++) {
        CanvasTextLayer* textLayer = Array_Get(&ctx->textLayers, i);
        Array_Clear(&textLayer->textRenderData);
        textLayer->zValuesRetargeted = 0;
    }

    // Reset state
    ctx->selectedTextLayerIdx = 0;
    ctx->zLayer = 1.0f;

    // Calculate width and height
    float newWidth = NODE_INNER_WIDTH(ctx->node);
    float newHeight = NODE_INNER_HEIGHT(ctx->node);
    newWidth = max(newWidth, 50);
    newHeight = max(newHeight, 50);

    // Size changed -> recreate render target
    if (ctx->canvasWidth != newWidth || ctx->canvasHeight != newHeight) {
        CanvasRenderTarget_Resize(
            displayManager,
            imageResourceManager,
            &ctx->renderTarget,
            (uint32)(newWidth),
            (uint32)(newHeight)
        );
    }
    NuColor clearColor = {
        ctx->node->public.generic.backgroundR,
        ctx->node->public.generic.backgroundG,
        ctx->node->public.generic.backgroundB,
        255,
    };

    // Update width and height
    ctx->canvasWidth = newWidth;
    ctx->canvasHeight = newHeight;

    // Must be re-rendered
    ctx->renderUpdated = false;
}

void Canvas_SetFont(NU_Canvas_Context* ctx, const char* fontName)
{
    // Get fontID
    void* found = LinearStringmap_Get(&GUI.stylesheet.fontNameIndexMap, fontName);
    int fontIdx = 0;
    if (found) fontIdx = *(int*)found;

    // Search text layers to see if there exists one for this font
    CanvasTextLayer* layer = NULL; int layerIndex = -1;
    for (uint32 l=0; l<Array_Length(&ctx->textLayers); l++) {
        CanvasTextLayer* candidateLayer = Array_Get(&ctx->textLayers, l);
        if (fontIdx == candidateLayer->fontIdx) {
            layer = candidateLayer;
            layerIndex = l;
            break;
        }
    }

    // If no layer exists for this font -> create a new layer
    if (layer == NULL) {
        layer = Array_PushEmpty(&ctx->textLayers);
        layer->fontIdx = fontIdx;
        layer->zValuesRetargeted = 0;
        Array_Init(&layer->textRenderData, sizeof(GlyphRenderData), 256);
        ctx->selectedTextLayerIdx = Array_Length(&ctx->textLayers) - 1;
    }
    // Found existing layer for this font
    else {
        // Switching from mesh layer -> increment z layer
        if (ctx->isMeshLayer) ctx->zLayer += 1.0f;
        // Switching from different text layer -> incremenz z layer
        else if (layerIndex != ctx->selectedTextLayerIdx) ctx->zLayer += 1.0f;
        // Is already on this text layer -> early return
        else {
            return;
        }

        // Update selected text layer
        ctx->selectedTextLayerIdx = layerIndex;
    }
}


void Canvas_DrawBorderRect(
    NU_Canvas_Context* ctx,
    float32 x, float32 y, float32 w, float32 h,
    float32 thickness,
    NuColor borderColor,
    NuColor fillColor)
{
    // Constrain dimensions based on thickness
    w = fmaxf(w, thickness * 2);
    h = fmaxf(h, thickness * 2);

    // Skip function if rect is not visible
    if (x + w < 0.0f || x > ctx->canvasWidth || y + h < 0.0f || y > ctx->canvasHeight) return;

    // Switching from text layer -> switch to mesh layer and increment z layer
    if (!ctx->isMeshLayer) {
        ctx->isMeshLayer = true;
        ctx->zLayer += 1.0f;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->meshLayer.vertices;
    Index_List* indices = &ctx->meshLayer.indices;

    // --- Allocate extra space in vertex and index lists ---
    uint32 additional_vertices = 12;
    uint32 additional_indices = 30;
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    uint32 vertex_offset = vertices->size;

    float32 z = ctx->zLayer;

    // Pack colors into float32
    float32 packedBorderColor = PackColorIntoFloat32(borderColor);
    float32 packedFillColor = PackColorIntoFloat32(fillColor);

    // Outer TL
    vertices->array[vertex_offset] = (vertex_rgb){ x, y, z, packedBorderColor };

    // Outer TR
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x + w, y, z, packedBorderColor };

    // Outer BL
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x, y + h, z, packedBorderColor };

    // Outer BR
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x + w, y + h, z, packedBorderColor };

    // Inner TL
    vertices->array[vertex_offset + 4] = (vertex_rgb){ x + thickness, y + thickness, z, packedBorderColor };

    // Inner TR
    vertices->array[vertex_offset + 5] = (vertex_rgb){ x + w - thickness, y + thickness, z, packedBorderColor };

    // Inner BL
    vertices->array[vertex_offset + 6] = (vertex_rgb){ x + thickness, y + h - thickness, z, packedBorderColor };

    // Inner BR
    vertices->array[vertex_offset + 7] = (vertex_rgb){ x + w - thickness, y + h - thickness, z, packedBorderColor };

    // Fill TL
    vertices->array[vertex_offset + 8] = (vertex_rgb){ x + thickness, y + thickness, z, packedFillColor };

    // Fill TR
    vertices->array[vertex_offset + 9] = (vertex_rgb){ x + w - thickness, y + thickness, z, packedFillColor };

    // Fill BL
    vertices->array[vertex_offset + 10] = (vertex_rgb){ x + thickness, y + h - thickness, z, packedFillColor };

    // Fill BR
    vertices->array[vertex_offset + 11] = (vertex_rgb){ x + w - thickness, y + h - thickness, z, packedFillColor };

    // Indices
    uint32* indices_write = indices->array + indices->size;

    // Top border face
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 5;

    // Right border face
    *indices_write++ = vertex_offset + 5;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 3;
    *indices_write++ = vertex_offset + 5;
    *indices_write++ = vertex_offset + 7;
    *indices_write++ = vertex_offset + 3;

    // Bottom border face
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 7;
    *indices_write++ = vertex_offset + 3;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 6;
    *indices_write++ = vertex_offset + 7;

    // Left border face
    *indices_write++ = vertex_offset + 0;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 6;

    // Center face
    *indices_write++ = vertex_offset + 8;
    *indices_write++ = vertex_offset + 9;
    *indices_write++ = vertex_offset + 10;
    *indices_write++ = vertex_offset + 9;
    *indices_write++ = vertex_offset + 10;
    *indices_write++ = vertex_offset + 11;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void Canvas_DrawTriangle(
    NU_Canvas_Context* ctx,
    float32 x1, float32 y1,
    float32 x2, float32 y2,
    float32 x3, float32 y3,
    float32 thickness,
    NuColor borderColor,
    NuColor fillColor)
{
    // Skip if triangle is offscreen
    float32 minX = fminf(x1, fminf(x2, x3));
    float32 maxX = fmaxf(x1, fmaxf(x2, x3));
    float32 minY = fminf(y1, fminf(y2, y3));
    float32 maxY = fmaxf(y1, fmaxf(y2, y3));
    if (maxX < 0.0f || minX > ctx->canvasWidth || maxY < 0.0f || minY > ctx->canvasHeight) return;

    // Switching from text layer -> switch to mesh layer and increment z layer
    if (!ctx->isMeshLayer) {
        ctx->isMeshLayer = true;
        ctx->zLayer += 1.0f;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->meshLayer.vertices;
    Index_List* indices = &ctx->meshLayer.indices;

    float32 z = ctx->zLayer;

    // --- Allocate extra space in vertex and index lists ---
    uint32 additional_vertices = 6;
    uint32 additional_indices = 21;
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    uint32 vertex_offset = vertices->size;

    // Pack colors into float32
    float32 packedBorderColor = PackColorIntoFloat32(borderColor);
    float32 packedFillColor = PackColorIntoFloat32(fillColor);

    // --- Outer triangle (border) ---
    vertices->array[vertex_offset + 0] = (vertex_rgb){ x1, y1, packedBorderColor };
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x2, y2, packedBorderColor };
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x3, y3, packedBorderColor };

    // Compute inward offset (very simple approximation using centroid)
    float32 cx = (x1 + x2 + x3) / 3.0f;
    float32 cy = (y1 + y2 + y3) / 3.0f;

    // Inner triangle points
    float32 dx1 = cx - x1, dy1 = cy - y1; NORMALIZE(dx1, dy1);
    float32 dx2 = cx - x2, dy2 = cy - y2; NORMALIZE(dx2, dy2);
    float32 dx3 = cx - x3, dy3 = cy - y3; NORMALIZE(dx3, dy3);

    // --- Inner triangle (fill) ---
    vertices->array[vertex_offset + 3] = (vertex_rgb){
        x1 + dx1 * thickness, y1 + dy1 * thickness, z,
        packedFillColor
    };
    vertices->array[vertex_offset + 4] = (vertex_rgb){
        x2 + dx2 * thickness, y2 + dy2 * thickness, z,
        packedFillColor
    };
    vertices->array[vertex_offset + 5] = (vertex_rgb){
        x3 + dx3 * thickness, y3 + dy3 * thickness, z,
        packedFillColor
    };

    // --- Indices ---
    uint32* indices_write = indices->array + indices->size;

    // Edge 1
    *indices_write++ = vertex_offset + 0;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 3;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 3;
    *indices_write++ = vertex_offset + 4;

    // Edge 2
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 5;

    // Edge 3
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 0;
    *indices_write++ = vertex_offset + 5;
    *indices_write++ = vertex_offset + 0;
    *indices_write++ = vertex_offset + 5;
    *indices_write++ = vertex_offset + 3;

    // Center
    *indices_write++ = vertex_offset + 3;
    *indices_write++ = vertex_offset + 4;
    *indices_write++ = vertex_offset + 5;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void Canvas_DrawVline(NU_Canvas_Context* ctx, float32 x, float32 y, float32 height, float32 thickness, NuColor color)
{
    float32 canvasWidth = ctx->canvasWidth;
    float32 canvasHeight = ctx->canvasHeight;

    // Skip if line is not visible on canvas
    if (x < -thickness || x > canvasWidth + thickness || y > canvasHeight || y + height < 0.0f) return;

    // Constrain to avoid large numbers
    if (y < 0.0f) y = 0.0f;
    if (y + height > canvasHeight) height = canvasHeight - y;

    // Switching from text layer -> switch to mesh layer and increment z layer
    if (!ctx->isMeshLayer) {
        ctx->isMeshLayer = true;
        ctx->zLayer += 1.0f;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->meshLayer.vertices;
    Index_List* indices = &ctx->meshLayer.indices;

    float32 z = ctx->zLayer;

    // --- Allocate extra space in vertex and index lists ---
    uint32 additional_vertices = 4;
    uint32 additional_indices = 6;
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    // Add pixel alignment offset
    x += 0.5f;
    float32 half_thick = thickness * 0.5f;

    // Pack color into float32
    float32 packedColor = PackColorIntoFloat32(color);

    // Create mesh
    uint32 vertex_offset = vertices->size;
    vertices->array[vertex_offset] = (vertex_rgb){ x - half_thick, y, z, packedColor }; // Bottom point (y) - thick -
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x + half_thick, y, z, packedColor }; // Bottom point (y) - thick +
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x - half_thick, y + height, z, packedColor }; // Top point (y1) - thick -
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x + half_thick, y + height, z, packedColor }; // Top point (y1) - thick +

    // Indices
    uint32* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void Canvas_DrawHline(NU_Canvas_Context* ctx, float32 x, float32 y, float32 width, float32 thickness, NuColor color)
{
    float32 canvasWidth = ctx->canvasWidth;
    float32 canvasHeight = ctx->canvasHeight;

    // Skip if line is not visible on canvas
    if (y < -thickness || y > canvasHeight + thickness || x > canvasWidth || x + width < 0.0f) return;

    // Constrain to avoid large numbers
    if (x < 0.0f) x = 0.0f;
    if (x + width > canvasWidth) width = canvasWidth - x;

    // Switching from text layer -> switch to mesh layer and increment z layer
    if (!ctx->isMeshLayer) {
        ctx->isMeshLayer = true;
        ctx->zLayer += 1.0f;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->meshLayer.vertices;
    Index_List* indices = &ctx->meshLayer.indices;

    float32 z = ctx->zLayer;

    // --- Allocate extra space in vertex and index lists ---
    uint32 additional_vertices = 4;
    uint32 additional_indices = 6;
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    // Add pixel alignment offset
    y += 0.5f;
    float32 half_thick = thickness * 0.5f;

    // Pack color into float32
    float32 packedColor = PackColorIntoFloat32(color);

    // Create mesh
    uint32 vertex_offset = vertices->size;
    vertices->array[vertex_offset] = (vertex_rgb){ x, y - half_thick, z, packedColor }; // Left point (x) - thick -
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x, y + half_thick, z, packedColor }; // Left point (x) - thick +
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x + width, y - half_thick, z, packedColor }; // Right point (x1) - thick -
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x + width, y + half_thick, z, packedColor }; // Right point (x1) - thick +

    // Indices
    uint32* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void Canvas_DrawLine(
    NU_Canvas_Context* ctx,
    float32 x1, float32 y1, float32 x2, float32 y2,
    float32 thickness,
    NuColor color)
{
    float32 twoThick = thickness * 2.0f;
    float32 canvasWidth = ctx->canvasWidth;
    float32 canvasHeight = ctx->canvasHeight;

    // Skip if line is not visible on canvas
    if ((x1 < -twoThick && x2 < -twoThick) ||
        (x1 > ctx->canvasWidth && x2 > ctx->canvasWidth) ||
        (y1 < -twoThick && y2 < -twoThick) ||
        (y1 > ctx->canvasHeight + twoThick && y2 > ctx->canvasHeight + twoThick)) {
        return;
    }

    bool coordsWithinLargeVirtualCanvas = x1 >= -twoThick && x1 <= canvasWidth + twoThick &&
                                          x2 >= -twoThick && x2 <= canvasWidth + twoThick &&
                                          y1 >= -twoThick && y1 <= canvasHeight + twoThick &&
                                          y2 >= -twoThick && y2 <= canvasHeight + twoThick;

    // Implement fancy clipping -> moves coords closer to canvas
    if (!coordsWithinLargeVirtualCanvas)
    {
        const float32 MINX = -twoThick, MINY = -twoThick;
        const float32 MAXX = ctx->canvasWidth + twoThick * 2.0f, MAXY = ctx->canvasHeight + twoThick * 2.0f;
        float32 dx = x2 - x1;
        float32 dy = y2 - y1;
        float32 t0 = 0.0f;
        float32 t1 = 1.0f;

        // left
        float32 p = -dx, q = x1 - MINX;
        if (p == 0.0f) {
            if (q < 0.0f) return;
        } else {
            float32 r = q / p;
            if (p < 0.0f) {
                if (r > t1) return;
                if (r > t0) t0 = r;
            } else {
                if (r < t0) return;
                if (r < t1) t1 = r;
            }
        }

        // right
        p = dx; q = MAXX - x1;
        if (p == 0.0f) {
            if (q < 0.0f) return;
        } else {
            float32 r = q / p;
            if (p < 0.0f) {
                if (r > t1) return;
                if (r > t0) t0 = r;
            } else {
                if (r < t0) return;
                if (r < t1) t1 = r;
            }
        }

        // bottom
        p = -dy; q = y1 - MINY;
        if (p == 0.0f) {
            if (q < 0.0f) return;
        } else {
            float32 r = q / p;
            if (p < 0.0f) {
                if (r > t1) return;
                if (r > t0) t0 = r;
            } else {
                if (r < t0) return;
                if (r < t1) t1 = r;
            }
        }

        // top
        p = dy; q = MAXY - y1;
        if (p == 0.0f) {
            if (q < 0.0f) return;
        } else {
            float32 r = q / p;
            if (p < 0.0f) {
                if (r > t1) return;
                if (r > t0) t0 = r;
            } else {
                if (r < t0) return;
                if (r < t1) t1 = r;
            }
        }

        // final rejection
        if (t0 > t1) return;

        // apply clipping
        float32 nx1 = x1 + dx * t0;
        float32 ny1 = y1 + dy * t0;
        float32 nx2 = x1 + dx * t1;
        float32 ny2 = y1 + dy * t1;
        x1 = nx1; y1 = ny1;
        x2 = nx2; y2 = ny2;
    }

    // Switching from text layer -> switch to mesh layer and increment z layer
    if (!ctx->isMeshLayer) {
        ctx->isMeshLayer = true;
        ctx->zLayer += 1.0f;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->meshLayer.vertices;
    Index_List* indices = &ctx->meshLayer.indices;

    float32 z = ctx->zLayer;

    // --- Allocate extra space in vertex and index lists ---
    uint32 additional_vertices = 4;
    uint32 additional_indices = 6;
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    x1 += 0.5f;
    x2 += 0.5f;
    y1 += 0.5f;
    y2 += 0.5f;


    float32 dx = x2 - x1;
    float32 dy = y2 - y1;
    float32 length = sqrtf(dx * dx + dy * dy);
    if (length == 0.0f) return; // avoid divide-by-zero
    dx /= length;
    dy /= length;
    float32 px = -dy;
    float32 py = dx;
    float32 half_thick = thickness * 0.5f;
    px *= half_thick;
    py *= half_thick;

    // Pack color into float32
    float32 packedColor = PackColorIntoFloat32(color);


    uint32 vertex_offset = vertices->size;
    vertices->array[vertex_offset] = (vertex_rgb){ x1 - px, y1 - py, z, packedColor }; // V1 -> thick -
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x1 + px, y1 + py, z, packedColor }; // V1 -> thick +
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x2 - px, y2 - py, z, packedColor }; // V2 -> thick -
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x2 + px, y2 + py, z, packedColor }; // V2 -> thick +

    // Indices
    uint32* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void Canvas_DrawDashedLine(
    NU_Canvas_Context* ctx,
    float32 x1, float32 y1, float32 x2, float32 y2,
    float32 thickness,
    uint8_t* dash_pattern,
    uint32 dash_pattern_len,
    NuColor color)
{
    float32 twoThick = thickness * 2.0f;
    float32 canvasWidth = ctx->canvasWidth;
    float32 canvasHeight = ctx->canvasHeight;

    // Skip if line is not visible on canvas
    if ((x1 < -twoThick && x2 < -twoThick) ||
        (x1 > ctx->canvasWidth && x2 > ctx->canvasWidth) ||
        (y1 < -twoThick && y2 < -twoThick) ||
        (y1 > ctx->canvasHeight + twoThick && y2 > ctx->canvasHeight + twoThick)) {
        return;
    }

    bool coordsWithinLargeVirtualCanvas = x1 >= -twoThick && x1 <= canvasWidth + twoThick &&
                                          x2 >= -twoThick && x2 <= canvasWidth + twoThick &&
                                          y1 >= -twoThick && y1 <= canvasHeight + twoThick &&
                                          y2 >= -twoThick && y2 <= canvasHeight + twoThick;

    float32 ox1 = x1, oy1 = y1;
    float32 ox2 = x2, oy2 = y2;
    float32 t_start = 0.0f;
    float32 t_end   = 1.0f;

    if (!coordsWithinLargeVirtualCanvas)
    {
        const float32 MINX = -twoThick, MINY = -twoThick;
        const float32 MAXX = ctx->canvasWidth + twoThick * 2.0f, MAXY = ctx->canvasHeight + twoThick * 2.0f;
        float32 dx = x2 - x1;
        float32 dy = y2 - y1;
        float32 t0 = 0.0f;
        float32 t1 = 1.0f;

        // LEFT
        {
            float32 p = -dx, q = x1 - MINX;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float32 r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        // RIGHT
        {
            float32 p = dx, q = MAXX - x1;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float32 r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        // BOTTOM
        {
            float32 p = -dy, q = y1 - MINY;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float32 r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        // TOP
        {
            float32 p = dy, q = MAXY - y1;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float32 r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        if (t0 > t1) return;

        // Calculate new clipped endpoints
        float32 cx1 = x1 + (x2 - x1) * t0;
        float32 cy1 = y1 + (y2 - y1) * t0;
        float32 cx2 = x1 + (x2 - x1) * t1;
        float32 cy2 = y1 + (y2 - y1) * t1;

        // Calculate the fractional positions along the original line
        float32 orig_dx = ox2 - ox1;
        float32 orig_dy = oy2 - oy1;
        float32 orig_len2 = orig_dx * orig_dx + orig_dy * orig_dy;

        if (orig_len2 == 0.0f) return;

        float32 inv_orig_len2 = 1.0f / orig_len2;

        // Project clipped points onto original line to get accurate t_start and t_end
        t_start = ((cx1 - ox1) * orig_dx + (cy1 - oy1) * orig_dy) * inv_orig_len2;
        t_end   = ((cx2 - ox1) * orig_dx + (cy2 - oy1) * orig_dy) * inv_orig_len2;

        // Clamp t values to ensure they're within [0,1]
        if (t_start < 0.0f) t_start = 0.0f;
        if (t_start > 1.0f) t_start = 1.0f;
        if (t_end < 0.0f) t_end = 0.0f;
        if (t_end > 1.0f) t_end = 1.0f;

        // Ensure t_start < t_end
        if (t_start > t_end) {
            float32 temp = t_start;
            t_start = t_end;
            t_end = temp;
        }

        x1 = cx1; y1 = cy1;
        x2 = cx2; y2 = cy2;
    }
    else
    {
        // No clipping needed, set t_start and t_end to full line range
        t_start = 0.0f;
        t_end = 1.0f;
    }

    // Pixel stradle offsets
    x1 += 0.5f;
    x2 += 0.5f;
    y1 += 0.5f;
    y2 += 0.5f;
    ox1 += 0.5f;
    ox2 += 0.5f;
    oy1 += 0.5f;
    oy2 += 0.5f;

    float32 dx = x2 - x1;
    float32 dy = y2 - y1;
    float32 length = sqrtf(dx * dx + dy * dy);
    if (length == 0.0f) return;

    float32 inv_len = 1.0f / length;
    float32 ox_dx = ox2 - ox1;
    float32 ox_dy = oy2 - oy1;
    float32 orig_len = sqrtf(ox_dx * ox_dx + ox_dy * ox_dy);

    if (orig_len == 0.0f) return;

    // Pre-calculate pattern total length
    float32 pattern_total_len = 0.0f;
    for (uint32 i = 0; i < dash_pattern_len; i++) {
        pattern_total_len += (float32)dash_pattern[i];
    }
    if (pattern_total_len == 0.0f) return;

    // Calculate the phase at the start of the clipped line
    float32 world_start = t_start * orig_len;
    float32 world_end   = t_end   * orig_len;

    // Find the starting offset within the pattern
    float32 phase = fmodf(world_start, pattern_total_len);
    uint32 start_pattern_index = 0;
    float32 pattern_offset = 0.0f;

    for (uint32 i = 0; i < dash_pattern_len; i++) {
        float32 segment_len = (float32)dash_pattern[i];
        if (phase < pattern_offset + segment_len) {
            start_pattern_index = i;
            break;
        }
        pattern_offset += segment_len;
    }

    float32 step_x = dx * inv_len;
    float32 step_y = dy * inv_len;


    // Switching from text layer -> switch to mesh layer and increment z layer
    if (!ctx->isMeshLayer) {
        ctx->isMeshLayer = true;
        ctx->zLayer += 1.0f;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->meshLayer.vertices;
    Index_List* indices = &ctx->meshLayer.indices;

    float32 z = ctx->zLayer;

    // Calculate number of vertices and indices are needed
    float32 pattern_len = 0.0f;
    uint32 vertices_per_pattern = 0;
    uint32 indices_per_pattern = 0;
    for (uint32 i = 0; i < dash_pattern_len; i++) {
        pattern_len += dash_pattern[i];
        vertices_per_pattern += (dash_pattern[i] * ((i % 2) == 0)) * 4;
        indices_per_pattern += (dash_pattern[i] * ((i % 2) == 0)) * 6;
    }
    float32 approx_cycles = length / pattern_len;
    uint32 min_additional_vertices = ((uint32)approx_cycles + 1) * vertices_per_pattern;
    uint32 min_additional_indices  = ((uint32)approx_cycles + 1) * indices_per_pattern;
    if (vertices->size + min_additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, min_additional_vertices);
    if (indices->size + min_additional_indices > indices->capacity) Index_List_Grow(indices, min_additional_indices);

    // Generate the mesh
    float32 travelled = world_start;
    uint32 i = start_pattern_index;
    float32 remaining_in_segment = (float32)dash_pattern[i] - (phase - pattern_offset);
    float32 half_thick = thickness * 0.5f;  // FIXED: was 0.25f

    // Pre-calculate perpendicular direction (constant for the whole line)
    float32 perp_x = -step_y * half_thick;
    float32 perp_y =  step_x * half_thick;

    // Pack color into float32
    float32 packedColor = PackColorIntoFloat32(color);

    while (travelled < world_end)
    {
        float32 segment_len = remaining_in_segment;
        float32 next = travelled + segment_len;

        if (next > world_end) {
            segment_len = world_end - travelled;
            next = world_end;
        }

        if (i % 2 == 0)
        {
            // Draw dash segment
            float32 sx1 = x1 + step_x * (travelled - world_start);
            float32 sy1 = y1 + step_y * (travelled - world_start);
            float32 sx2 = x1 + step_x * (next - world_start);
            float32 sy2 = y1 + step_y * (next - world_start);

            uint32 v0 = vertices->size;
            vertex_rgb* v = vertices->array + v0;
            v[0] = (vertex_rgb){ sx1 - perp_x, sy1 - perp_y, z, packedColor };
            v[1] = (vertex_rgb){ sx1 + perp_x, sy1 + perp_y, z, packedColor };
            v[2] = (vertex_rgb){ sx2 - perp_x, sy2 - perp_y, z, packedColor };
            v[3] = (vertex_rgb){ sx2 + perp_x, sy2 + perp_y, z, packedColor };

            uint32* id = indices->array + indices->size;
            id[0] = v0;
            id[1] = v0 + 1;
            id[2] = v0 + 2;
            id[3] = v0 + 1;
            id[4] = v0 + 2;
            id[5] = v0 + 3;

            vertices->size += 4;
            indices->size += 6;
        }

        travelled = next;
        i = (i + 1) % dash_pattern_len;
        remaining_in_segment = (float32)dash_pattern[i];
        pattern_offset = phase;
    }
}

void Canvas_DrawText(
    NU_Canvas_Context* ctx,
    Font* font,
    float32 x,
    float32 y,
    float32 wrapWidth,
    NuColor color,
    const char* string)
{
    // Switching from mesh layer -> switch to text layer and increment z layer
    if (ctx->isMeshLayer) {
        ctx->isMeshLayer = false;
        ctx->zLayer += 1.0f;
    }

    // Get text layer
    CanvasTextLayer* textLayer = Array_Get(&ctx->textLayers, ctx->selectedTextLayerIdx);

    // Compute z
    float32 z = ctx->zLayer;

    // Generate render data
    NU_Generate_Text_Renderdata(&textLayer->textRenderData, font, NULL, string, x, y, z, color, wrapWidth);
}

void Canvas_DrawLengthText(
    NU_Canvas_Context* ctx,
    Font* font,
    float32 x,
    float32 y,
    float32 wrapWidth,
    NuColor color,
    const char* string,
    size_t stringLen)
{
    // Switching from mesh layer -> switch to text layer and increment z layer
    if (ctx->isMeshLayer) {
        ctx->isMeshLayer = false;
        ctx->zLayer += 1.0f;
    }

    // Get text layer
    CanvasTextLayer* textLayer = Array_Get(&ctx->textLayers, ctx->selectedTextLayerIdx);

    // Compute z
    float32 z = ctx->zLayer;

    // Generate render data
    NU_Generate_LText_Renderdata(&textLayer->textRenderData, font, string, stringLen, x, y, z, color, wrapWidth);
}
