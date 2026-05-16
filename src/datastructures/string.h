#pragma once
#include <stdint.h>
#include <string.h>
#include <stdarg.h>



// -----------------------------------------------------------------------------------
// --- This is a custom string allocator. The purpose is to reduce heap fragmentation.
// --- A pool allocator will be used for small string allocations.
// -----------------------------------------------------------------------------------
typedef struct SallocChunk SallocChunk;
struct SallocChunk {
    SallocChunk* next;
};

typedef struct SallocBlock SallocBlock;
struct SallocBlock {
    void* block;
    SallocBlock* next;
};

typedef struct Salloc {
    SallocChunk* freeChunk;
    SallocBlock* blockStart;
    uint32_t chunksPerBlock;
    uint32_t chunkSize;
    int init;
} Salloc;

__declspec(thread) Salloc salloc = {0};

static void SallocEnsureInit()
{
    if (!salloc.init) 
    {
        size_t itemSize = 32;
        size_t itemsPerBlock = 256;
        size_t stride = sizeof(SallocChunk) + itemSize;

        void* block = malloc(itemsPerBlock * stride);
        if (!block) return;

        SallocBlock* node = (SallocBlock*)malloc(sizeof(SallocBlock));
        if (!node) {
            free(block);
            return;
        }

        char* ptr = (char*)block;

        for (size_t i=0; i<itemsPerBlock-1; i++) {
            SallocChunk* chunk = (SallocChunk*)(ptr + i * stride);
            SallocChunk* next  = (SallocChunk*)(ptr + (i+1) * stride);
            chunk->next = next;
        }

        SallocChunk* last = (SallocChunk*)(ptr + (itemsPerBlock-1) * stride);
        last->next = NULL;
        salloc.freeChunk = (SallocChunk*)block;
        salloc.blockStart = node;
        salloc.chunksPerBlock = itemsPerBlock;
        salloc.chunkSize = itemSize;
        node->block = block;
        node->next  = NULL;
    }

    salloc.init = 1;
}

int SallocExpand(size_t extraChunks)
{
    if (extraChunks == 0) return 0;
    size_t stride = sizeof(SallocChunk) + salloc.chunkSize;
    void* extraBlock = malloc(extraChunks * stride);
    if (extraBlock == NULL) return 0;
    SallocBlock* blockStart = (SallocBlock*)malloc(sizeof(SallocBlock));
    if (blockStart == NULL) {
        free(extraBlock);
        return 0;
    }
    char* ptr = (char*)extraBlock;
    for (size_t i=0; i<extraChunks-1; i++) {
        SallocChunk* chunk = (SallocChunk*)(ptr + i * stride);
        SallocChunk* next  = (SallocChunk*)(ptr + (i+1) * stride);
        chunk->next = next;
    }
    SallocChunk* last = (SallocChunk*)(ptr + (extraChunks-1) * stride);
    last->next = salloc.freeChunk;
    salloc.freeChunk = (SallocChunk*)extraBlock;
    blockStart->block = extraBlock;
    blockStart->next  = salloc.blockStart;
    salloc.blockStart  = blockStart;
    return 1;
}

void* SallocAlloc(size_t size)
{
    // Use malloc for large allocation
    if (size > 32) return malloc(size);

    SallocEnsureInit();

    // Use pool allocator for small allocation
    if (salloc.freeChunk == NULL) {
        if (!SallocExpand(salloc.chunksPerBlock)) return NULL;
    }
    SallocChunk* result = salloc.freeChunk;
    salloc.freeChunk = salloc.freeChunk->next;
    return (void*)(result + 1);
}

void SallocFree(void* ptr, size_t size)
{
    if (!salloc.init) return;
    if (size > 32) {
        free(ptr);
        return;
    }

    SallocChunk* freed = ((SallocChunk*)ptr) - 1;
    freed->next = salloc.freeChunk;
    salloc.freeChunk = freed;
}





// -----------------------------------------------------------------------------------
// --- These functions serve as alternatives to <string.h> standard implementations 
// --- The purpose is to avoid using unsafe deprecated string functions that trigger 
// --- compiler warnings. And give these functions proper names because eish!            
// -----------------------------------------------------------------------------------
inline size_t stringLen(const char* str)
{
    return strlen(str);
}

inline void stringCopy(char* dest, size_t destSize, const char* src)
{
    strcpy_s(dest, destSize, src);
}

inline int stringEquals(const char* strA, const char* strB)
{
    return (strcmp(strA, strB) == 0);
}

inline void stringNullTerminate(char* str, size_t bufSize)
{
    str[bufSize - 1] = '\0';
}

inline void stringNullTerminateAt(char* str, size_t bufSize, size_t index)
{
    if (index >= bufSize) index = bufSize - 1;
    str[index] = '\0';
}

inline int stringFormat(char* buf, size_t bufSize, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buf, bufSize, format, args);
    va_end(args);
    return n;
}

inline int stringAppend(char* dest, size_t destSize, const char* src)
{
    size_t len = strlen(dest);
    if (len >= destSize) return 0;
    int n = snprintf(dest + len, destSize - len, "%s", src);
    return (n >= 0 && (size_t)n < (destSize - len));
}

inline int stringContains(const char* str, const char* subStr)
{
    return strstr(str, subStr) != NULL;
}

inline int stringFind(const char* str, const char* subStr)
{
    const char* pos = strstr(str, subStr);
    return pos ? (int)(pos - str) : -1;
}

inline int stringStartsWith(const char* str, const char* prefix)
{
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) == 0;
}

inline int stringEndsWith(const char* str, const char* suffix)
{
    size_t strLen = strlen(str);
    size_t sufLen = strlen(suffix);
    if (sufLen > strLen) return 0;
    return strcmp(str + strLen - sufLen, suffix) == 0;
}



// ------------------------------------------------------------------------
// --- This is an implementation of utf8 encoded heap allocated strings ---
// ------------------------------------------------------------------------

// DATA LAYOUT
// [header: 4 bytes][data][\n]
typedef unsigned char* String;

String StringCreate(const char* str)
{
    uint32_t length = (uint32_t)strlen(str);
    uint32_t allocSize = length + 5;
    String result = SallocAlloc(allocSize);
    if (!result) return NULL;
    memcpy(result, &length, sizeof(uint32_t));
    memcpy(result + 4, str, length);
    result[4 + length] = '\0';
    return result;
}

String StringCreateBuffer(uint32_t bytes)
{
    uint32_t allocSize = bytes + 5;
    String result = SallocAlloc(allocSize);
    if (!result) return NULL;
    memcpy(result, &bytes, sizeof(uint32_t));
    result[bytes + 4] = '\0';
    return result;
}
inline uint32_t StringLen(String string)
{
    return *(uint32_t*)string;
}

inline void StringFree(String str)
{
    if (str == NULL) return;
    SallocFree(str, StringLen(str) + 5);
}

inline char* StringCstr(String str)
{
    return (char*)str + 4;
}

uint32_t NextUTF8Codepoint(String string, int* i)
{
    char* cStr = StringCstr(string);
    unsigned char* p = (unsigned char*)cStr + *i;
    uint32_t cp;

    if (*p == 0) return 0;  // end of string

    if ((*p & 0x80) == 0) {
        cp = *p++;
    }
    else if ((*p & 0xE0) == 0xC0 &&
            (p[1] & 0xC0) == 0x80) {
        cp = (*p & 0x1F) << 6 |
            (p[1] & 0x3F);
        p += 2;
    }
    else if ((*p & 0xF0) == 0xE0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80) {
        cp = (*p & 0x0F) << 12 |
            (p[1] & 0x3F) << 6 |
            (p[2] & 0x3F);
        p += 3;
    }
    else if ((*p & 0xF8) == 0xF0 &&
            (p[1] & 0xC0) == 0x80 &&
            (p[2] & 0xC0) == 0x80 &&
            (p[3] & 0xC0) == 0x80) {
        cp = (*p & 0x07) << 18  |
            (p[1] & 0x3F) << 12 |
            (p[2] & 0x3F) << 6  |
            (p[3] & 0x3F);
        p += 4;
    }
    else { // invalid byte
        cp = 0xFFFD;
        p += 1;
    }
    *i = (int)(p - (unsigned char*)cStr);
    return cp;
}

inline uint32_t GetUTF32Codepoint(String string, int index) 
{
    uint32_t codepoint;
    memcpy(&codepoint, StringCstr(string) + index * 4, 4);
    return codepoint;
}

String StringConcat(String a, String b)
{
    // validate strings and get metadata
    if (a == NULL || b == NULL) return NULL;
    uint32_t aLen = StringLen(a);
    uint32_t bLen = StringLen(b);

    // allocate space for return string
    String output = NULL;
    output = SallocAlloc(aLen + bLen + 5);
    if (output == NULL) return NULL;

    // copy data from string a and b
    memcpy(output + 4, a + 4, aLen);
    memcpy(output + 4 + aLen, b + 4, bLen);

    // set metadata
    uint32_t outputLen = aLen + bLen;
    memcpy(output, &outputLen, sizeof(uint32_t));
    output[outputLen + 4] = '\0';
    return output;
}

String StringConcatCstr(String a, char* b)
{
    if (a == NULL || b == NULL) return NULL;
    uint32_t aLen = StringLen(a);
    uint32_t bLen = strlen(b);

    // allocate space for return string
    String output = NULL;
    output = SallocAlloc(aLen + bLen + 5);
    if (output == NULL) return NULL;

    // copy data from string a and b
    memcpy(output + 4, a + 4, aLen);
    memcpy(output + 4 + aLen, b, bLen);

    // set metadata
    uint32_t outputLen = aLen + bLen;
    memcpy(output, &outputLen, sizeof(uint32_t));
    output[outputLen + 4] = '\0';
    return output;
}

String CstrConcatString(char* a, String b)
{
    if (a == NULL || b == NULL) return NULL;
    uint32_t aLen = strlen(a);
    uint32_t bLen = StringLen(b);
    
    // allocate space for return string
    String output = NULL;
    output = SallocAlloc(aLen + bLen + 5);
    if (output == NULL) return NULL;

    // copy data from string a and b
    memcpy(output + 4, a, aLen);
    memcpy(output + 4 + aLen, b + 4, bLen);

    // set metadata
    uint32_t outputLen = aLen + bLen;
    memcpy(output, &outputLen, sizeof(uint32_t));
    output[outputLen + 4] = '\0';
    return output;
}

inline String StringCopy(String string)
{
    uint32_t bytes = StringLen(string) + 5;
    String copy = SallocAlloc(bytes);
    memcpy(copy, string, bytes);
    return copy;
}

int StringContains(String string, String search)
{
    // validate strings and get metadata
    if (string == NULL || search == NULL) return -1;
    uint32_t stringLen = StringLen(string);
    uint32_t searchLen = StringLen(search);
    if (stringLen == 0 || searchLen == 0) {
        return -1;
    }
    char* stringCstr = StringCstr(string);
    char* searchCstr = StringCstr(search);

    // longest prefix-suffix
    uint16_t* lps = (uint16_t*)calloc(searchLen, sizeof(uint16_t));
    int j = 0;
    int i = 1;
    while (i < searchLen) {
        if (searchCstr[i] == searchCstr[j]) {
            j += 1; lps[i] = j; i += 1;
        }
        else {
            if (j != 0) { j = lps[j - 1]; }
            else {
                lps[i] = 0; i += 1;
            }
        }
    }

    // knuth-morris-pratt
    i = 0;
    j = 0;
    while (i < stringLen) {
        if (stringCstr[i] == searchCstr[j]) {
            i++; j++;
        }
        if (j == searchLen) {
            free(lps);
            return i - j;
        }
        else if (i < stringLen && stringCstr[i] != searchCstr[j]) {
            if (j != 0) { j = lps[j-1]; } 
            else { i++; }
        }
    }
    free(lps);
    return -1;
}

String StringReplaceFirst(String string, String target, String replacement)
{
    if (replacement == NULL) return NULL;
    int match = StringContains(string, target); // checks if string and target is NULL
    if (match == -1) return NULL;

    // construct result string
    uint32_t length = StringLen(string) - StringLen(target) + StringLen(replacement);
    String result = SallocAlloc(length + 5);
    if (!result) return NULL;
    memcpy(StringCstr(result), StringCstr(string), match);
    memcpy(StringCstr(result) + match, StringCstr(replacement), StringLen(replacement));
    memcpy(StringCstr(result) + match + StringLen(replacement), StringCstr(string) + match + StringLen(target), StringLen(string) - match - StringLen(target));
    memcpy(result, &length, sizeof(uint32_t));
    result[length + 4] = '\0';
    return result;
}

String StringRemoveSuffix(String string, String suffix)
{
    // critical errors
    if (string == NULL || 
        suffix == NULL || 
        StringLen(suffix) == 0 || 
        StringLen(suffix) > StringLen(string)) { 
        return NULL;
    }

    // check if string ends with suffix
    if (memcmp(StringCstr(string) + StringLen(string) - StringLen(suffix), StringCstr(suffix), StringLen(suffix)) != 0) {
        return StringCopy(string);
    }

    // construct new string without suffix
    uint32_t newLength = StringLen(string) - StringLen(suffix);
    String result = SallocAlloc(newLength + 5);
    if (!result) return NULL;
    memcpy(StringCstr(result), StringCstr(string), newLength);

    // set header
    memcpy(result, &newLength, sizeof(uint32_t));
    result[newLength + 4] = '\0';
    return result;
}

int StringEquals(String a, String b)
{
    if (!a || !b) return 0;
    uint32_t aLen = *(uint32_t*)a;
    return memcmp(a, b, aLen + 4) == 0;
}

String StringUpdate(String str, const char* new)
{
    StringFree(str);
    return StringCreate(new);
}