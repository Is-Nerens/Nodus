#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct InputText {
    char* buffer;
    u32 numBytes;
    u32 length;
    u32 capacity;
    u32 cursor;
    float cursorOffset;
    float textOffset;
    float moveDelta;
    int decimalLoc;
    u8 type; // 0 -> text, 1 -> number
    u8 decimals;
} InputText;

void InputText_Init(InputText* text)
{
    text->capacity = 64;
    text->buffer = malloc(text->capacity);
    text->buffer[0] = '\0';
    text->numBytes = 0;
    text->length = 0;
    text->cursor = 0;
    text->cursorOffset = 0.0f;
    text->textOffset = 0.0f;
    text->moveDelta = 0.0f;
    text->decimalLoc = -1;
    text->type = 0;
    text->decimals = UINT8_MAX;
}

void InputText_Free(InputText* text)
{
    free(text->buffer);
    text->buffer = NULL;
}

// writes to text at cursor position
void InputText_Write(InputText* text, const char* string)
{
    // if stores numeric value
    if (text->type == 1) {
        if (string[0] == '.' && text->decimalLoc != -1) return; // decimal already exists
        else if (string[0] == '.' && text->decimalLoc == -1) {
            text->decimalLoc = text->length;
        }
        else if (string[0] > '9' || string[0] < '0') return; // only accept digits
        else if (text->decimalLoc != -1 && text->length - text->decimalLoc > text->decimals) return; // enforce decimal count
    }

    u32 stringLen = strlen(string);
    u32 required = text->numBytes + stringLen + 1;
    if (required > text->capacity) {
        text->capacity *= 2;
        if (text->capacity < required) text->capacity = required;
        text->buffer = realloc(text->buffer, text->capacity);
    }
    memmove(
        text->buffer + text->cursor + stringLen,
        text->buffer + text->cursor,
        text->numBytes - text->cursor + 1
    );
    memcpy(text->buffer + text->cursor, string, stringLen);
    text->numBytes += stringLen;
    text->cursor += stringLen;
    text->buffer[text->numBytes] = '\0';
    text->moveDelta = 1.0f;
    text->length++;
}

// removes one char at cursor position
void InputText_Backspace(InputText* text)
{
    if (text->cursor == 0) return;

    // if stores numeric value
    if (text->type == 1) {
        if (text->buffer[text->cursor-1] == '.') {
            text->decimalLoc = -1;
        }
    }

    // move to start of previous UTF-8 codepoint
    u32 i = text->cursor - 1;  // step left once
    while (i > 0 && ((unsigned char)text->buffer[i] & 0xC0) == 0x80) {
        i--;
    }
    u32 bytes = text->cursor - i;
    memmove(
        text->buffer + i,
        text->buffer + text->cursor,
        text->numBytes - text->cursor + 1
    );
    text->cursor -= bytes;
    text->numBytes -= bytes;
    text->moveDelta = -1.0f;
    text->length--;
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
    text->moveDelta = -1.0f;

    // back over continuation bytes
    while (text->cursor > 0 && ((unsigned char)text->buffer[text->cursor] & 0xC0) == 0x80)
        text->cursor--;
}


inline void InputText_MoveCursorRight(InputText* text)
{
    if (text->cursor >= text->numBytes)
        return;

    unsigned char c = (unsigned char)text->buffer[text->cursor];

    // ASCII or start of UTF-8 codepoint
    if ((c & 0x80) == 0) {
        text->cursor += 1; // ASCII
    } else {
        // UTF-8 multi-byte: skip continuation bytes
        text->cursor += 1;
        while (text->cursor < text->numBytes && ((unsigned char)text->buffer[text->cursor] & 0xC0) == 0x80)
            text->cursor++;
    }

    text->moveDelta = 1.0f;
}