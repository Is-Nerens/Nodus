#pragma once
#include <stdint.h>
#include <stdlib.h>

typedef struct ParserWord
{
    uint32_t length;
    char buffer[256];
} ParserWord;

void ParserWord_Init(ParserWord *word)
{
    word->length = 0;
    word->buffer[0] = '\0';
}

inline void ParserWord_Append(ParserWord* word, uint32_t codepoint)
{
    if (codepoint <= 0x7F) {
        word->buffer[word->length++] = (char)codepoint;
    } 
    else if (codepoint <= 0x7FF) {
        word->buffer[word->length++] = 0xC0 | ((codepoint >> 6) & 0x1F);
        word->buffer[word->length++] = 0x80 | (codepoint & 0x3F);
    } 
    else if (codepoint <= 0xFFFF) {
        word->buffer[word->length++] = 0xE0 | ((codepoint >> 12) & 0x0F);
        word->buffer[word->length++] = 0x80 | ((codepoint >> 6) & 0x3F);
        word->buffer[word->length++] = 0x80 | (codepoint & 0x3F);
    } 
    else if (codepoint <= 0x10FFFF) {
        word->buffer[word->length++] = 0xF0 | ((codepoint >> 18) & 0x07);
        word->buffer[word->length++] = 0x80 | ((codepoint >> 12) & 0x3F);
        word->buffer[word->length++] = 0x80 | ((codepoint >> 6) & 0x3F);
        word->buffer[word->length++] = 0x80 | (codepoint & 0x3F);
    } 
    else { // invalid codepoint
        return;
    }
    word->buffer[word->length] = '\0';
}

inline void ParserWord_Clear(ParserWord* word)
{
    word->length = 0;
}