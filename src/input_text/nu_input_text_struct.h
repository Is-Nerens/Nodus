#pragma once

typedef struct InputText {
    char* buffer;
    float cursorOffset;
    float highlightOffset;
    float textOffset;
    int decimalByteIndex;
    u16 numBytes;
    u16 capacity;
    u16 cursorBytes;
    u16 highlightBytes;
    u8 type; // 0 -> text, 1 -> number
    bool dragging;
    bool updateOffsetsPostLayout;
} InputText;

// Initialises an InputText struct and allocated memory for the UTF8 string
void InputText_Init(InputText* text)
{
    text->capacity = 64;
    text->buffer = malloc(text->capacity);
    text->buffer[0] = '\0';
    text->numBytes = 0;
    text->cursorBytes = 0;
    text->cursorOffset = 0.0f;
    text->highlightOffset = 0.0f;
    text->textOffset = 0.0f;
    text->highlightBytes = 0; // highlighted region is from highlightBytes to cursorBytes
    text->decimalByteIndex = -1;
    text->type = 0;
    text->dragging = false;
}

// Frees the underlying UTF8 string memory
void InputText_Free(InputText* text)
{
    free(text->buffer);
    text->buffer = NULL;
}