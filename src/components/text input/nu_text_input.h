#pragma once
#include "nu_text_input_struct.h"

// Updates the cursorOffset and textOffset
// Updates the cursorOffset and textOffset
void InputText_UpdateCusorTextOffsets(TextInput* text, Node* node, Font* font)
{
    // calculate text width from start to cursor
    char temp = text->buffer[text->cursorBytes];
    text->buffer[text->cursorBytes] = '\0';
    float32 preCursorTextWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
    text->buffer[text->cursorBytes] = temp;

    // calculate inner input width
    float32 innerWidth = NODE_INNER_WIDTH(node);

    // set relative cursor offset
    text->cursorOffset = text->textOffset + preCursorTextWidth;

    // overflow correction
    if (text->cursorOffset < 0.0f) {
        float32 overshoot = -text->cursorOffset;
        text->cursorOffset = 0.0f;
        text->textOffset += overshoot;
    }
    else if (text->cursorOffset > innerWidth) {
        float32 overshoot = text->cursorOffset - innerWidth;
        text->cursorOffset = innerWidth;
        text->textOffset -= overshoot;
    }
}

void InputText_ComputeCursorTextOffset_PlaceEnd(TextInput* text, Node* node, Font* font)
{
    // Compute offsets
    float32 textWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
    float32 innerWidth = node->base.width
        - node->base.borderLeft
        - node->base.borderRight
        - node->base.padLeft
        - node->base.padRight;
    if (textWidth > innerWidth) text->textOffset = innerWidth - textWidth;
    else text->textOffset = 0.0f;
    text->cursorOffset = textWidth + text->textOffset;
}

// Resets the cursor position, cursor offset and text offset
void InputText_Defocus(TextInput* text)
{
    text->dragging = false;
}

void InputText_MouseUp(TextInput* text)
{
    text->dragging = false;
}

inline int InputText_IsHighlighting(TextInput* text)
{
    return text->cursorBytes != text->highlightBytes;
}

// Remove highlited text -> returns true if updated
int InputText_RemoveHighlightedText(TextInput* text, Node* node, Font* font)
{
    // Compute highlight start end
    uint16 highlightStart = text->highlightBytes;
    uint16 highlightEnd = text->cursorBytes;
    if (text->cursorBytes < highlightStart) {
        highlightStart = text->cursorBytes;
        highlightEnd = text->highlightBytes;
    }

    // Nothing to remove
    if (highlightStart == highlightEnd) return false;

    // Case 1: there are no bytes to the right of highlight end
    if (highlightEnd == text->numBytes) {
        text->numBytes -= (highlightEnd - highlightStart);
        text->buffer[text->numBytes] = '\0';
    }
    // Case 2: there are bytes to the right of highlight end
    else {
        uint16 bytesToShift = text->numBytes - highlightEnd;
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
    InputText_UpdateCusorTextOffsets(text, node, font);
    return true;
}

// Writes to text at cursorBytes position -> returns 1 if updated
bool InputText_Write(TextInput* text, Node* node, Font* font, const char* string)
{
    bool changed = false;

    // Remove highlighted text first
    if (InputText_IsHighlighting(text)) {
        changed |= InputText_RemoveHighlightedText(text, node, font);
    }

    // if stores numeric value
    if (text->type == 1) {
        if (string[0] == '.' && text->decimalByteIndex != -1) return 0; // decimal already exists
        else if (string[0] == '.' && text->decimalByteIndex == -1) {
            text->decimalByteIndex = text->cursorBytes;
        }
        else if (string[0] > '9' || string[0] < '0') return changed; // only accept digits
    }
    uint32 stringLen = strlen(string);
    uint32 required = text->numBytes + stringLen + 1;
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
    InputText_UpdateCusorTextOffsets(text, node, font);
    return true;
}

// Removes codepoint immediately preceeding the cursor -> returns true if updated
bool InputText_RemoveCodepointBeforeCursor(TextInput* text)
{
    if (text->cursorBytes == 0) return false;
    uint32 i = text->cursorBytes - 1;
    while (i > 0 && ((unsigned char)text->buffer[i] & 0xC0) == 0x80) i--;
    uint32 bytes = text->cursorBytes - i;
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
    return true;
}

// Removes one char at cursorBytes position -> returns true if updated
int InputText_Backspace(TextInput* text, Node* node, Font* font)
{
    if (text->cursorBytes == 0) return 0;

    // If stores numeric value
    if (text->type == 1 && text->buffer[text->cursorBytes-1] == '.') {
        text->decimalByteIndex = -1;
    }

    // Move to start of previous UTF-8 codepoint
    InputText_RemoveCodepointBeforeCursor(text);
    InputText_UpdateCusorTextOffsets(text, node, font);
    return 1;
}

// Removes one word before cursorBytes position -> returns tue if updated
bool InputText_BackspaceWord(TextInput* text, Node* node, Font* font)
{
    if (text->cursorBytes == 0) return false;

    // stores numeric value
    if (text->type == 1) {

        // case 1: no decimal xxx|xxx
        if (text->decimalByteIndex == -1) {
            uint32 bytesToRemove = text->cursorBytes;
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
            uint32 bytesToRemove = text->cursorBytes;
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
            uint32 startByte = text->decimalByteIndex + 1;
            uint32 bytesToRemove = text->cursorBytes - startByte;
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
        uint32 bytesStart = text->cursorBytes - 1;
        while (bytesStart > 0 && text->buffer[bytesStart - 1] == ' ') bytesStart--; // 1. skip spaces
        while (bytesStart > 0 && text->buffer[bytesStart - 1] != ' ') bytesStart--; // 2. skip the word
        uint32 bytesToRemove = text->cursorBytes - bytesStart;

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
    InputText_UpdateCusorTextOffsets(text, node, font);
    return true;
}

// Moves the cursor on space to the left -> returns 1 if cursor moved
int InputText_MoveCursorLeft(TextInput* text, Node* node, Font* font)
{
    // can't move any further left
    if (text->cursorBytes == 0) return 0;

    // move to previous UTF8 codepoint
    text->cursorBytes--;
    while (text->cursorBytes > 0 && ((unsigned char)text->buffer[text->cursorBytes] & 0xC0) == 0x80)
        text->cursorBytes--;

    // update book-keeping and return
    InputText_UpdateCusorTextOffsets(text, node, font);
    text->highlightBytes = text->cursorBytes;
    return 1;
}

// Moves the cursor on space to the right -> returns 1 if cursor moved
int InputText_MoveCursorRight(TextInput* text, Node* node, Font* font)
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
    InputText_UpdateCusorTextOffsets(text, node, font);
    return 1;
}

// Moves the cursor on space to the left of the preceeding word -> returns 1 if cursor moved
int InputText_MoveCursorLeftSpan(TextInput* text, Node* node, Font* font)
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
        uint32 bytesStart = text->cursorBytes - 1;
        while (bytesStart > 0 && text->buffer[bytesStart - 1] == ' ') bytesStart--; // 1. skip spaces
        while (bytesStart > 0 && text->buffer[bytesStart - 1] != ' ') bytesStart--; // 2. skip the word
        text->cursorBytes -= text->cursorBytes - bytesStart;
    }
    text->highlightBytes = text->cursorBytes;
    InputText_UpdateCusorTextOffsets(text, node, font);
    return 1;
}

// Moves the cursor on space to the right of the proceeding word -> returns 1 if cursor moved
int InputText_MoveCursorRightSpan(TextInput* text, Node* node, Font* font)
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
        uint32 bytesStart = text->cursorBytes + 1;
        while (bytesStart < text->numBytes && text->buffer[bytesStart] == ' ') bytesStart++; // 1. skip spaces
        while (bytesStart < text->numBytes && text->buffer[bytesStart] != ' ') bytesStart++; // 2. skip the word
        text->cursorBytes = bytesStart;
    }
    text->highlightBytes = text->cursorBytes;
    InputText_UpdateCusorTextOffsets(text, node, font);
    return 1;
}

void InputText_MousePlaceCursor(TextInput* text, Node* node, Font* font, float32 mouseX, bool doubleClick)
{
    float32 inputLeftX = node->base.x + node->base.padLeft + node->base.borderLeft;
    float32 mouseRelX = mouseX - inputLeftX - text->textOffset;

    uint32 cursorBytes = NU_Calculate_Unwrapped_Text_Cursorbytes(font, text->buffer, mouseRelX);
    if (cursorBytes < text->cursorBytes) {
        text->cursorBytes = (uint16)cursorBytes;
        InputText_UpdateCusorTextOffsets(text, node, font);
    }
    else {
        text->cursorBytes = (uint16)cursorBytes;
        InputText_UpdateCusorTextOffsets(text, node, font);
    }

    text->highlightBytes = text->cursorBytes;

    // Special double-click highlight
    if (doubleClick)
    {
        const uint32 n = text->numBytes;
        const uint32 clickPos = text->cursorBytes;  // nearest boundary, already half-snapped
        uint32 pos = clickPos;

        // Decide whether we're on a space or non-space. Handle the edge case
        // where the cursor sits right at a boundary by peeking both sides.
        bool onSpace;
        if (pos < n && text->buffer[pos] == ' ') {
            onSpace = true;
        } else if (pos > 0 && text->buffer[pos - 1] == ' ') {
            onSpace = true;
            pos = pos - 1;  // step onto the space run
        } else {
            onSpace = false;
            if (pos == n && n > 0) pos = n - 1;  // clicked past end -> sit on last char
        }

        uint32 start = pos;
        uint32 end = pos;

        if (onSpace) {
            // Find the full run of spaces
            while (start > 0 && text->buffer[start - 1] == ' ') start--;
            while (end < n && text->buffer[end] == ' ') end++;

            // Short run (< 3 spaces): select the nearer neighbouring word instead
            if (end - start < 3) {
                const uint32 runStart = start;
                const uint32 runEnd = end;
                const bool hasLeftWord  = (runStart > 0);
                const bool hasRightWord = (runEnd < n);

                bool pickLeft;
                if (hasLeftWord && hasRightWord) {
                    // Compare distance from the snapped click boundary to each edge of the run
                    pickLeft = (clickPos - runStart) <= (runEnd - clickPos);
                } else {
                    pickLeft = hasLeftWord;
                }

                if (hasLeftWord && pickLeft) {
                    end = runStart;
                    start = end;
                    while (start > 0 && text->buffer[start - 1] != ' ') start--;
                }
                else if (hasRightWord) {
                    start = runEnd;
                    end = start;
                    while (end < n && text->buffer[end] != ' ') end++;
                }
                // else: the whole string is a short run of spaces, keep the run selected
            }
        } else {
            // Clicked on a word: walk to both ends of it
            while (start > 0 && text->buffer[start - 1] != ' ') start--;
            while (end < n && text->buffer[end] != ' ') end++;
        }

        // Set selection: highlight at word start, cursor at word end
        text->highlightBytes = (uint16)start;
        text->cursorBytes    = (uint16)end;

        // Compute offsets directly rather than using the delta-based updaters,
        // since we jumped arbitrary distances.
        char temp = text->buffer[start];
        text->buffer[start] = '\0';
        float32 startWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
        text->buffer[start] = temp;

        temp = text->buffer[end];
        text->buffer[end] = '\0';
        float32 endWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
        text->buffer[end] = temp;

        text->highlightOffset = text->textOffset + startWidth;
        text->cursorOffset    = text->textOffset + endWidth;

        // Overflow correction: if the selection extends past the right edge,
        // shift textOffset so the end stays visible.
        float32 innerWidth = NODE_INNER_WIDTH(node);
        if (text->cursorOffset > innerWidth) {
            float32 overshoot = text->cursorOffset - innerWidth;
            text->cursorOffset    -= overshoot;
            text->highlightOffset -= overshoot;
            text->textOffset      -= overshoot;
        }
    }

    text->dragging = !doubleClick;
    text->lastDragTime = nu_time();
    text->dragStartMouseX = mouseX;
}

int InputText_MouseDrag(TextInput* text, Node* node, Font* font, float32 mouseX)
{
    if (!text->dragging) return 0;

    // Cache highlight bytes
    uint16 prevHighlightBytes = text->highlightBytes;
    float32 prevTextOffset = text->textOffset;

    // Calculate "frame" delta time
    uint64 lastTime = text->lastDragTime;
    uint64 time = nu_time();
    text->lastDragTime = time;
    uint64 dtMicroseconds = time - lastTime;
    float dtSeconds = fminf((float)(dtMicroseconds) / 1e6f, 1.0f / 30.0f);
    float scrollPixels = 600 * dtSeconds;

    // Calculate relative positions
    float32 inputLeftX = node->base.x + node->base.padLeft + node->base.borderLeft;
    float32 innerWidth = NODE_INNER_WIDTH(node);
    float32 wallLeft  = inputLeftX;
    float32 wallRight = inputLeftX + innerWidth;

    // Calculate number of pixels to offset by
    float32 offsetPixels = 0.0f;
    if (mouseX < wallLeft && text->textOffset < 0.0f) {
        offsetPixels = scrollPixels;
    }
    else if (mouseX > wallRight) {
        offsetPixels = -scrollPixels;
    }

    // Apply offset with clamping
    if (offsetPixels != 0.0f)
    {
        float32 textWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
        float32 minOffset = fminf(0.0f, innerWidth - textWidth);
        text->textOffset += offsetPixels;
        if (text->textOffset > 0.0f) text->textOffset = 0.0f;
        if (text->textOffset < minOffset) text->textOffset = minOffset;
    }

    // Recompute highlight bytes given the new offset
    float32 mouseRelX = mouseX - inputLeftX - text->textOffset;
    uint32 highlightBytes = NU_Calculate_Unwrapped_Text_Cursorbytes(font, text->buffer, mouseRelX);
    text->highlightBytes = (uint16)highlightBytes;

    // Calculate text width from start to (new) highlight bytes
    char temp = text->buffer[text->highlightBytes];
    text->buffer[text->highlightBytes] = '\0';
    float32 preHighlightTextWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
    text->buffer[text->highlightBytes] = temp;

    // Calculate text width from start to cursor bytes (stationary anchor)
    temp = text->buffer[text->cursorBytes];
    text->buffer[text->cursorBytes] = '\0';
    float32 preCursorTextWidth = NU_Calculate_Text_Unwrapped_Width(font, text->buffer);
    text->buffer[text->cursorBytes] = temp;

    // Derive offsets from new textOffset + prefix widths
    // (anchor may scroll off-screen during drag; that's correct)
    text->highlightOffset = text->textOffset + preHighlightTextWidth;
    if (text->highlightOffset < 0.0f) text->highlightOffset = 0.0f;
    if (text->highlightOffset > innerWidth) text->highlightOffset = innerWidth;

    text->cursorOffset = text->textOffset + preCursorTextWidth;
    if (text->cursorOffset < 0.0f) text->cursorOffset = 0.0f;
    if (text->cursorOffset > innerWidth) text->cursorOffset = innerWidth;

    return (text->highlightBytes != prevHighlightBytes) || (text->textOffset != prevTextOffset);
}

void InputText_SelectAll(TextInput* text, Font* font)
{
    text->highlightBytes = 0;
    text->cursorBytes = text->numBytes;
    text->highlightOffset = 0.0f;
    text->cursorOffset = NU_Calculate_Text_Unwrapped_Width(font, text->buffer) - text->textOffset;
}

void InputText_CopyToClipboard(TextInput* text)
{
    // Compute highlight start and end
    uint16 highlightStart = text->highlightBytes;
    uint16 highlightEnd = text->cursorBytes;
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

void InputText_SetText(TextInput* text, Node* node, Font* font, const char* str)
{
    if (!str) return;  // nothing to paste

    text->cursorBytes = 0;
    text->decimalByteIndex = -1;
    text->highlightBytes = 0;
    text->numBytes = 0;
    text->buffer[text->numBytes] = '\0';

    // If setting to an empty string -> done
    if (str[0] == '\0') {
        text->updateOffsetsPostLayout = true;
        return;
    }

    // Compute length of valid characters
    int validBytes = 0;
    int hasDecimal = (text->type == 1 && text->decimalByteIndex != -1);

    // Scan clipboard once to count valid bytes
    for (const char* p = str; *p; )
    {
        unsigned char c = (unsigned char)*p;

        if (text->type == 0) {// text input: accept everything
            validBytes++;
        }
        else if (text->type == 1) {// number input
            if (c >= '0' && c <= '9') {
                validBytes++;
            }
            else if (c == '.' && !hasDecimal) {
                validBytes++;
                hasDecimal = 1; // mark decimal will be inserted
            }
        }

        p++;
    }

    if (validBytes == 0) return; // nothing valid to paste

    // Allocate buffer if needed
    int required = text->numBytes + validBytes + 1;
    if (required > text->capacity) {
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

    for (const char* p = str; *p; )
    {
        unsigned char c = (unsigned char)*p;

        // text input: accept everything
        if (text->type == 0) {
            text->buffer[dstIndex++] = c;
        }
        else if (text->type == 1) // number input
        {
            if (c >= '0' && c <= '9') {
                text->buffer[dstIndex++] = c;
            }
            else if (c == '.' && !hasDecimal) {
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

    // Defer offsets calculation
    text->updateOffsetsPostLayout = true;
}

static inline bool InputText_PasteFromClipboard(TextInput* text, Node* node, Font* font)
{
    char* clip = SDL_GetClipboardText();
    if (!clip || clip[0] == '\0') {
        if (clip) SDL_free(clip);
        return false;
    }

    // Sanitize for numeric inputs: keep digits and at most one '.'
    // (only if the field doesn't already contain one).
    if (text->type == 1)
    {
        bool hasDecimal = (text->decimalByteIndex != -1);
        char* dst = clip;
        for (char* p = clip; *p; p++) {
            char c = *p;
            if (c >= '0' && c <= '9') {
                *dst++ = c;
            } else if (c == '.' && !hasDecimal) {
                *dst++ = '.';
                hasDecimal = true;
            }
        }
        *dst = '\0';

        // Selection was removed, nothing valid to insert
        if (clip[0] == '\0') {
            SDL_free(clip);
            return false;
        }
    }

    // Insert at cursor (which is now at the old selection start).
    bool changed = InputText_Write(text, node, font, clip);

    SDL_free(clip);
    return changed;
}
