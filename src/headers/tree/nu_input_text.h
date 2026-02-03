#include <stdint.h>
#include <string.h>
typedef int8_t i8;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Updates the cursorOffset and textOffset
void InputText_UpdateOffsets(InputText* text, NodeP* node, NU_Font* font, i8 moveDelta) 
{
    // calculate text width from start to cursor
    char temp = text->buffer[text->cursorBytes];
    text->buffer[text->cursorBytes] = '\0';
    float preCursorTextWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
    text->buffer[text->cursorBytes] = temp;

    // calculate inner input width
    float innerWidth = node->node.width
        - node->node.borderLeft
        - node->node.borderRight
        - node->node.padLeft
        - node->node.padRight;

    // set relative cursor offset
    text->cursorOffset = text->textOffset + preCursorTextWidth;

    // overflow correction
    if (moveDelta < 0) {
        if (text->cursorOffset < 0.0f) {
            float overshoot = -text->cursorOffset;
            text->cursorOffset = 0.0f;
            text->textOffset += overshoot;
        }
    }
    else if (moveDelta > 0) {
        if (text->cursorOffset > innerWidth) {
            float overshoot = text->cursorOffset - innerWidth;
            text->cursorOffset = innerWidth;
            text->textOffset -= overshoot;
        }
    }
}

// Moves cursor to the end of the text, highlights everything
void InputText_Focus(InputText* text)
{
    text->cursorBytes = text->numBytes;
    text->highlightBytes = 0;
}

// Resets the cursor position, cursor offset and text offset
void InputText_Defocus(InputText* text)
{
    text->cursorOffset = 0.0f;
    text->textOffset = 0.0f;
    text->cursorBytes = 0;
    text->highlightBytes = text->cursorBytes;
}

// Writes to text at cursorBytes position -> returns 1 if updated
int InputText_Write(InputText* text, NodeP* node, NU_Font* font, const char* string)
{
    // if stores numeric value
    if (text->type == 1) {
        if (string[0] == '.' && text->decimalByteIndex != -1) return 0; // decimal already exists
        else if (string[0] == '.' && text->decimalByteIndex == -1) {
            text->decimalByteIndex = text->cursorBytes;
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
    if (text->type == 1 && text->decimalByteIndex != -1 && text->decimalByteIndex > text->cursorBytes) {
        text->decimalByteIndex += stringLen;
    }
    text->numBytes += stringLen;
    text->cursorBytes += stringLen;
    text->buffer[text->numBytes] = '\0';
    InputText_UpdateOffsets(text, node, font, 1);
    return 1;
}

/* Unsafe function to remove the codepoint immediately preceeding the cursor
   IMPORTANT! This function assumes there is / are codepoint(s) to the left of the cursor */
void InputText_RemoveCodepointBeforeCursor(InputText* text)
{
    u32 i = text->cursorBytes - 1;
    while (i > 0 && ((unsigned char)text->buffer[i] & 0xC0) == 0x80) i--;
    u32 bytes = text->cursorBytes - i;
    memmove(
        text->buffer + i,
        text->buffer + text->cursorBytes,
        text->numBytes - text->cursorBytes + 1);
    if (text->type == 1 && text->decimalByteIndex != -1 && text->decimalByteIndex > text->cursorBytes) {
        text->decimalByteIndex -= bytes;
    }
    text->cursorBytes -= bytes;
    text->numBytes -= bytes;
    text->buffer[text->numBytes] = '\0';
}

// Removes one char at cursorBytes position -> returns 1 if updated
int InputText_Backspace(InputText* text, NodeP* node, NU_Font* font)
{
    if (text->cursorBytes == 0) return 0;

    // if stores numeric value
    if (text->type == 1 && text->buffer[text->cursorBytes-1] == '.') {
        text->decimalByteIndex = -1;
    }

    // move to start of previous UTF-8 codepoint
    InputText_RemoveCodepointBeforeCursor(text);
     InputText_UpdateOffsets(text, node, font, -1);
    return 1;
}

// Removes one word before cursorBytes position -> returns 1 if updated
int InputText_BackspaceWord(InputText* text, NodeP* node, NU_Font* font)
{
    if (text->cursorBytes == 0) return 0;

    // stores numeric value
    if (text->type == 1) {

        // case 1: no decimal xxx|xxx
        if (text->decimalByteIndex == -1) {
            u32 bytesToRemove = text->cursorBytes;
            memmove(
                text->buffer,
                text->buffer + text->cursorBytes,
                text->numBytes - text->cursorBytes + 1
            );
            text->cursorBytes = 0;
            text->numBytes   -= bytesToRemove;
            text->buffer[text->numBytes] = '\0';
        }
        // case 2: cursor is before decimal  xxx|xxx.xxx
        else if (text->cursorBytes-1 < text->decimalByteIndex) {
            u32 bytesToRemove = text->cursorBytes;
            memmove(
                text->buffer,
                text->buffer + text->cursorBytes,
                text->numBytes - text->cursorBytes + 1
            );
            text->cursorBytes = 0;
            text->decimalByteIndex -= bytesToRemove;
            text->numBytes   -= bytesToRemove;
            text->buffer[text->numBytes] = '\0';
        }
        // case 3: decimal is just before cursor xxx.|
        else if (text->decimalByteIndex == text->cursorBytes-1) {
            InputText_RemoveCodepointBeforeCursor(text);
            text->decimalByteIndex = -1;
        }
        // case 4: cursor after decimal xxx.xxx|xx
        else {
            u32 startByte = text->decimalByteIndex + 1;
            u32 bytesToRemove = text->cursorBytes - startByte;
            memmove(
                text->buffer + startByte,
                text->buffer + text->cursorBytes,
                text->numBytes - text->cursorBytes + 1
            );
            text->cursorBytes = startByte;
            text->numBytes   -= bytesToRemove;
            text->buffer[text->numBytes] = '\0';
        }
    }
    // stores text value
    else {

        // iterate backwards until the beginning of the string OR a space is found and the prev char is not a space
        u32 bytesStart = text->cursorBytes - 1;
        while (bytesStart > 0 && text->buffer[bytesStart - 1] == ' ') bytesStart--; // 1. skip spaces
        while (bytesStart > 0 && text->buffer[bytesStart - 1] != ' ') bytesStart--; // 2. skip the word
        u32 bytesToRemove = text->cursorBytes - bytesStart;

        // perform removal
        memmove(
            text->buffer + bytesStart,
            text->buffer + text->cursorBytes,
            text->numBytes - text->cursorBytes + 1
        );
        text->cursorBytes -= bytesToRemove;
        text->numBytes   -= bytesToRemove;
        text->buffer[text->numBytes] = '\0';
    }
    InputText_UpdateOffsets(text, node, font, -1);
    return 1;
}

// Replaces highlighted section with a new string
void InputText_ReplaceSlice(InputText* text, char* string, u32 index, u32 numChars)
{
    
}

// Removes a highlighted section
void InputText_RemoveSlice(InputText* text, u32 index, u32 numChars)
{

}

// Moves the cursor on space to the left -> returns 1 if cursor moved
int InputText_MoveCursorLeft(InputText* text, NodeP* node, NU_Font* font)
{
    // can't move any further left
    if (text->cursorBytes == 0) return 0;

    // move to previous UTF8 codepoint
    text->cursorBytes--;
    while (text->cursorBytes > 0 && ((unsigned char)text->buffer[text->cursorBytes] & 0xC0) == 0x80)
        text->cursorBytes--;

    // update book-keeping and return
     InputText_UpdateOffsets(text, node, font, -1);
    return 1;
}

// Moves the cursor on space to the right -> returns 1 if cursor moved
int InputText_MoveCursorRight(InputText* text, NodeP* node, NU_Font* font)
{
    // can't move any further right
    if (text->cursorBytes >= text->numBytes) return 0;

    // advance to next UTF8 codepoint
    text->cursorBytes++;
    while (text->cursorBytes < text->numBytes &&
       ((unsigned char)text->buffer[text->cursorBytes] & 0xC0) == 0x80) {
        text->cursorBytes++;
    }

    // update book-keeping and return
     InputText_UpdateOffsets(text, node, font, 1);
    return 1;
}

// Moves the cursor on space to the left of the preceeding word -> returns 1 if cursor moved
int InputText_MoveCursorLeftSpan(InputText* text, NodeP* node, NU_Font* font)
{
    // can't move any further left
    if (text->cursorBytes == 0) return 0;
    
    // stores numeric value
    if (text->type == 1)
    {
        // case 1: no decimal xxx|xxx OR cursor is before decimal  xxx|xxx.xxx
        if (text->decimalByteIndex == -1 || text->cursorBytes-1 < text->decimalByteIndex) {
            text->cursorBytes = 0;
        }
        // case 2: decimal is just before cursor xxx.|
        else if (text->decimalByteIndex == text->cursorBytes-1) {
            text->cursorBytes--;
        }
        // case 3: cursor after decimal xxx.xxx|xx
        else {
            text->cursorBytes = text->decimalByteIndex + 1;
        }
    }
    // stores text value
    else
    {
        u32 bytesStart = text->cursorBytes - 1;
        while (bytesStart > 0 && text->buffer[bytesStart - 1] == ' ') bytesStart--; // 1. skip spaces
        while (bytesStart > 0 && text->buffer[bytesStart - 1] != ' ') bytesStart--; // 2. skip the word
        text->cursorBytes -= text->cursorBytes - bytesStart;
    }
     InputText_UpdateOffsets(text, node, font, -1);
    return 1;
}

// Moves the cursor on space to the right of the proceeding word -> returns 1 if cursor moved
int InputText_MoveCursorRightSpan(InputText* text, NodeP* node, NU_Font* font)
{
    // can't move any further right
    if (text->cursorBytes >= text->numBytes) return 0;

    // stores numeric value
    if (text->type == 1)
    {
        // case 1: no decimal xxx|xxx
        if (text->decimalByteIndex == -1) {
            text->cursorBytes = text->numBytes;
        }
        // case 2: cursor is before decimal  xxx|xxx.xxx
        else if (text->cursorBytes < text->decimalByteIndex) {
            text->cursorBytes = text->decimalByteIndex;
        }
        // case 3: cursor is just before decimal xxx|.xxx
        else if (text->cursorBytes == text->decimalByteIndex) {
            text->cursorBytes++;
        }
        // case 4: cursor after decimal xxx.xxx|xx
        else {
            text->cursorBytes = text->numBytes;
        }
    }   
    // stores text value
    else
    {
        u32 bytesStart = text->cursorBytes + 1;
        while (bytesStart < text->numBytes && text->buffer[bytesStart] == ' ') bytesStart++; // 1. skip spaces
        while (bytesStart < text->numBytes && text->buffer[bytesStart] != ' ') bytesStart++; // 2. skip the word
        text->cursorBytes = bytesStart;
    }
    InputText_UpdateOffsets(text, node, font, 1);
    return 1;
}