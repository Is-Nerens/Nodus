#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <datastructures/string.h>

String FileReadUTF8(const char* filepath)
{
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filepath, strerror(errno)); return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f); return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f); return NULL;
    }
    rewind(f);
    String result = StringCreateBuffer((uint32_t)((size_t)size));
    if (result == NULL) return NULL;
    size_t bytes = fread(StringCstr(result), 1, (size_t)size, f);
    if (bytes != (size_t)size && ferror(f)) {
        StringFree(result);
        fclose(f);
        return NULL;
    }
    uint32_t bytesU32 = (uint32_t)bytes;
    memcpy(result, &bytes, sizeof(uint32_t));
    StringCstr(result)[bytes] = '\0';
    fclose(f);
    return result;
}