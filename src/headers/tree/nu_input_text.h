#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct InputText {
    char* buffer;
    float cursorOffset;
    float textOffset;
    float moveDelta;
    int decimalLoc;
    u16 numBytes;
    u16 length;
    u16 capacity;
    u16 cursorBytes;
    u16 cursorIndex;
    u8 type; // 0 -> text, 1 -> number
} InputText;

void InputText_Init(InputText* text)
{
    text->capacity = 64;
    text->buffer = malloc(text->capacity);
    text->buffer[0] = '\0';
    text->numBytes = 0;
    text->length = 0;
    text->cursorBytes = 0;
    text->cursorIndex = 0;
    text->cursorOffset = 0.0f;
    text->textOffset = 0.0f;
    text->moveDelta = 0.0f;
    text->decimalLoc = -1;
    text->type = 0;
}

void InputText_Free(InputText* text)
{
    free(text->buffer);
    text->buffer = NULL;
}

void InputText_Defocus(InputText* text)
{
    text->cursorOffset = 0.0f;
    text->textOffset = 0.0f;
    text->moveDelta = 0.0f;
    text->cursorBytes = 0;
    text->cursorIndex = 0;
}

// writes to text at cursorBytes position -> returns 1 if updated
int InputText_Write(InputText* text, const char* string)
{
    // if stores numeric value
    if (text->type == 1) {
        if (string[0] == '.' && text->decimalLoc != -1) return 0; // decimal already exists
        else if (string[0] == '.' && text->decimalLoc == -1) {
            text->decimalLoc = text->cursorIndex;
        }
        else if (string[0] > '9' || string[0] < '0') return 0; // only accept digits
    }

    u32 stringLen = strlen(string);
    u32 required = text->numBytes + stringLen + 1;
    if (required > text->capacity) {
        text->capacity *= 2;
        if (text->capacity < required) text->capacity = required;
        text->buffer = realloc(text->buffer, text->capacity);
    }
    memmove(
        text->buffer + text->cursorBytes + stringLen,
        text->buffer + text->cursorBytes,
        text->numBytes - text->cursorBytes + 1
    );
    memcpy(text->buffer + text->cursorBytes, string, stringLen);
    text->numBytes += stringLen;
    text->cursorBytes += stringLen;
    text->buffer[text->numBytes] = '\0';
    text->moveDelta = 1.0f;
    text->cursorIndex++;
    text->length++;
    return 1;
}

// removes one char at cursorBytes position -> returns 1 if updated
int InputText_Backspace(InputText* text)
{
    if (text->cursorBytes == 0) return 0;

    // if stores numeric value
    if (text->type == 1 && text->buffer[text->cursorBytes-1] == '.') {
        text->decimalLoc = -1;
    }

    // move to start of previous UTF-8 codepoint
    u32 i = text->cursorBytes - 1;  // step left once
    while (i > 0 && ((unsigned char)text->buffer[i] & 0xC0) == 0x80) {
        i--;
    }
    u32 bytes = text->cursorBytes - i;
    memmove(
        text->buffer + i,
        text->buffer + text->cursorBytes,
        text->numBytes - text->cursorBytes + 1
    );
    text->cursorBytes -= bytes;
    text->numBytes -= bytes;
    text->moveDelta = -1.0f;
    text->cursorIndex--;
    text->length--;
    return 1;
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
    if (text->cursorBytes == 0)
        return;

    text->cursorBytes--;  // step left at least once
    text->moveDelta = -1.0f;

    // back over continuation bytes
    while (text->cursorBytes > 0 && ((unsigned char)text->buffer[text->cursorBytes] & 0xC0) == 0x80)
        text->cursorBytes--;

    text->cursorIndex--;
}


inline void InputText_MoveCursorRight(InputText* text)
{
    if (text->cursorBytes >= text->numBytes)
        return;

    unsigned char c = (unsigned char)text->buffer[text->cursorBytes];

    // ASCII or start of UTF-8 codepoint
    if ((c & 0x80) == 0) {
        text->cursorBytes += 1; // ASCII
    } else {
        // UTF-8 multi-byte: skip continuation bytes
        text->cursorBytes += 1;
        while (text->cursorBytes < text->numBytes && ((unsigned char)text->buffer[text->cursorBytes] & 0xC0) == 0x80)
            text->cursorBytes++;
    }

    text->cursorIndex++;
    text->moveDelta = 1.0f;
}