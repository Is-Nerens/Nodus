#include <datastructures/Linalloc.h>
#include <datastructures/Array.h>

typedef struct ErrorSystem {
    char* buffer;
    size_t bufferSize;
    size_t readHead;
    size_t writeHead;
    u32 errorMessageCount;
    u32 droppedMessageCount; // Messages that didn't fit in the buffer
} ErrorSystem;

void ErrorSystem_Init(ErrorSystem* errorSys)
{
    errorSys->buffer = malloc(512);
    errorSys->errorMessageCount = 0;
    errorSys->readHead = 0;
    errorSys->writeHead = 0;
}

void ErrorSystem_Free(ErrorSystem* errorSys)
{
    free(errorSys->buffer);
}

void ErrorSystem_AddError(ErrorSystem* errorSys, const char* err)
{
    size_t errLen = stringLen(err);
    u16 advance = (u16)(sizeof(u16) + errLen + 1);

    // Ran out of write space -> increment dropped count
    if (errorSys->writeHead + advance > errorSys->bufferSize) {
        errorSys->droppedMessageCount++;
        errorSys->errorMessageCount++;
        return;
    }

    memcpy(errorSys->buffer + errorSys->writeHead, &advance, sizeof(advance));
    memcpy(errorSys->buffer + errorSys->writeHead + sizeof(advance), err, errLen + 1);
    errorSys->writeHead += advance;
    errorSys->errorMessageCount++;
}

void ErrorSystem_Clear(ErrorSystem* errorSys)
{   
    errorSys->errorMessageCount = 0;
    errorSys->readHead = 0;
    errorSys->writeHead = 0;
}

const char* ErrorSystem_GetNextError(ErrorSystem* errorSys)
{
    static char nMoreBuf[32];

    if (errorSys->readHead >= errorSys->writeHead) {
        if (errorSys->droppedMessageCount > 0) {
            u32 n = errorSys->droppedMessageCount;
            errorSys->readHead = 0;
            errorSys->writeHead = 0;
            errorSys->errorMessageCount = 0;
            errorSys->droppedMessageCount = 0;
            snprintf(nMoreBuf, sizeof(nMoreBuf), "%u more", n);
            return nMoreBuf;
        }
        errorSys->readHead = 0;
        errorSys->writeHead = 0;
        errorSys->errorMessageCount = 0;
        return NULL;
    }

    u16 advance;
    memcpy(&advance, errorSys->buffer + errorSys->readHead, sizeof(u16));
    const char* err = errorSys->buffer + errorSys->readHead + sizeof(u16);
    errorSys->readHead += advance;
    return err;
}