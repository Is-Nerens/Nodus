#pragma once
#include <GL/glew.h>
#include <math.h>

typedef struct {
    float x;
    float y;
} vec2;

typedef struct {
    float x; 
    float y;
    float r;
    float g;
    float b;
} vertex_rgb;

typedef struct {
    float x;
    float y;
    float u;
    float v;
} vertex_uv;

typedef struct {
    float x, y;
    float r, g, b;
    float u, v;
} vertex_rgb_uv;

typedef struct {
    GLuint* array;
    uint32_t size;
    uint32_t capacity;
} Index_List;

typedef struct {
    vertex_rgb* array;
    uint32_t size;
    uint32_t capacity;
} Vertex_RGB_List;

typedef struct {
    vertex_uv* array;
    uint32_t size;
    uint32_t capacity;
} Vertex_UV_List;

typedef struct {
    vertex_rgb_uv* array;
    uint32_t size;
    uint32_t capacity;
} Vertex_RGB_UV_List;

// ----------------------
// --- Init Functions ---
// ----------------------
void Index_List_Init(Index_List* list, uint32_t capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(GLuint));
}
void Vertex_RGB_List_Init(Vertex_RGB_List* list, uint32_t capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(vertex_rgb));
}
void Vertex_UV_List_Init(Vertex_UV_List* list, uint32_t capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(vertex_uv));
}
void Vertex_RGB_UV_List_Init(Vertex_RGB_UV_List* list, uint32_t capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(vertex_rgb_uv));
}

// ----------------------
// --- Free Functions ---
// ----------------------
void Index_List_Free(Index_List* list) {
    free(list->array);
}
void Vertex_RGB_List_Free(Vertex_RGB_List* list) {
    free(list->array);
}
void Vertex_UV_List_Free(Vertex_UV_List* list) {
    free(list->array);
}
void Vertex_RGB_UV_List_Free(Vertex_RGB_UV_List* list) {
    free(list->array);
}

// ----------------------
// --- Grow Functions ---
// ----------------------
void Index_List_Grow(Index_List* list, uint32_t extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(GLuint));
}
void Vertex_RGB_List_Grow(Vertex_RGB_List* list, uint32_t extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(vertex_rgb));
}
void Vertex_UV_List_Grow(Vertex_UV_List* list, uint32_t extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(vertex_uv));
}
void Vertex_RGB_UV_List_Grow(Vertex_RGB_UV_List* list, uint32_t extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(vertex_rgb_uv));
}