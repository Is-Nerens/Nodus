#include "nu_memory_map.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif

#include <stddef.h>

void* NU_Memory_Map(size_t size)
{
#ifdef _WIN32
    return VirtualAlloc(
        NULL,
        size,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    );
#else
    void* ptr = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    return ptr == MAP_FAILED ? NULL : ptr;
#endif
}

void NU_Memory_Unmap(void* ptr, size_t size)
{
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}
