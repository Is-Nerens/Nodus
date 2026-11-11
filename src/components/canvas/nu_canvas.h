#pragma once

#include "nu_canvas_render_target.h"

#define NORMALIZE(dx, dy) do { float32 len = sqrtf((dx)*(dx) + (dy)*(dy)); if (len > 0.0001f) { dx /= len; dy /= len; } } while(0)

typedef enum {
    NU_CANVAS_TYPE_UNBUFFERED,
    NU_CANVAS_TYPE_BUFFERED_NO_MSAA,
    NU_CANVAS_TYPE_BUFFERED_MSAA_4X,
} CanvasRenderType;

typedef struct {
    VertexSDFList vertexList;
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

    // Render target (Optional)
    CanvasRenderTarget renderTarget;
    CanvasRenderType desiredRenderType;
    CanvasRenderType currentRenderType;
    bool renderTargetUpToDate;
} NU_Canvas_Context;

void Canvas_InitContext(
    Node* node,
    NU_Canvas_Context* ctx,
    DisplayManager* displayManager,
    ImageResourceManager* imageResourceManager)
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
    VertexSDFList_Init(&ctx->meshLayer.vertexList, 1024, 2048);
    ctx->meshLayer.zValuesRetargeted = 0;

    // Initialise default text layer
    Array_Init(&ctx->textLayers, sizeof(CanvasTextLayer), 4);
    CanvasTextLayer* defaultTextLayer = Array_PushEmpty(&ctx->textLayers);
    defaultTextLayer->fontIdx = 0;
    defaultTextLayer->zValuesRetargeted = 0;
    Array_Init(&defaultTextLayer->textRenderData, sizeof(GlyphRenderData), 256);

    // Set rendertype to default
    ctx->currentRenderType = NU_CANVAS_TYPE_UNBUFFERED;
    ctx->desiredRenderType = NU_CANVAS_TYPE_UNBUFFERED;
    ctx->renderTargetUpToDate = false;
}

void Canvas_FreeContext(NU_Canvas_Context* ctx, DisplayManager* displayManager, ImageResourceManager* imageResourceManager)
{
    // Free mesh layer
    VertexSDFList_Free(&ctx->meshLayer.vertexList);

    // Free text layers
    for (uint32 i=0; i<Array_Length(&ctx->textLayers); i++) {
        CanvasTextLayer* textLayer = Array_Get(&ctx->textLayers, i);
        Array_Free(&textLayer->textRenderData);
    }
    Array_Free(&ctx->textLayers);

    // Destroy render target if one exists
    if (ctx->currentRenderType != NU_CANVAS_TYPE_UNBUFFERED) {
        CanvasRenderTarget_Destroy(displayManager, imageResourceManager, &ctx->renderTarget);
    }
}

void Canvas_ClearContext(NU_Canvas_Context* ctx, DisplayManager* displayManager, ImageResourceManager* imageResourceManager)
{
    // Clear mesh layer
    VertexSDFList_Clear(&ctx->meshLayer.vertexList);
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

    // If canvas has a render target AND the size changed -> resize render target
    if (ctx->currentRenderType != NU_CANVAS_TYPE_UNBUFFERED &&
        (ctx->canvasWidth != newWidth || ctx->canvasHeight != newHeight))
    {
        bool MSAA_4X = (ctx->currentRenderType == NU_CANVAS_TYPE_BUFFERED_MSAA_4X);
        CanvasRenderTarget_Resize(
            displayManager,
            imageResourceManager,
            &ctx->renderTarget,
            (uint32)(newWidth),
            (uint32)(newHeight),
            MSAA_4X
        );
    }

    // Update width and height
    ctx->canvasWidth = newWidth;
    ctx->canvasHeight = newHeight;

    // Must be re-rendered
    ctx->renderTargetUpToDate = false;
}

bool Canvas_CommitDesiredRenderType(NU_Canvas_Context* ctx, DisplayManager* displayManager, ImageResourceManager* imageResourceManager)
{
    if (ctx->currentRenderType == ctx->desiredRenderType) return false; // 0 = unchanged

    // Destroy render target
    if (ctx->desiredRenderType == NU_CANVAS_TYPE_UNBUFFERED) {
        CanvasRenderTarget_Destroy(
            displayManager,
            imageResourceManager,
            &ctx->renderTarget
        );
    }
    // Create render target
    else if (ctx->currentRenderType == NU_CANVAS_TYPE_UNBUFFERED) {
        bool MSAA_4X = (ctx->desiredRenderType == NU_CANVAS_TYPE_BUFFERED_MSAA_4X);
        CanvasRenderTarget_Create(
            displayManager,
            imageResourceManager, &ctx->renderTarget,
            ctx->canvasWidth, ctx->canvasHeight,
            MSAA_4X
        );
    }
    // Resize render target
    else {
        bool MSAA_4X = (ctx->desiredRenderType == NU_CANVAS_TYPE_BUFFERED_MSAA_4X);
        CanvasRenderTarget_Resize(
            displayManager,
            imageResourceManager,
            &ctx->renderTarget,
            (uint32)(ctx->canvasWidth),
            (uint32)(ctx->canvasHeight),
            MSAA_4X
        );
    }

    ctx->currentRenderType = ctx->desiredRenderType;
    return true; // 1 = changed
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

inline bool Canvas_IS_MSAA_4X(NU_Canvas_Context* ctx) {
    return ctx->currentRenderType == NU_CANVAS_TYPE_BUFFERED_MSAA_4X;
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

    // Get vertexList
    VertexSDFList* vertexList = &ctx->meshLayer.vertexList;

    // Ensure vertex and index capacity
    uint32 additionalVertices = 12;
    uint32 additionalIndices = 30;
    VertexSDFList_EnsureVertexCapacity(vertexList, additionalVertices);
    VertexSDFList_EnsureIndexCapacity(vertexList, additionalIndices);

    // Cache z
    float32 z = ctx->zLayer;

    // Pack colors into uint32
    float32 packedBorderColor = PackColorIntoFloat32(borderColor);
    float32 packedFillColor = PackColorIntoFloat32(fillColor);

    // Perfectly straight lines -> sdf2 unused, set to large positive sentinel
    float32 sdfUnused = 1e9f;

    // Vertices
    uint32 vertexOffset = VertexSDFList_VertexCount(vertexList);

    // Outer TL
    vertexList->vertices[vertexOffset] = (vertexSDF){ x, y, z, 0.0f, sdfUnused, packedBorderColor };
    // Outer TR
    vertexList->vertices[vertexOffset + 1] = (vertexSDF){ x + w, y, z, 0.0f, sdfUnused, packedBorderColor };
    // Outer BL
    vertexList->vertices[vertexOffset + 2] = (vertexSDF){ x, y + h, z, 0.0f, sdfUnused, packedBorderColor };
    // Outer BR
    vertexList->vertices[vertexOffset + 3] = (vertexSDF){ x + w, y + h, z, 0.0f, sdfUnused, packedBorderColor };
    // Inner TL
    vertexList->vertices[vertexOffset + 4] = (vertexSDF){ x + thickness, y + thickness, z, 0.0f, sdfUnused, packedBorderColor };
    // Inner TR
    vertexList->vertices[vertexOffset + 5] = (vertexSDF){ x + w - thickness, y + thickness, z, 0.0f, sdfUnused, packedBorderColor };
    // Inner BL
    vertexList->vertices[vertexOffset + 6] = (vertexSDF){ x + thickness, y + h - thickness, z, 0.0f, sdfUnused, packedBorderColor };
    // Inner BR
    vertexList->vertices[vertexOffset + 7] = (vertexSDF){ x + w - thickness, y + h - thickness, z, 0.0f, sdfUnused, packedBorderColor };
    // Fill TL
    vertexList->vertices[vertexOffset + 8] = (vertexSDF){ x + thickness, y + thickness, z, 0.0f, sdfUnused, packedFillColor };
    // Fill TR
    vertexList->vertices[vertexOffset + 9] = (vertexSDF){ x + w - thickness, y + thickness, z, 0.0f, sdfUnused, packedFillColor };
    // Fill BL
    vertexList->vertices[vertexOffset + 10] = (vertexSDF){ x + thickness, y + h - thickness, z, 0.0f, sdfUnused, packedFillColor };
    // Fill BR
    vertexList->vertices[vertexOffset + 11] = (vertexSDF){ x + w - thickness, y + h - thickness, z, 0.0f, sdfUnused, packedFillColor };


    // Indices
    uint32* indicesWrite = vertexList->indices + VertexSDFList_IndexCount(vertexList);

    // Top border face
    *indicesWrite++ = vertexOffset;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 4;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 4;
    *indicesWrite++ = vertexOffset + 5;
    // Right border face
    *indicesWrite++ = vertexOffset + 5;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 3;
    *indicesWrite++ = vertexOffset + 5;
    *indicesWrite++ = vertexOffset + 7;
    *indicesWrite++ = vertexOffset + 3;
    // Bottom border face
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 7;
    *indicesWrite++ = vertexOffset + 3;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 6;
    *indicesWrite++ = vertexOffset + 7;
    // Left border face
    *indicesWrite++ = vertexOffset + 0;
    *indicesWrite++ = vertexOffset + 4;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 4;
    *indicesWrite++ = vertexOffset + 6;
    // Center face
    *indicesWrite++ = vertexOffset + 8;
    *indicesWrite++ = vertexOffset + 9;
    *indicesWrite++ = vertexOffset + 10;
    *indicesWrite++ = vertexOffset + 9;
    *indicesWrite++ = vertexOffset + 10;
    *indicesWrite++ = vertexOffset + 11;

    // Increment vertex and index count
    vertexList->vertexCount += additionalVertices;
    vertexList->indexCount += additionalIndices;
}

// void Canvas_DrawTriangle(
//     NU_Canvas_Context* ctx,
//     float32 x1, float32 y1,
//     float32 x2, float32 y2,
//     float32 x3, float32 y3,
//     float32 thickness,
//     NuColor borderColor,
//     NuColor fillColor)
// {
//     // Skip if triangle is offscreen
//     float32 minX = fminf(x1, fminf(x2, x3));
//     float32 maxX = fmaxf(x1, fmaxf(x2, x3));
//     float32 minY = fminf(y1, fminf(y2, y3));
//     float32 maxY = fmaxf(y1, fmaxf(y2, y3));
//     if (maxX < 0.0f || minX > ctx->canvasWidth || maxY < 0.0f || minY > ctx->canvasHeight) return;

//     // Switching from text layer -> switch to mesh layer and increment z layer
//     if (!ctx->isMeshLayer) {
//         ctx->isMeshLayer = true;
//         ctx->zLayer += 1.0f;
//     }

//     // Get vertexList
//     VertexSDFList* vertexList = &ctx->meshLayer.vertexList;

//     // Ensure vertex and index capacity
//     uint32 additionalVertices = 9;
//     uint32 additionalIndices = 21;
//     VertexSDFList_EnsureVertexCapacity(vertexList, additionalVertices);
//     VertexSDFList_EnsureIndexCapacity(vertexList, additionalIndices);

//     // Cache z
//     float32 z = ctx->zLayer;

//     // Pack colors into uint32
//     float32 packedBorderColor = PackColorIntoFloat32(borderColor);
//     float32 packedFillColor = PackColorIntoFloat32(fillColor);

//     // Vertices
//     uint32 vertexOffset = VertexSDFList_VertexCount(vertexList);

//     // No AA needed on this flat triangle -> deep inside sentinel for sdf, unused sentinel for sdf2
//     float32 sdfInside = -1e9f;
//     float32 sdfUnused =  1e9f;

//     // Outer triangle (border)
//     vertexList->vertices[vertexOffset + 0] = (vertexSDF){ x1, y1, z, sdfInside, sdfUnused, packedBorderColor };
//     vertexList->vertices[vertexOffset + 1] = (vertexSDF){ x2, y2, z, sdfInside, sdfUnused, packedBorderColor };
//     vertexList->vertices[vertexOffset + 2] = (vertexSDF){ x3, y3, z, sdfInside, sdfUnused, packedBorderColor };

//     // Compute inward offset (very simple approximation using centroid)
//     float32 dx1 = x2 - x1, dy1 = y2 - y1;
//     float32 dx2 = x3 - x2, dy2 = y3 - y2;
//     float32 dx3 = x1 - x3, dy3 = y1 - y3;

//     float32 len1 = sqrtf(dx1 * dx1 + dy1 * dy1);
//     float32 len2 = sqrtf(dx2 * dx2 + dy2 * dy2);
//     float32 len3 = sqrtf(dx3 * dx3 + dy3 * dy3);

//     float32 area = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
//     float32 sign = (area > 0.0f) ? 1.0f : -1.0f;

//     float32 nx1 = sign * dy1 / len1, ny1 = -sign * dx1 / len1;
//     float32 nx2 = sign * dy2 / len2, ny2 = -sign * dx2 / len2;
//     float32 nx3 = sign * dy3 / len3, ny3 = -sign * dx3 / len3;

//     float32 inset = thickness;

//     float32 d1 = nx1 * x1 + ny1 * y1 - inset;
//     float32 d2 = nx2 * x2 + ny2 * y2 - inset;
//     float32 d3 = nx3 * x3 + ny3 * y3 - inset;

//     float32 det = nx1 * ny2 - ny1 * nx2;

//     float32 ix1 = (d1 * ny2 - ny1 * d2) / det;
//     float32 iy1 = (nx1 * d2 - d1 * nx2) / det;

//     float32 det2 = nx2 * ny3 - ny2 * nx3;

//     float32 ix2 = (d2 * ny3 - ny2 * d3) / det2;
//     float32 iy2 = (nx2 * d3 - d2 * nx3) / det2;

//     float32 det3 = nx3 * ny1 - ny3 * nx1;

//     float32 ix3 = (d3 * ny1 - ny3 * d1) / det3;
//     float32 iy3 = (nx3 * d1 - d3 * nx1) / det3;

//     // Inner triangle points
//     // Inner triangle (border)
//     vertexList->vertices[vertexOffset + 3] = (vertexSDF){
//         ix1, iy1, z,
//         sdfInside, sdfUnused,
//         packedBorderColor
//     };
//     vertexList->vertices[vertexOffset + 4] = (vertexSDF){
//         ix2, iy2, z,
//         sdfInside, sdfUnused,
//         packedBorderColor
//     };
//     vertexList->vertices[vertexOffset + 5] = (vertexSDF){
//         ix3, iy3, z,
//         sdfInside, sdfUnused,
//         packedBorderColor
//     };

//     // Inner triangle (fill)
//     vertexList->vertices[vertexOffset + 6] = (vertexSDF){
//         ix1, iy1, z,
//         sdfInside, sdfUnused,
//         packedFillColor
//     };
//     vertexList->vertices[vertexOffset + 7] = (vertexSDF){
//         ix2, iy2, z,
//         sdfInside, sdfUnused,
//         packedFillColor
//     };
//     vertexList->vertices[vertexOffset + 8] = (vertexSDF){
//         ix3, iy3, z,
//         sdfInside, sdfUnused,
//         packedFillColor
//     };

//     // Indices
//     uint32* indicesWrite = vertexList->indices + VertexSDFList_IndexCount(vertexList);

//     // Edge 1
//     *indicesWrite++ = vertexOffset + 0;
//     *indicesWrite++ = vertexOffset + 1;
//     *indicesWrite++ = vertexOffset + 3;
//     *indicesWrite++ = vertexOffset + 1;
//     *indicesWrite++ = vertexOffset + 3;
//     *indicesWrite++ = vertexOffset + 4;
//     // Edge 2
//     *indicesWrite++ = vertexOffset + 1;
//     *indicesWrite++ = vertexOffset + 2;
//     *indicesWrite++ = vertexOffset + 4;
//     *indicesWrite++ = vertexOffset + 2;
//     *indicesWrite++ = vertexOffset + 4;
//     *indicesWrite++ = vertexOffset + 5;
//     // Edge 3
//     *indicesWrite++ = vertexOffset + 2;
//     *indicesWrite++ = vertexOffset + 0;
//     *indicesWrite++ = vertexOffset + 5;
//     *indicesWrite++ = vertexOffset + 0;
//     *indicesWrite++ = vertexOffset + 5;
//     *indicesWrite++ = vertexOffset + 3;
//     // Center
//     *indicesWrite++ = vertexOffset + 6;
//     *indicesWrite++ = vertexOffset + 7;
//     *indicesWrite++ = vertexOffset + 8;

//     // Increment vertex and index count
//     vertexList->vertexCount += additionalVertices;
//     vertexList->indexCount += additionalIndices;
// }

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

    // Get vertexList
    VertexSDFList* vertexList = &ctx->meshLayer.vertexList;

    // Ensure vertex and index capacity
    uint32 additionalVertices = 9;
    uint32 additionalIndices = 21;
    VertexSDFList_EnsureVertexCapacity(vertexList, additionalVertices);
    VertexSDFList_EnsureIndexCapacity(vertexList, additionalIndices);

    // Cache z
    float32 z = ctx->zLayer;

    // Pack colors into uint32
    float32 packedBorderColor = PackColorIntoFloat32(borderColor);
    float32 packedFillColor = PackColorIntoFloat32(fillColor);

    // Vertices
    uint32 vertexOffset = VertexSDFList_VertexCount(vertexList);

    // No AA needed on this flat triangle -> deep inside sentinel for sdf, unused sentinel for sdf2
    float32 sdfInside = -1e9f;
    float32 sdfUnused =  1e9f;

    // Outer triangle (border)
    vertexList->vertices[vertexOffset + 0] = (vertexSDF){ x1, y1, z, sdfInside, sdfUnused, packedBorderColor };
    vertexList->vertices[vertexOffset + 1] = (vertexSDF){ x2, y2, z, sdfInside, sdfUnused, packedBorderColor };
    vertexList->vertices[vertexOffset + 2] = (vertexSDF){ x3, y3, z, sdfInside, sdfUnused, packedBorderColor };

    // Compute inward offset (very simple approximation using centroid)
    float32 dx1 = x2 - x1, dy1 = y2 - y1;
    float32 dx2 = x3 - x2, dy2 = y3 - y2;
    float32 dx3 = x1 - x3, dy3 = y1 - y3;

    float32 len1 = sqrtf(dx1 * dx1 + dy1 * dy1);
    float32 len2 = sqrtf(dx2 * dx2 + dy2 * dy2);
    float32 len3 = sqrtf(dx3 * dx3 + dy3 * dy3);

    float32 area = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
    float32 sign = (area > 0.0f) ? 1.0f : -1.0f;

    float32 nx1 = sign * dy1 / len1, ny1 = -sign * dx1 / len1;
    float32 nx2 = sign * dy2 / len2, ny2 = -sign * dx2 / len2;
    float32 nx3 = sign * dy3 / len3, ny3 = -sign * dx3 / len3;

    float32 inset = thickness;

    float32 d1 = nx1 * x1 + ny1 * y1 - inset;
    float32 d2 = nx2 * x2 + ny2 * y2 - inset;
    float32 d3 = nx3 * x3 + ny3 * y3 - inset;

    float32 det = nx1 * ny2 - ny1 * nx2;

    float32 ix1 = (d1 * ny2 - ny1 * d2) / det;
    float32 iy1 = (nx1 * d2 - d1 * nx2) / det;

    float32 det2 = nx2 * ny3 - ny2 * nx3;

    float32 ix2 = (d2 * ny3 - ny2 * d3) / det2;
    float32 iy2 = (nx2 * d3 - d2 * nx3) / det2;

    float32 det3 = nx3 * ny1 - ny3 * nx1;

    float32 ix3 = (d3 * ny1 - ny3 * d1) / det3;
    float32 iy3 = (nx3 * d1 - d3 * nx1) / det3;

    // Inner triangle points
    // Inner triangle (border)
    vertexList->vertices[vertexOffset + 3] = (vertexSDF){
        ix1, iy1, z,
        sdfInside, sdfUnused,
        packedBorderColor
    };
    vertexList->vertices[vertexOffset + 4] = (vertexSDF){
        ix2, iy2, z,
        sdfInside, sdfUnused,
        packedBorderColor
    };
    vertexList->vertices[vertexOffset + 5] = (vertexSDF){
        ix3, iy3, z,
        sdfInside, sdfUnused,
        packedBorderColor
    };

    // Inner triangle (fill)
    vertexList->vertices[vertexOffset + 6] = (vertexSDF){
        ix1, iy1, z,
        sdfInside, sdfUnused,
        packedFillColor
    };
    vertexList->vertices[vertexOffset + 7] = (vertexSDF){
        ix2, iy2, z,
        sdfInside, sdfUnused,
        packedFillColor
    };
    vertexList->vertices[vertexOffset + 8] = (vertexSDF){
        ix3, iy3, z,
        sdfInside, sdfUnused,
        packedFillColor
    };

    // Indices
    uint32* indicesWrite = vertexList->indices + VertexSDFList_IndexCount(vertexList);

    // Edge 1
    *indicesWrite++ = vertexOffset + 0;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 3;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 3;
    *indicesWrite++ = vertexOffset + 4;
    // Edge 2
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 4;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 4;
    *indicesWrite++ = vertexOffset + 5;
    // Edge 3
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 0;
    *indicesWrite++ = vertexOffset + 5;
    *indicesWrite++ = vertexOffset + 0;
    *indicesWrite++ = vertexOffset + 5;
    *indicesWrite++ = vertexOffset + 3;
    // Center
    *indicesWrite++ = vertexOffset + 6;
    *indicesWrite++ = vertexOffset + 7;
    *indicesWrite++ = vertexOffset + 8;

    // Increment vertex and index count
    vertexList->vertexCount += additionalVertices;
    vertexList->indexCount += additionalIndices;
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

    // Get vertexList
    VertexSDFList* vertexList = &ctx->meshLayer.vertexList;

    // Ensure vertex and index capacity
    uint32 additionalVertices = 4;
    uint32 additionalIndices = 6;
    VertexSDFList_EnsureVertexCapacity(vertexList, additionalVertices);
    VertexSDFList_EnsureIndexCapacity(vertexList, additionalIndices);

    // Cache z
    float32 z = ctx->zLayer;

    // Add pixel alignment offset
    x += 0.5f;
    float32 half_thick = thickness * 0.5f;

    // Pack color into float32
    float32 packedColor = PackColorIntoFloat32(color);

    // SDF: vertical line -> only the two long edges (left/right) need AA.
    // sdf = distance to nearest long edge (negative inside, positive outside)
    // sdf2 = unused (1e9 sentinel), so min(sdf, sdf2) == sdf
    float32 sdfLeft  = -half_thick;   // on left edge, negative = inside
    float32 sdfRight =  half_thick;   // on right edge, positive = outside
    float32 sdfUnused = 1e9f;

    // Vertices
    uint32 vertexOffset = VertexSDFList_VertexCount(vertexList);
    vertexList->vertices[vertexOffset]     = (vertexSDF){ x - half_thick, y,          z, sdfLeft,  sdfUnused, packedColor }; // Bottom left
    vertexList->vertices[vertexOffset + 1] = (vertexSDF){ x + half_thick, y,          z, sdfRight, sdfUnused, packedColor }; // Bottom right
    vertexList->vertices[vertexOffset + 2] = (vertexSDF){ x - half_thick, y + height, z, sdfLeft,  sdfUnused, packedColor }; // Top left
    vertexList->vertices[vertexOffset + 3] = (vertexSDF){ x + half_thick, y + height, z, sdfRight, sdfUnused, packedColor }; // Top right

    // Indices
    uint32* indicesWrite = vertexList->indices + VertexSDFList_IndexCount(vertexList);
    *indicesWrite++ = vertexOffset;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 3;

    // Increment vertex and index count
    vertexList->vertexCount += additionalVertices;
    vertexList->indexCount += additionalIndices;
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

    // Get vertexList
    VertexSDFList* vertexList = &ctx->meshLayer.vertexList;

    // Ensure vertex and index capacity
    uint32 additionalVertices = 4;
    uint32 additionalIndices = 6;
    VertexSDFList_EnsureVertexCapacity(vertexList, additionalVertices);
    VertexSDFList_EnsureIndexCapacity(vertexList, additionalIndices);

    // Cache z
    float32 z = ctx->zLayer;

    // Add pixel alignment offset
    y += 0.5f;
    float32 half_thick = thickness * 0.5f;

    // Pack color into uint32
    float32 packedColor = PackColorIntoFloat32(color);

    // No AA needed on axis-aligned line -> deep inside sentinel for sdf, unused sentinel for sdf2
    float32 sdfInside = -1e9f;
    float32 sdfUnused =  1e9f;

    // Vertices
    uint32 vertexOffset = VertexSDFList_VertexCount(vertexList);
    vertexList->vertices[vertexOffset] = (vertexSDF){ x, y - half_thick, z, sdfInside, sdfUnused, packedColor }; // Left point (x) - thick -
    vertexList->vertices[vertexOffset + 1] = (vertexSDF){ x, y + half_thick, z, sdfInside, sdfUnused, packedColor }; // Left point (x) - thick +
    vertexList->vertices[vertexOffset + 2] = (vertexSDF){ x + width, y - half_thick, z, sdfInside, sdfUnused, packedColor }; // Right point (x1) - thick -
    vertexList->vertices[vertexOffset + 3] = (vertexSDF){ x + width, y + half_thick, z, sdfInside, sdfUnused, packedColor }; // Right point (x1) - thick +

    // Indices
    uint32* indicesWrite = vertexList->indices + VertexSDFList_IndexCount(vertexList);
    *indicesWrite++ = vertexOffset;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 3;

    // Increment vertex and index count
    vertexList->vertexCount += additionalVertices;
    vertexList->indexCount += additionalIndices;
}

void Canvas_DrawLine(
    NU_Canvas_Context* ctx,
    float32 x1, float32 y1, float32 x2, float32 y2,
    float32 thickness,
    NuColor color)
{
    if (thickness <= 0.0f) return;

    float32 twoThick = thickness * 2.0f;
    float32 canvasWidth = ctx->canvasWidth;
    float32 canvasHeight = ctx->canvasHeight;

    // Skip if line is not visible on canvas.
    if ((x1 < -twoThick && x2 < -twoThick) ||
        (x1 > canvasWidth && x2 > canvasWidth) ||
        (y1 < -twoThick && y2 < -twoThick) ||
        (y1 > canvasHeight + twoThick && y2 > canvasHeight + twoThick))
        return;

    bool coordsWithinLargeVirtualCanvas =
        x1 >= -twoThick && x1 <= canvasWidth + twoThick &&
        x2 >= -twoThick && x2 <= canvasWidth + twoThick &&
        y1 >= -twoThick && y1 <= canvasHeight + twoThick &&
        y2 >= -twoThick && y2 <= canvasHeight + twoThick;

    // Clip against an expanded canvas.
    if (!coordsWithinLargeVirtualCanvas)
    {
        const float32 MINX = -twoThick;
        const float32 MINY = -twoThick;
        const float32 MAXX = canvasWidth + twoThick * 2.0f;
        const float32 MAXY = canvasHeight + twoThick * 2.0f;
        float32 dx = x2 - x1;
        float32 dy = y2 - y1;
        float32 t0 = 0.0f;
        float32 t1 = 1.0f;

        // Left
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

        // Right
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

        // Bottom
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

        // Top
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

        if (t0 > t1) return;
        float32 ox = x1;
        float32 oy = y1;
        x1 = ox + dx * t0;
        y1 = oy + dy * t0;
        x2 = ox + dx * t1;
        y2 = oy + dy * t1;
    }

    // Pixel alignment
    x1 += 0.5f;
    x2 += 0.5f;
    y1 += 0.5f;
    y2 += 0.5f;
    float32 dx = x2 - x1;
    float32 dy = y2 - y1;
    float32 length = sqrtf(dx * dx + dy * dy);
    if (length < 0.0001f) return;
    dx /= length;
    dy /= length;

    // Normal perpendicular to the line
    float32 px = -dy;
    float32 py = dx;
    float32 halfThick = thickness * 0.5f;

    // Extra geometry beyond the real line boundary
    // Must match the distance range handled by the shader
    const float32 aaPadding = 1.5f;
    float32 extent = halfThick + aaPadding;

    // Switching from text layer to mesh layer
    if (!ctx->isMeshLayer) {
        ctx->isMeshLayer = true;
        ctx->zLayer += 1.0f;
    }

    // Get vertex list
    VertexSDFList* vertexList = &ctx->meshLayer.vertexList;

    // Ensure vertex and index capacity
    const uint32 additionalVertices = 8;
    const uint32 additionalIndices = 12;
    VertexSDFList_EnsureVertexCapacity(vertexList, additionalVertices);
    VertexSDFList_EnsureIndexCapacity(vertexList, additionalIndices);

    // Cache z
    float32 z = ctx->zLayer;

    // Pack color into uint32
    float32 packedColor = PackColorIntoFloat32(color);

    // The SDF is measured from the ACTUAL line boundary.
    //
    // At the expanded mesh boundary: +aaPadding
    // At the real line boundary:       0
    // At the centreline:              -halfThick
    //
    // Each half has a linear SDF interpolation
    const float32 sdfUnused = 1e9f;
    const float32 sdfOuter = aaPadding;
    const float32 sdfCenter = -halfThick;

    uint32 vertexOffset = VertexSDFList_VertexCount(vertexList);

    // Left half: expanded outer edge -> centreline
    vertexList->vertices[vertexOffset] = (vertexSDF){ x1 - px * extent, y1 - py * extent, z, sdfOuter, sdfUnused, packedColor };
    vertexList->vertices[vertexOffset + 1] = (vertexSDF){ x1, y1, z, sdfCenter, sdfUnused, packedColor };
    vertexList->vertices[vertexOffset + 2] = (vertexSDF){ x2 - px * extent, y2 - py * extent, z, sdfOuter, sdfUnused, packedColor };
    vertexList->vertices[vertexOffset + 3] = (vertexSDF){ x2, y2, z, sdfCenter, sdfUnused, packedColor };

    // Right half: centreline -> expanded outer edge
    vertexList->vertices[vertexOffset + 4] = (vertexSDF){ x1, y1, z, sdfCenter, sdfUnused, packedColor };
    vertexList->vertices[vertexOffset + 5] = (vertexSDF){ x1 + px * extent, y1 + py * extent, z, sdfOuter, sdfUnused, packedColor };
    vertexList->vertices[vertexOffset + 6] = (vertexSDF){ x2, y2, z, sdfCenter, sdfUnused, packedColor };
    vertexList->vertices[vertexOffset + 7] = (vertexSDF){ x2 + px * extent, y2 + py * extent, z, sdfOuter, sdfUnused, packedColor };

    // Indices
    uint32* indicesWrite = vertexList->indices + VertexSDFList_IndexCount(vertexList);

    // Left half
    *indicesWrite++ = vertexOffset;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 1;
    *indicesWrite++ = vertexOffset + 2;
    *indicesWrite++ = vertexOffset + 3;
    // Right half
    *indicesWrite++ = vertexOffset + 4;
    *indicesWrite++ = vertexOffset + 5;
    *indicesWrite++ = vertexOffset + 6;
    *indicesWrite++ = vertexOffset + 5;
    *indicesWrite++ = vertexOffset + 6;
    *indicesWrite++ = vertexOffset + 7;

    // Increment vertex and index count
    vertexList->vertexCount += additionalVertices;
    vertexList->indexCount += additionalIndices;
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

    // Get vertexList
    VertexSDFList* vertexList = &ctx->meshLayer.vertexList;

    // Ensure vertex and index capacity
    VertexSDFList_EnsureVertexCapacity(vertexList, min_additional_vertices);
    VertexSDFList_EnsureIndexCapacity(vertexList, min_additional_indices);

    // Cache z
    float32 z = ctx->zLayer;

    // Generate the mesh
    float32 travelled = world_start;
    uint32 i = start_pattern_index;
    float32 remaining_in_segment = (float32)dash_pattern[i] - (phase - pattern_offset);
    float32 half_thick = thickness * 0.5f;

    // Pre-calculate perpendicular direction (constant for the whole line)
    float32 perp_x = -step_y * half_thick;
    float32 perp_y =  step_x * half_thick;

    // Pack color into uint32
    float32 packedColor = PackColorIntoFloat32(color);

    // No AA needed for now -> deep inside sentinel for sdf, unused sentinel for sdf2
    float32 sdfInside = -1e9f;
    float32 sdfUnused =  1e9f;

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

            uint32 v0 = VertexSDFList_VertexCount(vertexList);
            vertexSDF* v = vertexList->vertices + v0;
            v[0] = (vertexSDF){ sx1 - perp_x, sy1 - perp_y, z, sdfInside, sdfUnused, packedColor };
            v[1] = (vertexSDF){ sx1 + perp_x, sy1 + perp_y, z, sdfInside, sdfUnused, packedColor };
            v[2] = (vertexSDF){ sx2 - perp_x, sy2 - perp_y, z, sdfInside, sdfUnused, packedColor };
            v[3] = (vertexSDF){ sx2 + perp_x, sy2 + perp_y, z, sdfInside, sdfUnused, packedColor };

            uint32* id = vertexList->indices + VertexSDFList_IndexCount(vertexList);
            id[0] = v0;
            id[1] = v0 + 1;
            id[2] = v0 + 2;
            id[3] = v0 + 1;
            id[4] = v0 + 2;
            id[5] = v0 + 3;

            // Increment vertex and index count
            vertexList->vertexCount += 4;
            vertexList->indexCount += 6;
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
