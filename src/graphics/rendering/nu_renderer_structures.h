#pragma once
#include <math.h>

typedef struct {
    float32 x, y;
} vec2;

typedef struct {
    float32 x, y, z;
    float32 packedColor;
} vertex_rgb;

typedef struct {
    uint32* array;
    uint32 size;
    uint32 capacity;
} Index_List;

typedef struct {
    vertex_rgb* array;
    uint32 size;
    uint32 capacity;
} Vertex_RGB_List;

typedef struct BorderRectRenderData {
    float32 x, y, z;
    float32 w, h;
    uint32 backgroundRGBA;
    uint32 borderRGBA;
    float32 radiusTl, radiusTr, radiusBl, radiusBr;
    float32 borderTop, borderBottom, borderLeft, borderRight;
    float32 scissorTop, scissorBottom, scissorLeft, scissorRight;
} BorderRectRenderData;

typedef struct GlyphRenderData {
    float32 x, y, z;
    float32 w, h;
    float32 u0, v0;
    float32 u1, v1;
    uint32 colourRGBA;
    uint32 textureIndex;
} GlyphRenderData;

typedef struct ClippedGlyphRenderData {
    float32 x, y, z;
    float32 w, h;
    float32 u0, v0;
    float32 u1, v1;
    uint16 clipTop, clipBottom;
    uint16 clipLeft, clipRight;
    uint8 clipRadiusTopLeft, clipRadiusTopRight;
    uint8 clipRadiusBottomLeft, clipRadiusBottomRight;
    uint32 colourRGBA;
    uint32 textureIndex;
} ClippedGlyphRenderData;

typedef struct ImageRenderData {
    float32 x, y, z;
    float32 w, h;
    float32 u0, v0;
    float32 u1, v1;
    uint16 clipTop, clipBottom;
    uint16 clipLeft, clipRight;
    uint8 clipRadiusTopLeft, clipRadiusTopRight;
    uint8 clipRadiusBottomLeft, clipRadiusBottomRight;
    uint8 textureIndex, filter, repeat;
} ImageRenderData;

// ----------------------
// --- Init Functions ---
// ----------------------
void Index_List_Init(Index_List* list, uint32 capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(uint32));
}
void Vertex_RGB_List_Init(Vertex_RGB_List* list, uint32 capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(vertex_rgb));
}

// ----------------------
// --- Free Functions ---
// ----------------------
void Index_List_Free(Index_List* list) {
    free(list->array);
    list->size = 0;
    list->capacity = 0;
}
void Vertex_RGB_List_Free(Vertex_RGB_List* list) {
    free(list->array);
    list->size = 0;
    list->capacity = 0;
}
// ----------------------
// --- Length Functions --
// ----------------------
inline uint32 Vertex_RGB_List_Length(Vertex_RGB_List* list) {
    return list->size;
}

// ----------------------
// --- Clear Functions --
// ----------------------
void Index_List_Clear(Index_List* list) {
    list->size = 0;
    list->capacity = 0;
}
void Vertex_RGB_List_Clear(Vertex_RGB_List* list) {
    list->size = 0;
    list->capacity = 0;
}

// ----------------------
// --- Grow Functions ---
// ----------------------
void Index_List_Grow(Index_List* list, uint32 extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(uint32));
}
void Vertex_RGB_List_Grow(Vertex_RGB_List* list, uint32 extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(vertex_rgb));
}






typedef struct {
    float32 x, y, z, sdf, sdf2;
    float32 packedColor;
} vertexSDF;

typedef struct {
    vertexSDF* vertices;
    uint32* indices;
    uint32 vertexCount;
    uint32 vertexCapacity;
    uint32 indexCount;
    uint32 indexCapacity;
} VertexSDFList;


void VertexSDFList_Init(VertexSDFList* vertexList, uint32 vertexCapacity, uint32 indexCapacity)
{
    vertexList->vertexCapacity = vertexCapacity;
    vertexList->indexCapacity = indexCapacity;
    vertexList->vertexCount = 0;
    vertexList->indexCount = 0;
    vertexList->vertices = malloc(vertexCapacity * sizeof(vertexSDF));
    vertexList->indices = malloc(indexCapacity * sizeof(uint32));
}

inline void VertexSDFList_Free(VertexSDFList* vertexList)
{
    free(vertexList->vertices);
    free(vertexList->indices);
    vertexList->vertexCapacity = 0;
    vertexList->indexCapacity = 0;
    vertexList->vertexCount = 0;
    vertexList->indexCount = 0;
}

void VertexSDFList_EnsureVertexCapacity(VertexSDFList* vertexList, uint32 additionalVertices)
{
    uint32 newCapacityRequired = vertexList->vertexCapacity + additionalVertices;
    if (newCapacityRequired > vertexList->vertexCapacity) {
        vertexList->vertexCapacity = max(vertexList->vertexCapacity * 2, newCapacityRequired);
        vertexList->vertices = realloc(vertexList->vertices, vertexList->vertexCapacity * sizeof(vertexSDF));
    }
}

void VertexSDFList_EnsureIndexCapacity(VertexSDFList* vertexList, uint32 additionalIndices)
{
    uint32 newCapacityRequired = vertexList->vertexCapacity + additionalIndices;
    if (newCapacityRequired > vertexList->indexCapacity) {
        vertexList->indexCapacity = max(vertexList->indexCapacity * 2, newCapacityRequired);
        vertexList->indices = realloc(vertexList->indices, vertexList->indexCapacity * sizeof(uint32));
    }
}

inline void VertexSDFList_Clear(VertexSDFList* vertexList)
{
    vertexList->vertexCount = 0;
    vertexList->indexCount = 0;
}

inline uint32 VertexSDFList_VertexCount(VertexSDFList* vertexList)
{
    return vertexList->vertexCount;
}

inline uint32 VertexSDFList_IndexCount(VertexSDFList* vertexList)
{
    return vertexList->indexCount;
}

inline uint32 VertexSDFList_VertexBytes(VertexSDFList* vertexList)
{
    return vertexList->vertexCount * sizeof(vertexSDF);
}

inline uint32 VertexSDFList_IndexBytes(VertexSDFList* vertexList)
{
    return vertexList->indexCount * sizeof(uint32);
}
