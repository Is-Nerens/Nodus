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
    u32 xOffset;
} InputText;

// writes to text at cursor position
void InputText_Write(InputText* text, char* string)
{
    u32 stringLen = strlen(string);
    text->buffer = realloc(text->buffer, text->length + stringLen + 1);
    memcpy(text->buffer + text->length, string, stringLen);
}

// removes one char at cursor position
void InputText_Backspace(InputText* text)
{

}

// replaces highlighted section with a new string
void InputText_ReplaceSlice(InputText* text, char* string, u32 index, u32 numChars)
{

}

// removes a highlighted section
void InputText_RemoveSlice(InputText* text, u32 index, u32 numChars)
{

}

inline void InputText_MoveCursorRight(InputText* text)
{
    
}

inline void InputText_MoveCursorLeft(InputText* text)
{
    
}