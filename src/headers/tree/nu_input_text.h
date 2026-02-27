#include <stdint.h>
#include <string.h>
typedef int8_t i8;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Updates the cursorOffset and textOffset
void InputText_UpdateCusorTextOffsets(InputText* text, NodeP* node, NU_Font* font, i8 moveDelta) 
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

// Updates the highlightOffset and textOffset
void InputText_UpdateHighlightTextOffsets(InputText* text, NodeP* node, NU_Font* font, i8 moveDelta) 
{
    // calculate text width from start to cursor
    char temp = text->buffer[text->highlightBytes];
    text->buffer[text->highlightBytes] = '\0';
    float preHighlightTextWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
    text->buffer[text->highlightBytes] = temp;

    // calculate inner input width
    float innerWidth = node->node.width
        - node->node.borderLeft
        - node->node.borderRight
        - node->node.padLeft
        - node->node.padRight;

    // set relative highlight offset
    text->highlightOffset = text->textOffset + preHighlightTextWidth;

    // overflow correction
    if (moveDelta < 0) {
        if (text->highlightOffset < 0.0f) {
            float overshoot = -text->highlightOffset;
            text->highlightOffset = 0.0f;
            text->textOffset += overshoot;
        }
    }
    else if (moveDelta > 0) {
        if (text->highlightOffset > innerWidth) {
            float overshoot = text->highlightOffset - innerWidth;
            text->highlightOffset = innerWidth;
            text->highlightOffset -= overshoot;
        }
    }
}

// Moves cursor to the end of the text, highlights everything
void InputText_Focus(InputText* text, NodeP* node, NU_Font* font)
{
    // text->cursorBytes = text->numBytes;
    text->highlightBytes = 0;
    text->dragging = true;
}

// Resets the cursor position, cursor offset and text offset
void InputText_Defocus(InputText* text)
{
    text->textOffset = 0.0f;
    text->cursorBytes = 0;
    text->highlightBytes = 0;
    text->dragging = false;
}

void InputText_MouseUp(InputText* text)
{
    text->dragging = false;
}

inline int InputText_IsHighlighting(InputText* text)
{
    return text->cursorBytes != text->highlightBytes;
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
    text->highlightBytes = text->cursorBytes;
    text->buffer[text->numBytes] = '\0';
    InputText_UpdateCusorTextOffsets(text, node, font, 1);
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
    text->highlightBytes = text->cursorBytes;
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
    InputText_UpdateCusorTextOffsets(text, node, font, -1);
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
    text->highlightBytes = text->cursorBytes;
    InputText_UpdateCusorTextOffsets(text, node, font, -1);
    return 1;
}

int InputText_RemoveHighlightedText(InputText* text, NodeP* node, NU_Font* font)
{
    // Compute highlight start end
    u16 highlightStart = text->highlightBytes;
    u16 highlightEnd = text->cursorBytes;
    if (text->cursorBytes < highlightStart) {
        highlightStart = text->cursorBytes;
        highlightEnd = text->highlightBytes;
    }

    // Case 1: there are no bytes to the right of highlight end
    if (highlightEnd == text->numBytes) {
        text->numBytes -= (highlightEnd - highlightStart);
        text->buffer[text->numBytes] = '\0';
    }
    // Case 2: there are bytes to the right of highlight end
    else {
        u16 bytesToShift = text->numBytes - highlightEnd;
        memmove(text->buffer + highlightStart, text->buffer + highlightEnd, bytesToShift);
        text->numBytes -= highlightEnd - highlightStart;
        text->buffer[text->numBytes] = '\0';
    }

    // If is number input and deleted | updated the decimal
    if (text->type == 1 && text->decimalByteIndex != -1) {
        if (text->decimalByteIndex >= highlightStart && text->decimalByteIndex < highlightEnd) {
            text->decimalByteIndex = -1;
        }
        else if (text->decimalByteIndex >= highlightEnd) {
            text->decimalByteIndex -= (highlightEnd - highlightStart);
        }
    }

    // Update book-keeping
    text->highlightBytes = highlightStart;
    text->cursorBytes = highlightStart;
    InputText_UpdateCusorTextOffsets(text, node, font, -1);
    return 1;
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
     InputText_UpdateCusorTextOffsets(text, node, font, -1);\
     text->highlightBytes = text->cursorBytes;
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
    text->highlightBytes = text->cursorBytes;
    InputText_UpdateCusorTextOffsets(text, node, font, 1);
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
    text->highlightBytes = text->cursorBytes;
    InputText_UpdateCusorTextOffsets(text, node, font, -1);
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
    text->highlightBytes = text->cursorBytes;
    InputText_UpdateCusorTextOffsets(text, node, font, 1);
    return 1;
}

void InputText_MousePlaceCursor(InputText* text, NodeP* node, NU_Font* font, float mouseX)
{
    float inputLeftX = node->node.x + node->node.padLeft + node->node.borderLeft;
    float mouseRelX = mouseX - inputLeftX - text->textOffset;

    // Loop over glyphs in text, computing the glyph's x and width given the textOffset
    // If mouseX >= glyphX + glyphWidth * 0.5f && mouseX < nextGlyphX + nextGlyphWidth * 0.5f;
    u32 cursorBytes = NU_Calculate_Unwrapped_Text_Cursorbytes(font, text->buffer, mouseRelX);
    if (cursorBytes < text->cursorBytes) {
        text->cursorBytes = (u16)cursorBytes;
        InputText_UpdateCusorTextOffsets(text, node, font, -1);
    } 
    else {
        text->cursorBytes = (u16)cursorBytes;
        InputText_UpdateCusorTextOffsets(text, node, font, 1);
    }
    text->highlightBytes = text->cursorBytes;
}

int InputText_MouseDrag(InputText* text, NodeP* node, NU_Font* font, float mouseX)
{
    if (!text->dragging) return 0;

    float inputLeftX = node->node.x + node->node.padLeft + node->node.borderLeft;
    float mouseRelX = mouseX - inputLeftX - text->textOffset;

    u16 prevHighlightBytes = text->highlightBytes;

    // Loop over glyphs in text, computing the glyph's x and width given the textOffset
    // If mouseX >= glyphX + glyphWidth * 0.5f && mouseX < nextGlyphX + nextGlyphWidth * 0.5f;
    u32 highlightBytes = NU_Calculate_Unwrapped_Text_Cursorbytes(font, text->buffer, mouseRelX);
    if (highlightBytes < text->highlightBytes) {
        text->highlightBytes = (u16)highlightBytes;
        InputText_UpdateHighlightTextOffsets(text, node, font, -1);
    } 
    else {
        text->highlightBytes = (u16)highlightBytes;
        InputText_UpdateHighlightTextOffsets(text, node, font, 1);
    }
    return prevHighlightBytes != text->highlightBytes;
}

inline void InputText_CopyToClipboard(InputText* text)
{
    // Compute highlight start and end
    u16 highlightStart = text->highlightBytes;
    u16 highlightEnd = text->cursorBytes;
    if (text->cursorBytes < highlightStart) {
        highlightStart = text->cursorBytes;
        highlightEnd = text->highlightBytes;
    }

    // Nothing selected? Don't copy
    if (highlightStart == highlightEnd) return;

    // Temporarily null-terminate the highlight region
    char temp = text->buffer[highlightEnd];
    text->buffer[highlightEnd] = '\0';

    SDL_SetClipboardText(text->buffer + highlightStart);

    // Restore original character
    text->buffer[highlightEnd] = temp;
}

inline void InputText_PasteFromClipboard(InputText* text, NodeP* node, NU_Font* font)
{
    // Get clipboard text
    char* clip = SDL_GetClipboardText();
    if (!clip || clip[0] == '\0') return;  // nothing to paste

    // Compute length of valid characters
    int validBytes = 0;
    int hasDecimal = (text->type == 1 && text->decimalByteIndex != -1);

    // Scan clipboard once to count valid bytes
    for (char* p = clip; *p; )
    {
        unsigned char c = (unsigned char)*p;

        if (text->type == 0) // text input: accept everything
        {
            validBytes++;
        }
        else if (text->type == 1) // number input
        {
            if (c >= '0' && c <= '9')
            {
                validBytes++;
            }
            else if (c == '.' && !hasDecimal)
            {
                validBytes++;
                hasDecimal = 1; // mark decimal will be inserted
            }
        }

        p++;
    }

    if (validBytes == 0)
    {
        SDL_free(clip);
        return; // nothing valid to paste
    }

    // Allocate buffer if needed
    int required = text->numBytes + validBytes + 1;
    if (required > text->capacity)
    {
        text->capacity *= 2;
        if (text->capacity < required) text->capacity = required;
        text->buffer = realloc(text->buffer, text->capacity);
    }

    // Shift existing text to make space
    memmove(
        text->buffer + text->cursorBytes + validBytes,
        text->buffer + text->cursorBytes,
        text->numBytes - text->cursorBytes + 1
    );

    // Copy valid characters into the gap
    int dstIndex = text->cursorBytes;
    hasDecimal = (text->type == 1 && text->decimalByteIndex != -1); // reset for insertion

    for (char* p = clip; *p; )
    {
        unsigned char c = (unsigned char)*p;

        if (text->type == 0) // text input: accept everything
        {
            text->buffer[dstIndex++] = c;
        }
        else if (text->type == 1) // number input
        {
            if (c >= '0' && c <= '9')
            {
                text->buffer[dstIndex++] = c;
            }
            else if (c == '.' && !hasDecimal)
            {
                text->buffer[dstIndex++] = '.';
                text->decimalByteIndex = dstIndex - 1;
                hasDecimal = 1;
            }
        }

        p++;
    }

    // Update text state
    int inserted = dstIndex - text->cursorBytes;
    text->numBytes += inserted;
    text->cursorBytes += inserted;
    text->highlightBytes = text->cursorBytes;
    text->buffer[text->numBytes] = '\0';

    SDL_free(clip);
    InputText_UpdateCusorTextOffsets(text, node, font, 1);
}