#pragma once
#include <stddef.h>

void* NU_Memory_Map(size_t size);
void  NU_Memory_Unmap(void* ptr, size_t size);
