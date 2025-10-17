#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

unsigned char* Load_File(const char* filepath, int* size_out)
{
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filepath, strerror(errno));
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buffer = (unsigned char*)malloc(size);
    fread(buffer, 1, size, f);
    fclose(f);
    *size_out = (int)size;
    return buffer;
}
