#pragma once

#include "vector.h"

struct Text_Ref
{
    uint32_t node_ID;
    uint32_t buffer_index;
    uint32_t char_count;
    uint32_t char_capacity; // excludes the null terminator
};

struct Arena_Free_Element
{
    uint32_t index;
    uint32_t length;
};

struct Text_Arena
{
    struct Vector free_list;
    struct Vector text_refs;
    struct Vector char_buffer;
};