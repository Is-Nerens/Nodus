#pragma once
#include "nu_draw_structures.h"
#include <math.h>

void NU_Internal_Clear_Canvas(uint32_t canvas_handle)
{
    NU_Canvas_Context* canvas_context = Hashmap_Get(&__nu_global_gui.canvas_contexts, &canvas_handle);
    canvas_context->vertices.size = 0;
    canvas_context->indices.size = 0;
}

void NU_Internal_Border_Rect(
    uint32_t canvas_handle,
    float x, float y, float w, float h, 
    float thickness,
    NU_RGB* border_col,
    NU_RGB* fill_col)
{
    struct Node* canvas_node = NODE(canvas_handle);
    if (canvas_node->tag != CANVAS) return;

    NU_Canvas_Context* canvas_context = Hashmap_Get(&__nu_global_gui.canvas_contexts, &canvas_handle);
    Vertex_RGB_List* vertices = &canvas_context->vertices;
    Index_List* indices = &canvas_context->indices;

    // Constrain dimensions based on thickness
    w = fmaxf(w, thickness * 2);
    h = fmaxf(h, thickness * 2);

    // --- Allocate extra space in vertex and index lists ---
    uint32_t additional_vertices = 12;    
    uint32_t additional_indices = 30;          
    if (vertices->size + additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, additional_vertices);
    if (indices->size + additional_indices > indices->capacity) Index_List_Grow(indices, additional_indices);

    uint32_t vertex_offset = vertices->size;

    // Outer TL
    vertices->array[vertex_offset].x = x;
    vertices->array[vertex_offset].y = y;
    vertices->array[vertex_offset].r = border_col->r;
    vertices->array[vertex_offset].g = border_col->g;
    vertices->array[vertex_offset].b = border_col->b;

    // Outer TR
    vertices->array[vertex_offset + 1].x = x + w;
    vertices->array[vertex_offset + 1].y = y;
    vertices->array[vertex_offset + 1].r = border_col->r;
    vertices->array[vertex_offset + 1].g = border_col->g;
    vertices->array[vertex_offset + 1].b = border_col->b;

    // Outer BL
    vertices->array[vertex_offset + 2].x = x;
    vertices->array[vertex_offset + 2].y = y + h;
    vertices->array[vertex_offset + 2].r = border_col->r;
    vertices->array[vertex_offset + 2].g = border_col->g;
    vertices->array[vertex_offset + 2].b = border_col->b;

    // Outer BR
    vertices->array[vertex_offset + 3].x = x + w;
    vertices->array[vertex_offset + 3].y = y + h;
    vertices->array[vertex_offset + 3].r = border_col->r;
    vertices->array[vertex_offset + 3].g = border_col->g;
    vertices->array[vertex_offset + 3].b = border_col->b;

    // Inner TL
    vertices->array[vertex_offset + 4].x = x + thickness;
    vertices->array[vertex_offset + 4].y = y + thickness;
    vertices->array[vertex_offset + 4].r = border_col->r;
    vertices->array[vertex_offset + 4].g = border_col->g;
    vertices->array[vertex_offset + 4].b = border_col->b;

    // Inner TR
    vertices->array[vertex_offset + 5].x = x + w - thickness;
    vertices->array[vertex_offset + 5].y = y + thickness;
    vertices->array[vertex_offset + 5].r = border_col->r;
    vertices->array[vertex_offset + 5].g = border_col->g;
    vertices->array[vertex_offset + 5].b = border_col->b;

    // Inner BL
    vertices->array[vertex_offset + 6].x = x + thickness;
    vertices->array[vertex_offset + 6].y = y + h - thickness;
    vertices->array[vertex_offset + 6].r = border_col->r;
    vertices->array[vertex_offset + 6].g = border_col->g;
    vertices->array[vertex_offset + 6].b = border_col->b;

    // Inner BR
    vertices->array[vertex_offset + 7].x = x + w - thickness;
    vertices->array[vertex_offset + 7].y = y + h - thickness;
    vertices->array[vertex_offset + 7].r = border_col->r;
    vertices->array[vertex_offset + 7].g = border_col->g;
    vertices->array[vertex_offset + 7].b = border_col->b;

    // Fill TL
    vertices->array[vertex_offset + 8].x = x + thickness;
    vertices->array[vertex_offset + 8].y = y + thickness;
    vertices->array[vertex_offset + 8].r = fill_col->r;
    vertices->array[vertex_offset + 8].g = fill_col->g;
    vertices->array[vertex_offset + 8].b = fill_col->b;

    // Fill TR
    vertices->array[vertex_offset + 9].x = x + w - thickness;
    vertices->array[vertex_offset + 9].y = y + thickness;
    vertices->array[vertex_offset + 9].r = fill_col->r;
    vertices->array[vertex_offset + 9].g = fill_col->g;
    vertices->array[vertex_offset + 9].b = fill_col->b;

    // Fill BL
    vertices->array[vertex_offset + 10].x = x + thickness;
    vertices->array[vertex_offset + 10].y = y + h - thickness;
    vertices->array[vertex_offset + 10].r = fill_col->r;
    vertices->array[vertex_offset + 10].g = fill_col->g;
    vertices->array[vertex_offset + 10].b = fill_col->b;

    // Fill BR
    vertices->array[vertex_offset + 11].x = x + w - thickness;
    vertices->array[vertex_offset + 11].y = y + h - thickness;
    vertices->array[vertex_offset + 11].r = fill_col->r;
    vertices->array[vertex_offset + 11].g = fill_col->g;
    vertices->array[vertex_offset + 11].b = fill_col->b;

    // Indices
    uint32_t* indices_write = indices->array + indices->size;

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

void NU_Internal_Line(
    uint32_t canvas_handle,
    float x1, float y1, float x2, float y2,
    float thickness,
    NU_RGB* col
)
{
    struct Node* canvas_node = NODE(canvas_handle);
    if (canvas_node->tag != CANVAS) return;

    NU_Canvas_Context* canvas_context = Hashmap_Get(&__nu_global_gui.canvas_contexts, &canvas_handle);
    Vertex_RGB_List* vertices = &canvas_context->vertices;
    Index_List* indices = &canvas_context->indices;

    // --- Allocate extra space in vertex and index lists ---
    uint32_t additional_vertices = 4;    
    uint32_t additional_indices = 6;          
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


    uint32_t vertex_offset = vertices->size;

    // V1 -> thick -
    vertices->array[vertex_offset].x = x1 - px;
    vertices->array[vertex_offset].y = y1 - py;
    vertices->array[vertex_offset].r = col->r;
    vertices->array[vertex_offset].g = col->g;
    vertices->array[vertex_offset].b = col->b;

    // V1 -> thick + 
    vertices->array[vertex_offset + 1].x = x1 + px;
    vertices->array[vertex_offset + 1].y = y1 + py;
    vertices->array[vertex_offset + 1].r = col->r;
    vertices->array[vertex_offset + 1].g = col->g;
    vertices->array[vertex_offset + 1].b = col->b;

    // V2 -> thick -
    vertices->array[vertex_offset + 2].x = x2 - px;
    vertices->array[vertex_offset + 2].y = y2 - py;
    vertices->array[vertex_offset + 2].r = col->r;
    vertices->array[vertex_offset + 2].g = col->g;
    vertices->array[vertex_offset + 2].b = col->b;

    // V2 -> thick + 
    vertices->array[vertex_offset + 3].x = x2 + px;
    vertices->array[vertex_offset + 3].y = y2 + py;
    vertices->array[vertex_offset + 3].r = col->r;
    vertices->array[vertex_offset + 3].g = col->g;
    vertices->array[vertex_offset + 3].b = col->b;


    // Indices
    uint32_t* indices_write = indices->array + indices->size;
    *indices_write++ = vertex_offset;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 1;
    *indices_write++ = vertex_offset + 2;
    *indices_write++ = vertex_offset + 3;

    vertices->size += additional_vertices;
    indices->size += additional_indices;
}