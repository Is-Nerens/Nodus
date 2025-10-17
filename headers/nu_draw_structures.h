#pragma once
#include <GL/glew.h>
#include <nu_convert.h>
#include <math.h>

typedef struct {
    float r, g, b;
} NU_RGB;

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

typedef struct {
    Vertex_RGB_List vertices;
    Index_List indices;
} NU_Canvas_Context;

// typedef struct {

// } Canvas_Context_List;

void RGB_From_Hex(const char* hexstring, NU_RGB* result)
{
    // Ensure string is 7 chars long and first char is '#'
    int i=0;
    while(1) {
        if (hexstring[i] == 0) {
            break;
        }
        if (i > 7) {
            result->r = 1.0f;
            result->g = 1.0f;
            result->b = 1.0f;
            return;
        }
        i++;
    }
    if (hexstring[0] != '#') {
        result->r = 1.0f;
        result->g = 1.0f;
        result->b = 1.0f;
        return;
    }

    // Perform the conversion
    int r1 = Hex_To_Int(hexstring[1]);
    int r2 = Hex_To_Int(hexstring[2]);
    int g1 = Hex_To_Int(hexstring[3]);
    int g2 = Hex_To_Int(hexstring[4]);
    int b1 = Hex_To_Int(hexstring[5]);
    int b2 = Hex_To_Int(hexstring[6]);
    if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0)
    {
        result->r = 1.0f;
        result->g = 1.0f;
        result->b = 1.0f;
        return;
    }
    result->r = (uint8_t)((r1 << 4) | r2);
    result->g = (uint8_t)((g1 << 4) | g2);
    result->b = (uint8_t)((b1 << 4) | b2);
}

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
    list->size = 0;
    list->capacity = 0;
}
void Vertex_RGB_List_Free(Vertex_RGB_List* list) {
    free(list->array);
    list->size = 0;
    list->capacity = 0;
}
void Vertex_UV_List_Free(Vertex_UV_List* list) {
    free(list->array);
    list->size = 0;
    list->capacity = 0;
}
void Vertex_RGB_UV_List_Free(Vertex_RGB_UV_List* list) {
    free(list->array);
    list->size = 0;
    list->capacity = 0;
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