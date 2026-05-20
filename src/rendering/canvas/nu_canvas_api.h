#pragma once
#include <rendering/nu_renderer_structures.h>
#include <text/nu_text_layout.h>
#include <math.h>

#define NORMALIZE(dx, dy) do { float len = sqrtf((dx)*(dx) + (dy)*(dy)); if (len > 0.0001f) { dx /= len; dy /= len; } } while(0)

int NU_Internal_Get_Canvas_Context(Node* node)
{
    NodeP* nodeP = NODEP_OF(node);
    if (nodeP->type != NU_CANVAS) return -1;
    if (nodeP->typeData.canvas.ctxHandle != -1) return nodeP->typeData.canvas.ctxHandle;

    // Create a new canvas ctx
    NU_Canvas_Context ctx;
    ctx.isShapeLayer = true; 
    ctx.fontID = 0;
    ctx.z = 0;
    ctx.textLayerIndex = 0;
    ctx.node = nodeP;
    ArrayInit(&ctx.textLayers, sizeof(CanvasTextLayer), 4);

    // Create default text layer
    CanvasTextLayer textLayer;
    textLayer.fontID = 0;
    Vertex_RGB_UV_List_Init(&textLayer.vertices, 256);
    Index_List_Init(&textLayer.indices, 512);
    ArrayPush(&ctx.textLayers, &textLayer);

    // Create shape layer
    Vertex_RGB_List_Init(&ctx.shapeLayer.vertices, 1024);
    Index_List_Init(&ctx.shapeLayer.indices, 2048);

    // Add ctx
    int ctxId = Container_Add(&GUI.canvasContexts, &ctx);
    nodeP->typeData.canvas.ctxHandle = ctxId;
    return ctxId;
}

void NU_DeleteCanvasContext(int contextID)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    // Free vertices and indices of each layer
    for (u32 i=0; i<ctx->textLayers.size; i++) {
        CanvasTextLayer* layer = ArrayGet(&ctx->textLayers, i);
        Vertex_RGB_UV_List_Free(&layer->vertices);
        Index_List_Free(&layer->indices);
    }
    ArrayFree(&ctx->textLayers);

    // Free vertices and indices of shape layer
    Vertex_RGB_List_Free(&ctx->shapeLayer.vertices);
    Index_List_Free(&ctx->shapeLayer.indices);

    // Remove ctx
    Container_Remove(&GUI.canvasContexts, contextID);
}

void NU_Internal_Clear_Canvas(int contextID)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    // Clear vertices and indices of each text layer (except layer 0)
    for (u32 i=0; i<ctx->textLayers.size; i++) {
        CanvasTextLayer* layer = ArrayGet(&ctx->textLayers, i);
        Vertex_RGB_UV_List_Clear(&layer->vertices);
        Index_List_Clear(&layer->indices);
        layer->fontID = 0;
    }

    // Clear vertices and indices of shape layer
    Vertex_RGB_List_Clear(&ctx->shapeLayer.vertices);
    Index_List_Clear(&ctx->shapeLayer.indices);

    // Reset state
    ctx->fontID = 0;
    ctx->z = 0;
    ctx->textLayerIndex = 0;
    ctx->isShapeLayer = true;
}

void NU_Internal_Border_Rect(
    int contextID,
    float x, float y, float w, float h, 
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    // Constrain dimensions based on thickness
    w = fmaxf(w, thickness * 2);
    h = fmaxf(h, thickness * 2);

    // Skip function if rect is not visible
    if (x + w < 0.0f || x > ctx->canvasWidth || y + h < 0.0f || y > ctx->canvasHeight) return;

    // Switching from text -> shape, increase depth
    if (!ctx->isShapeLayer) {
        ctx->isShapeLayer = true;
        ctx->z++;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->shapeLayer.vertices;
    Index_List* indices = &ctx->shapeLayer.indices;

    // --- Allocate extra space in vertex and index lists ---
    u32 additional_vertices = 12;    
    u32 additional_indices = 30;          
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    u32 vertex_offset = vertices->size;

    float z = (float)(ctx->node->layer) + ctx->z * 0.005f;

    // Outer TL
    vertices->array[vertex_offset] = (vertex_rgb){ x, y, z, border_col.r, border_col.g, border_col.b };

    // Outer TR
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x + w, y, z, border_col.r, border_col.g, border_col.b };

    // Outer BL
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x, y + h, z, border_col.r, border_col.g, border_col.b };

    // Outer BR
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x + w, y + h, z, border_col.r, border_col.g, border_col.b };

    // Inner TL
    vertices->array[vertex_offset + 4] = (vertex_rgb){ x + thickness, y + thickness, z, border_col.r, border_col.g, border_col.b };

    // Inner TR
    vertices->array[vertex_offset + 5] = (vertex_rgb){ x + w - thickness, y + thickness, z, border_col.r, border_col.g, border_col.b };

    // Inner BL
    vertices->array[vertex_offset + 6] = (vertex_rgb){ x + thickness, y + h - thickness, z, border_col.r, border_col.g, border_col.b };

    // Inner BR
    vertices->array[vertex_offset + 7] = (vertex_rgb){ x + w - thickness, y + h - thickness, z, border_col.r, border_col.g, border_col.b };

    // Fill TL
    vertices->array[vertex_offset + 8] = (vertex_rgb){ x + thickness, y + thickness, z, fill_col.r, fill_col.g, fill_col.b };

    // Fill TR
    vertices->array[vertex_offset + 9] = (vertex_rgb){ x + w - thickness, y + thickness, z, fill_col.r, fill_col.g, fill_col.b };

    // Fill BL
    vertices->array[vertex_offset + 10] = (vertex_rgb){ x + thickness, y + h - thickness, z, fill_col.r, fill_col.g, fill_col.b };

    // Fill BR
    vertices->array[vertex_offset + 11] = (vertex_rgb){ x + w - thickness, y + h - thickness, z, fill_col.r, fill_col.g, fill_col.b };

    // Indices
    u32* indices_write = indices->array + indices->size;

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

void NU_Internal_Triangle(
    int contextID,
    float x1, float y1, 
    float x2, float y2, 
    float x3, float y3,
    float thickness,
    NU_RGB border_col,
    NU_RGB fill_col)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    // Skip if triangle is offscreen
    float minX = fminf(x1, fminf(x2, x3));
    float maxX = fmaxf(x1, fmaxf(x2, x3));
    float minY = fminf(y1, fminf(y2, y3));
    float maxY = fmaxf(y1, fmaxf(y2, y3));
    if (maxX < 0.0f || minX > ctx->canvasWidth || maxY < 0.0f || minY > ctx->canvasHeight) return;

    // Switching from text -> shape, increase depth
    if (!ctx->isShapeLayer) {
        ctx->isShapeLayer = true;
        ctx->z++;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->shapeLayer.vertices;
    Index_List* indices = &ctx->shapeLayer.indices;

    float z = (float)(ctx->node->layer) + ctx->z * 0.005f;

    // --- Allocate extra space in vertex and index lists ---
    u32 additional_vertices = 6;    
    u32 additional_indices = 21;          
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    u32 vertex_offset = vertices->size;
    
    // --- Outer triangle (border) ---
    vertices->array[vertex_offset + 0] = (vertex_rgb){ x1, y1, border_col.r, border_col.g, border_col.b };
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x2, y2, border_col.r, border_col.g, border_col.b };
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x3, y3, border_col.r, border_col.g, border_col.b };

    // Compute inward offset (very simple approximation using centroid)
    float cx = (x1 + x2 + x3) / 3.0f;
    float cy = (y1 + y2 + y3) / 3.0f;

    // Inner triangle points
    float dx1 = cx - x1, dy1 = cy - y1; NORMALIZE(dx1, dy1);
    float dx2 = cx - x2, dy2 = cy - y2; NORMALIZE(dx2, dy2);
    float dx3 = cx - x3, dy3 = cy - y3; NORMALIZE(dx3, dy3);

    // --- Inner triangle (fill) ---
    vertices->array[vertex_offset + 3] = (vertex_rgb){
        x1 + dx1 * thickness, y1 + dy1 * thickness, z,
        fill_col.r, fill_col.g, fill_col.b
    };
    vertices->array[vertex_offset + 4] = (vertex_rgb){
        x2 + dx2 * thickness, y2 + dy2 * thickness, z,
        fill_col.r, fill_col.g, fill_col.b
    };
    vertices->array[vertex_offset + 5] = (vertex_rgb){
        x3 + dx3 * thickness, y3 + dy3 * thickness, z,
        fill_col.r, fill_col.g, fill_col.b
    };

    // --- Indices ---
    u32* indices_write = indices->array + indices->size;

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

void NU_Internal_Vline(int contextID, float x, float y, float height, float thickness, NU_RGB col)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    float canvasWidth = ctx->canvasWidth;
    float canvasHeight = ctx->canvasHeight;

    // Skip if line is not visible on canvas
    if (x < -thickness || x > canvasWidth + thickness || y > canvasHeight || y + height < 0.0f) return;

    // Constrain to avoid large numbers
    if (y < 0.0f) y = 0.0f;
    if (y + height > canvasHeight) height = canvasHeight - y;

    // Switching from text -> shape, increase depth
    if (!ctx->isShapeLayer) {
        ctx->isShapeLayer = true;
        ctx->z++;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->shapeLayer.vertices;
    Index_List* indices = &ctx->shapeLayer.indices;

    float z = (float)(ctx->node->layer) + ctx->z * 0.005f;

    // --- Allocate extra space in vertex and index lists ---
    u32 additional_vertices = 4;    
    u32 additional_indices = 6;          
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    // Add pixel alignment offset
    x += 0.5f;
    float half_thick = thickness * 0.5f;
 
    // Create mesh
    u32 vertex_offset = vertices->size;
    vertices->array[vertex_offset] = (vertex_rgb){ x - half_thick, y, z, col.r, col.g, col.b }; // Bottom point (y) - thick -
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x + half_thick, y, z, col.r, col.g, col.b }; // Bottom point (y) - thick + 
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x - half_thick, y + height, z, col.r, col.g, col.b }; // Top point (y1) - thick -
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x + half_thick, y + height, z, col.r, col.g, col.b }; // Top point (y1) - thick + 

    // Indices
    u32* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void NU_Internal_Hline(int contextID, float x, float y, float width, float thickness, NU_RGB col)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    float canvasWidth = ctx->canvasWidth;
    float canvasHeight = ctx->canvasHeight;

    // Skip if line is not visible on canvas
    if (y < -thickness || y > canvasHeight + thickness || x > canvasWidth || x + width < 0.0f) return;

    // Constrain to avoid large numbers
    if (x < 0.0f) x = 0.0f;
    if (x + width > canvasWidth) width = canvasWidth - x;

    // Switching from text -> shape, increase depth
    if (!ctx->isShapeLayer) {
        ctx->isShapeLayer = true;
        ctx->z++;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->shapeLayer.vertices;
    Index_List* indices = &ctx->shapeLayer.indices;

    float z = (float)(ctx->node->layer) + ctx->z * 0.005f;

    // --- Allocate extra space in vertex and index lists ---
    u32 additional_vertices = 4;    
    u32 additional_indices = 6;          
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    // Add pixel alignment offset
    y += 0.5f;
    float half_thick = thickness * 0.5f;
    
    // Create mesh
    u32 vertex_offset = vertices->size;
    vertices->array[vertex_offset] = (vertex_rgb){ x, y - half_thick, z, col.r, col.g, col.b }; // Left point (x) - thick -
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x, y + half_thick, z, col.r, col.g, col.b }; // Left point (x) - thick + 
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x + width, y - half_thick, z, col.r, col.g, col.b }; // Right point (x1) - thick -
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x + width, y + half_thick, z, col.r, col.g, col.b }; // Right point (x1) - thick + 

    // Indices
    u32* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void NU_Internal_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB col
)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    float twoThick = thickness * 2.0f;
    float canvasWidth = ctx->canvasWidth;
    float canvasHeight = ctx->canvasHeight;

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
        const float MINX = -twoThick, MINY = -twoThick;
        const float MAXX = ctx->canvasWidth + twoThick * 2.0f, MAXY = ctx->canvasHeight + twoThick * 2.0f;
        float dx = x2 - x1;
        float dy = y2 - y1;
        float t0 = 0.0f;
        float t1 = 1.0f;

        // left
        float p = -dx, q = x1 - MINX;
        if (p == 0.0f) {
            if (q < 0.0f) return;
        } else {
            float r = q / p;
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
            float r = q / p;
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
            float r = q / p;
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
            float r = q / p;
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
        float nx1 = x1 + dx * t0;
        float ny1 = y1 + dy * t0;
        float nx2 = x1 + dx * t1;
        float ny2 = y1 + dy * t1;
        x1 = nx1; y1 = ny1;
        x2 = nx2; y2 = ny2;
    }

    // Switching from text -> shape, increase depth
    if (!ctx->isShapeLayer) {
        ctx->isShapeLayer = true;
        ctx->z++;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->shapeLayer.vertices;
    Index_List* indices = &ctx->shapeLayer.indices;

    float z = (float)(ctx->node->layer) + ctx->z * 0.005f;

    // --- Allocate extra space in vertex and index lists ---
    u32 additional_vertices = 4;    
    u32 additional_indices = 6;          
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    x1 += 0.5f;
    x2 += 0.5f;
    y1 += 0.5f;
    y2 += 0.5f;


    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    if (length == 0.0f) return; // avoid divide-by-zero
    dx /= length;
    dy /= length;
    float px = -dy;
    float py = dx;
    float half_thick = thickness * 0.5f;
    px *= half_thick;
    py *= half_thick;


    u32 vertex_offset = vertices->size;
    vertices->array[vertex_offset] = (vertex_rgb){ x1 - px, y1 - py, z, col.r, col.g, col.b }; // V1 -> thick -
    vertices->array[vertex_offset + 1] = (vertex_rgb){ x1 + px, y1 + py, z, col.r, col.g, col.b }; // V1 -> thick + 
    vertices->array[vertex_offset + 2] = (vertex_rgb){ x2 - px, y2 - py, z, col.r, col.g, col.b }; // V2 -> thick -
    vertices->array[vertex_offset + 3] = (vertex_rgb){ x2 + px, y2 + py, z, col.r, col.g, col.b }; // V2 -> thick + 

    // Indices
    u32* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}

void NU_Internal_Dashed_Line(
    int contextID,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    u32 dash_pattern_len,
    NU_RGB col
)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    float twoThick = thickness * 2.0f;
    float canvasWidth = ctx->canvasWidth;
    float canvasHeight = ctx->canvasHeight;

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

    float ox1 = x1, oy1 = y1;
    float ox2 = x2, oy2 = y2;
    float t_start = 0.0f;
    float t_end   = 1.0f;

    if (!coordsWithinLargeVirtualCanvas)
    {
        const float MINX = -twoThick, MINY = -twoThick;
        const float MAXX = ctx->canvasWidth + twoThick * 2.0f, MAXY = ctx->canvasHeight + twoThick * 2.0f;
        float dx = x2 - x1;
        float dy = y2 - y1;
        float t0 = 0.0f;
        float t1 = 1.0f;

        // LEFT
        {
            float p = -dx, q = x1 - MINX;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        // RIGHT
        {
            float p = dx, q = MAXX - x1;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        // BOTTOM
        {
            float p = -dy, q = y1 - MINY;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        // TOP
        {
            float p = dy, q = MAXY - y1;
            if (p == 0.0f) { if (q < 0.0f) return; }
            else {
                float r = q / p;
                if (p < 0.0f) { if (r > t1) return; if (r > t0) t0 = r; }
                else          { if (r < t0) return; if (r < t1) t1 = r; }
            }
        }

        if (t0 > t1) return;

        // Calculate new clipped endpoints
        float cx1 = x1 + (x2 - x1) * t0;
        float cy1 = y1 + (y2 - y1) * t0;
        float cx2 = x1 + (x2 - x1) * t1;
        float cy2 = y1 + (y2 - y1) * t1;
        
        // Calculate the fractional positions along the original line
        float orig_dx = ox2 - ox1;
        float orig_dy = oy2 - oy1;
        float orig_len2 = orig_dx * orig_dx + orig_dy * orig_dy;
        
        if (orig_len2 == 0.0f) return;
        
        float inv_orig_len2 = 1.0f / orig_len2;
        
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
            float temp = t_start;
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

    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    if (length == 0.0f) return;

    float inv_len = 1.0f / length;
    float ox_dx = ox2 - ox1;
    float ox_dy = oy2 - oy1;
    float orig_len = sqrtf(ox_dx * ox_dx + ox_dy * ox_dy);

    if (orig_len == 0.0f) return;

    // Pre-calculate pattern total length
    float pattern_total_len = 0.0f;
    for (u32 i = 0; i < dash_pattern_len; i++) {
        pattern_total_len += (float)dash_pattern[i];
    }
    if (pattern_total_len == 0.0f) return;

    // Calculate the phase at the start of the clipped line
    float world_start = t_start * orig_len;
    float world_end   = t_end   * orig_len;
    
    // Find the starting offset within the pattern
    float phase = fmodf(world_start, pattern_total_len);
    u32 start_pattern_index = 0;
    float pattern_offset = 0.0f;
    
    for (u32 i = 0; i < dash_pattern_len; i++) {
        float segment_len = (float)dash_pattern[i];
        if (phase < pattern_offset + segment_len) {
            start_pattern_index = i;
            break;
        }
        pattern_offset += segment_len;
    }
    
    float step_x = dx * inv_len;
    float step_y = dy * inv_len;


    // Switching from text -> shape, increase depth
    if (!ctx->isShapeLayer) {
        ctx->isShapeLayer = true;
        ctx->z++;
    }

    // Get vertex and index lists
    Vertex_RGB_List* vertices = &ctx->shapeLayer.vertices;
    Index_List* indices = &ctx->shapeLayer.indices;

    float z = (float)(ctx->node->layer) + ctx->z * 0.005f;

    // Calculate number of vertices and indices are needed
    float pattern_len = 0.0f;
    u32 vertices_per_pattern = 0;
    u32 indices_per_pattern = 0;
    for (u32 i = 0; i < dash_pattern_len; i++) {
        pattern_len += dash_pattern[i];
        vertices_per_pattern += (dash_pattern[i] * ((i % 2) == 0)) * 4;
        indices_per_pattern += (dash_pattern[i] * ((i % 2) == 0)) * 6;
    }
    float approx_cycles = length / pattern_len;
    u32 min_additional_vertices = ((u32)approx_cycles + 1) * vertices_per_pattern;
    u32 min_additional_indices  = ((u32)approx_cycles + 1) * indices_per_pattern;
    if (vertices->size + min_additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, min_additional_vertices);
    if (indices->size + min_additional_indices > indices->capacity) Index_List_Grow(indices, min_additional_indices);

    // Generate the mesh
    float travelled = world_start;
    u32 i = start_pattern_index;
    float remaining_in_segment = (float)dash_pattern[i] - (phase - pattern_offset);
    float half_thick = thickness * 0.5f;  // FIXED: was 0.25f
    
    // Pre-calculate perpendicular direction (constant for the whole line)
    float perp_x = -step_y * half_thick;
    float perp_y =  step_x * half_thick;
    
    while (travelled < world_end)
    {
        float segment_len = remaining_in_segment;
        float next = travelled + segment_len;
        
        if (next > world_end) {
            segment_len = world_end - travelled;
            next = world_end;
        }
        
        if (i % 2 == 0)
        {
            // Draw dash segment
            float sx1 = x1 + step_x * (travelled - world_start);
            float sy1 = y1 + step_y * (travelled - world_start);
            float sx2 = x1 + step_x * (next - world_start);
            float sy2 = y1 + step_y * (next - world_start);
            
            u32 v0 = vertices->size;
            vertex_rgb* v = vertices->array + v0;
            v[0] = (vertex_rgb){ sx1 - perp_x, sy1 - perp_y, z, col.r, col.g, col.b };
            v[1] = (vertex_rgb){ sx1 + perp_x, sy1 + perp_y, z, col.r, col.g, col.b };
            v[2] = (vertex_rgb){ sx2 - perp_x, sy2 - perp_y, z, col.r, col.g, col.b };
            v[3] = (vertex_rgb){ sx2 + perp_x, sy2 + perp_y, z, col.r, col.g, col.b };
            
            u32* id = indices->array + indices->size;
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
        remaining_in_segment = (float)dash_pattern[i];
        pattern_offset = phase;
    }
}

void NU_Internal_Set_Canvas_Font(int contextID, const char* fontName)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    // Get fontID
    void* found = LinearStringmapGet(&GUI.stylesheet.fontNameIndexMap, fontName);
    int fontID = *(int*)found;

    // If already using that font -> return early
    if (ctx->fontID == fontID) return;

    // Update fontID in use
    ctx->fontID = fontID;
    ctx->textLayerIndex++;

    // Create new text layer
    if (ctx->textLayerIndex > ctx->textLayers.size-1) {
        CanvasTextLayer textLayer;
        textLayer.fontID = fontID;
        Vertex_RGB_UV_List_Init(&textLayer.vertices, 512);
        Index_List_Init(&textLayer.indices, 1024);
        ArrayPush(&ctx->textLayers, &textLayer);
    }
    // Reuse exising layer
    else {
        CanvasTextLayer* existingLayer = ArrayGet(&ctx->textLayers, ctx->textLayerIndex);
        existingLayer->fontID = fontID;
    }
}

void NU_Internal_Text(
    int contextID, 
    float x, 
    float y, 
    float wrapWidth, 
    NU_RGB col,
    const char* string)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return;

    // Skip if text is not visible on large virtual canvas
    NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, ctx->fontID);
    float textWidth = NU_Calculate_Text_Unwrapped_Width(font, string); if (textWidth > wrapWidth) textWidth = wrapWidth;
    float textHeight = NU_Calculate_FreeText_Height_From_Wrap_Width(font, string, wrapWidth);
    if (x > ctx->canvasWidth || x + textWidth < 0.0f || y > ctx->canvasHeight || y + textHeight < 0.0f) return;

    // Switching from shape -> text, increase depth
    if (ctx->isShapeLayer) {
        ctx->isShapeLayer = false;
        ctx->z++;
    }

    // Get text layer
    CanvasTextLayer* textLayer = ArrayGet(&ctx->textLayers, ctx->textLayerIndex);

    // Get vertex and index lists
    Vertex_RGB_UV_List* vertices = &textLayer->vertices;
    Index_List* indices = &textLayer->indices;

    float z = (float)(ctx->node->layer) + ctx->z * 0.005f;

    // Generate text mesh
    NU_Generate_Text_Mesh(vertices, indices, font, string, x, y, z, col.r, col.g, col.b, wrapWidth);
}

float NU_Internal_Text_Height(int contextID, float wrapWidth, const char* string)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return 0.0f; // node type is not valid therefore there is no context

    NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, ctx->fontID);
    return NU_Calculate_FreeText_Height_From_Wrap_Width(font, string, wrapWidth);
}

float NU_Internal_Text_Width(int contextID, const char* string)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return 0.0f; // node type is not valid therefore there is no context

    NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, ctx->fontID);
    return NU_Calculate_Text_Unwrapped_Width(font, string);
}

float NU_Internal_Text_Line_Height(int contextID)
{
    NU_Canvas_Context* ctx = Container_Get(&GUI.canvasContexts, contextID); 
    if (ctx == NULL) return 0.0f; // node type is not valid therefore there is no context
    NU_Font* font = Stylesheet_Get_Font(&GUI.stylesheet, ctx->fontID);
    return font->line_height;
}