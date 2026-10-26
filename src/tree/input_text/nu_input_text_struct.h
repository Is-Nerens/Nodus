#pragma once

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
    text->updateOffsetsPostLayout = false;
}

// Frees the underlying UTF8 string memory
void InputText_Free(InputText* text)
{
    free(text->buffer);
    text->buffer = NULL;
}
