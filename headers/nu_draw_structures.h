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
    vertex_rgb* array;
    uint32_t size;
    uint32_t capacity;
} Vertex_RGB_List;

void Vertex_RGB_List_Init(Vertex_RGB_List* list, uint32_t capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(vertex_rgb));
}

void Vertex_RGB_List_Free(Vertex_RGB_List* list) {
    free(list->array);
}

void Vertex_RGB_List_Grow(Vertex_RGB_List* list, uint32_t extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(vertex_rgb));
}

typedef struct {
    GLuint* array;
    uint32_t size;
    uint32_t capacity;
} Index_List;

void Index_List_Init(Index_List* list, uint32_t capacity) {
    list->size = 0;
    list->capacity = capacity;
    list->array = malloc(capacity * sizeof(GLuint));
}

void Index_List_Free(Index_List* list) {
    free(list->array);
}

void Index_List_Grow(Index_List* list, uint32_t extra_capacity) {
    list->capacity = max(list->capacity * 2, list->capacity + extra_capacity);
    list->array = realloc(list->array, list->capacity * sizeof(GLuint));
}
