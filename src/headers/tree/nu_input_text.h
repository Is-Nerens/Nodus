#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct InputText {
    char* buffer;
    u32 length;
    u32 capacity;
    u32 cursor;
    float cursorOffset;
    float textOffset;
} InputText;

void InputText_Init(InputText* text)
{
    text->capacity = 64;
    text->buffer = malloc(text->capacity);
    text->buffer[0] = '\0';
    text->length = 0;
    text->cursor = 0;
    text->cursorOffset = 0.0f;
    text->textOffset = 0.0f;
}

void InputText_Free(InputText* text)
{
    free(text->buffer);
    text->buffer = NULL;
    text->length = 0;
    text->capacity = 0;
    text->cursor = 0;
    text->cursorOffset = 0.0f;
    text->textOffset = 0.0f;
}

// writes to text at cursor position
void InputText_Write(InputText* text, const char* string)
{
    u32 stringLen = strlen(string);
    u32 required = text->length + stringLen + 1;
    if (required > text->capacity) {
        text->capacity *= 2;
        if (text->capacity < required) text->capacity = required;
        text->buffer = realloc(text->buffer, text->capacity);
    }
    memmove(
        text->buffer + text->cursor + stringLen,
        text->buffer + text->cursor,
        text->length - text->cursor + 1
    );
    memcpy(text->buffer + text->cursor, string, stringLen);
    text->length += stringLen;
    text->cursor += stringLen;
    text->buffer[text->length] = '\0';
}

// removes one char at cursor position
void InputText_Backspace(InputText* text)
{
    if (text->cursor == 0) return;

    // move to start of previous UTF-8 codepoint
    u32 i = text->cursor - 1;  // step left once
    while (i > 0 && ((unsigned char)text->buffer[i] & 0xC0) == 0x80) {
        i--;
    }
    u32 bytes = text->cursor - i;
    memmove(
        text->buffer + i,
        text->buffer + text->cursor,
        text->length - text->cursor + 1
    );
    text->cursor -= bytes;
    text->length -= bytes;
}

// replaces highlighted section with a new string
void InputText_ReplaceSlice(InputText* text, char* string, u32 index, u32 numChars)
{

}

// removes a highlighted section
void InputText_RemoveSlice(InputText* text, u32 index, u32 numChars)
{

}

inline void InputText_MoveCursorLeft(InputText* text)
{
    if (text->cursor == 0)
        return;

    text->cursor--;  // step left at least once

    // back over continuation bytes
    while (text->cursor > 0 && ((unsigned char)text->buffer[text->cursor] & 0xC0) == 0x80)
        text->cursor--;
}


inline void InputText_MoveCursorRight(InputText* text)
{
    if (text->cursor >= text->length)
        return;

    unsigned char c = (unsigned char)text->buffer[text->cursor];

    // ASCII or start of UTF-8 codepoint
    if ((c & 0x80) == 0) {
        text->cursor += 1; // ASCII
    } else {
        // UTF-8 multi-byte: skip continuation bytes
        text->cursor += 1;
        while (text->cursor < text->length && ((unsigned char)text->buffer[text->cursor] & 0xC0) == 0x80)
            text->cursor++;
    }
}