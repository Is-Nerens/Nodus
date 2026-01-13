#pragma once
#include <rendering/nu_draw_structures.h>
#include <math.h>

void NU_Internal_Clear_Canvas(uint32_t canvas_handle)
{
    NU_Canvas_Context* canvas_context = HashmapGet(&__NGUI.canvas_contexts, &canvas_handle);
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
    Node* canvas_node = NODE(canvas_handle);
    if (canvas_node->tag != CANVAS) return;

    NU_Canvas_Context* canvas_context = HashmapGet(&__NGUI.canvas_contexts, &canvas_handle);
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
    Node* canvas_node = NODE(canvas_handle);
    if (canvas_node->tag != CANVAS) return;

    NU_Canvas_Context* canvas_context = HashmapGet(&__NGUI.canvas_contexts, &canvas_handle);
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

void NU_Internal_Dashed_Line(
    uint32_t canvas_handle,
    float x1, float y1, float x2, float y2,
    float thickness,
    uint8_t* dash_pattern,
    uint32_t dash_pattern_len,
    NU_RGB* col
)
{
    Node* canvas_node = NODE(canvas_handle);
    if (canvas_node->tag != CANVAS) return;


    // Calculate min number of additional vertices + indices
    x1 += 0.5f;
    x2 += 0.5f;
    y1 += 0.5f;
    y2 += 0.5f;
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    if (length == 0.0f) return; // avoid divide-by-zero
    uint32_t pattern_len = 0;
    uint32_t vertices_per_pattern = 0;
    uint32_t indices_per_pattern = 0;
    for (uint32_t i=0; i<dash_pattern_len; i++)
    {
        pattern_len += dash_pattern[i];
        vertices_per_pattern += (dash_pattern[i] * ((i % 2) == 0)) * 4;
        indices_per_pattern += (dash_pattern[i] * ((i % 2) == 0)) * 6;
    }
    uint32_t min_additional_vertices = ((uint32_t)(length / pattern_len) + 1) * vertices_per_pattern;    
    uint32_t min_additional_indices  = ((uint32_t)(length / pattern_len) + 1) * indices_per_pattern;   



    // Allocate extra space in vertex and index lists (if needed)
    NU_Canvas_Context* canvas_context = HashmapGet(&__NGUI.canvas_contexts, &canvas_handle);
    Vertex_RGB_List* vertices = &canvas_context->vertices;
    Index_List* indices = &canvas_context->indices;
    if (vertices->size + min_additional_vertices > vertices->capacity) Vertex_RGB_List_Grow(vertices, min_additional_vertices);
    if (indices->size + min_additional_indices > indices->capacity) Index_List_Grow(indices, min_additional_indices);



    // Construct line mesh
    uint32_t i = 0;
    float travelled = 0.0f;
    float step_x = dx / length;
    float step_y = dy / length;
    float half_thick = thickness * 0.5f;
    while (travelled < length && travelled < 16000.0f)
    {
        float segment_len = dash_pattern[i];
        float segment_start = travelled;
        float segment_end = travelled + segment_len;

        if (i % 2 == 0) // Dash segment
        {
            if (segment_end > length) segment_end = length;

            float sx1 = x1 + step_x * segment_start;
            float sy1 = y1 + step_y * segment_start;
            float sx2 = x1 + step_x * segment_end;
            float sy2 = y1 + step_y * segment_end;

            // Construct quad mesh
            float dx_seg = sx2 - sx1;
            float dy_seg = sy2 - sy1;
            float seg_len = sqrtf(dx_seg * dx_seg + dy_seg * dy_seg);
            if (seg_len > 0.0f)
            {
                dx_seg /= seg_len;
                dy_seg /= seg_len;
                float px = -dy_seg * half_thick;
                float py =  dx_seg * half_thick;
                uint32_t v0 = vertices->size;
                vertex_rgb* v = vertices->array + v0;
                // V1-
                v[0].x = sx1 - px; v[0].y = sy1 - py;
                v[0].r = col->r; v[0].g = col->g; v[0].b = col->b;
                // V1+
                v[1].x = sx1 + px; v[1].y = sy1 + py;
                v[1].r = col->r; v[1].g = col->g; v[1].b = col->b;
                // V2-
                v[2].x = sx2 - px; v[2].y = sy2 - py;
                v[2].r = col->r; v[2].g = col->g; v[2].b = col->b;
                // V2+
                v[3].x = sx2 + px; v[3].y = sy2 + py;
                v[3].r = col->r; v[3].g = col->g; v[3].b = col->b;
                uint32_t* id = indices->array + indices->size;
                *id++ = v0;
                *id++ = v0 + 1;
                *id++ = v0 + 2;
                *id++ = v0 + 1;
                *id++ = v0 + 2;
                *id++ = v0 + 3;
                vertices->size += 4;
                indices->size += 6;
            }
        }

        travelled += segment_len;
        i = (i + 1) % dash_pattern_len;
    }
}