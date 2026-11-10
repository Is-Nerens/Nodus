
typedef struct ErrorSystem {
    char* buffer;
    usize bufferSize;
    usize readHead;
    usize writeHead;
    uint32 errorMessageCount;
    uint32 droppedMessageCount; // Messages that didn't fit in the buffer
} ErrorSystem;

void ErrorSystem_Init(ErrorSystem* errorSys)
{
    errorSys->bufferSize = 512;
    errorSys->buffer = malloc(errorSys->bufferSize);
    errorSys->errorMessageCount = 0;
    errorSys->readHead = 0;
    errorSys->writeHead = 0;
    errorSys->droppedMessageCount = 0;
}

void ErrorSystem_Free(ErrorSystem* errorSys)
{
    free(errorSys->buffer);
    errorSys->buffer = NULL;
    errorSys->bufferSize = 0;
    errorSys->errorMessageCount = 0;
    errorSys->droppedMessageCount = 0;
    errorSys->readHead = 0;
    errorSys->writeHead = 0;
}

void ErrorSystem_AddError(ErrorSystem* errorSys, const char* err)
{
    usize errLen = stringLen(err);
    uint16 advance = (uint16)(sizeof(uint16) + errLen + 1);

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
    errorSys->droppedMessageCount = 0;
    errorSys->readHead = 0;
    errorSys->writeHead = 0;
}

const char* ErrorSystem_GetNextError(ErrorSystem* errorSys)
{
    static char nMoreBuf[32];

    // Return the next buffered error first
    if (errorSys->readHead < errorSys->writeHead) {
        uint16 advance;
        memcpy(&advance, errorSys->buffer + errorSys->readHead, sizeof(uint16));

        const char* err = errorSys->buffer + errorSys->readHead + sizeof(uint16);
        errorSys->readHead += advance;
        return err;
    }

    // Report dropped messages after buffered errors
    if (errorSys->droppedMessageCount > 0) {
        uint32 n = errorSys->droppedMessageCount;
        errorSys->droppedMessageCount = 0;
        snprintf(nMoreBuf, sizeof(nMoreBuf), "%u errors dropped", n);
        return nMoreBuf;
    }

    // No more errors
    errorSys->readHead = 0;
    errorSys->writeHead = 0;
    errorSys->errorMessageCount = 0;
    return NULL;
}
