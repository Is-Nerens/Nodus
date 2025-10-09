#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <nu_font-old.h>

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

void Load_Font_Resource(const char* ttf_path, struct Font_Resource* resource_out)
{
    const char* filename = strrchr(ttf_path, '/');
    #ifdef _WIN32
    if (!filename) filename = strrchr(ttf_path, '\\');
    #endif
    if (filename) filename++;
    else filename = ttf_path;
    resource_out->name = strdup(filename); 
    resource_out->data = Load_File(ttf_path, &resource_out->size);
    if (!resource_out->data) {
        fprintf(stderr, "Error: could not load font resource '%s'\n", ttf_path);
        free( resource_out->name);
         resource_out->name = NULL;
    }
}