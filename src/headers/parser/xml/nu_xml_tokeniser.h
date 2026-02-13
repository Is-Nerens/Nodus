#pragma once
#include <stdint.h>
#include <datastructures/string.h>
#include <datastructures/utf8_parser_word.h>
#include "nu_xml_tokens.h"
#include "nu_xml_grammar_assertions.h"

void NU_Tokenise(String src, TokenArray* tokenVectorOut, Vector* textRefsOut)
{
    ParserWord word;
    ParserWordInit(&word);
    uint32_t textLen     = 0;
    uint32_t trailingWS  = 0;
    uint8_t  ctx         = 0; // 0: globalspace, 1: commentspace, 2: tagspace, 3: property valuespace
    uint8_t  seenTagName = 0;
    
    // iterate over src file
    uint32_t srcLen = StringLen(src);
    int i = 0;
    while (i < srcLen)
    {   
        uint32_t c = NextUTF8Codepoint(src, &i);

        // comment begins
        int peekI = i;
        if (ctx == 0 && i < srcLen-3 
            && c == '<' 
            && NextUTF8Codepoint(src, &peekI) == '!' 
            && NextUTF8Codepoint(src, &peekI) == '-' 
            && NextUTF8Codepoint(src, &peekI) == '-') {
            i=peekI; ctx=1; continue; // ^
        }

        // comment ends
        peekI = i;
        if (ctx == 1 && i < srcLen-2 
            && c == '-' 
            && NextUTF8Codepoint(src, &peekI) == '-' 
            && NextUTF8Codepoint(src, &peekI) == '>') {
            i=peekI; ctx=0; continue; // ^
        }

        // in comment
        if (ctx == 1) {
            continue; // ^
        }

        // in globalspace
        if (ctx == 0) 
        {
            // open end tag </
            peekI = i;
            if (i < srcLen-1 && c == '<' && NextUTF8Codepoint(src, &peekI) == '/') {
    
                // add text content token and text reference
                if (textLen > 0) {
                    TokenArray_Add(tokenVectorOut, TEXT_CONTENT);
                    Text_Ref ref = { tokenVectorOut->size, i - textLen - 1, textLen };
                    Vector_Push(textRefsOut, &ref);
                    textLen=0;
                }

                TokenArray_Add(tokenVectorOut, OPEN_END_TAG);
                ParserWordClear(&word); i=peekI; ctx=2; seenTagName=0; continue; // ^
            }

            // tag opens
            if (c == '<') {
                // add text content token and text reference
                if (textLen > 0) {
                    TokenArray_Add(tokenVectorOut, TEXT_CONTENT);
                    Text_Ref ref = { tokenVectorOut->size, i - textLen - 1, textLen };
                    Vector_Push(textRefsOut, &ref);
                    textLen=0;
                }

                TokenArray_Add(tokenVectorOut, OPEN_TAG);
                ParserWordClear(&word); ctx=2; seenTagName=0; continue; // ^
            }

            // text starts
            if (textLen == 0) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    textLen = 1;
                    trailingWS = 0;
                }
            }
            // text continues
            else {
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    trailingWS += 1;  // temporarily count as trailing
                } else {
                    // a meaningful char after some trailing spaces
                    textLen += 1 + trailingWS;  // include the spaces
                    trailingWS = 0;
                }
            }
            continue; // ^
        }

        // in tagspace
        if (ctx == 2)
        {
            // self closing tag end
            peekI = i;
            if (i < srcLen-1 && c == '/' && NextUTF8Codepoint(src, &peekI) == '>') 
            {
                // word ends -> convert to token
                if (word.length > 0) {
                    TokenArray_Add(tokenVectorOut, NU_Word_To_Token(word.buffer, word.length));
                    ParserWordClear(&word);
                }

                TokenArray_Add(tokenVectorOut, CLOSE_END_TAG);
                i=peekI; ctx=0; continue; // ^
            }

            // tag ends
            if (c == '>') 
            {
                // word ends -> convert to token
                if (word.length > 0) {
                    TokenArray_Add(tokenVectorOut, NU_Word_To_Token(word.buffer, word.length));
                    ParserWordClear(&word);
                }

                TokenArray_Add(tokenVectorOut, CLOSE_TAG);
                ctx=0; continue; // ^
            }
            
            // property assignment
            if (c == '=') {
                // word ends -> convert to token
                if (word.length > 0) {
                    TokenArray_Add(tokenVectorOut, NU_Word_To_Property_Token(word.buffer, word.length));
                    ParserWordClear(&word);
                }

                TokenArray_Add(tokenVectorOut, PROPERTY_ASSIGNMENT);
                continue; // ^
            }

            // property value begins
            if (c == '"') {
                if (word.length > 0) { // word ends (there shouldn't be a word here, so this will trigger an error in the parser)
                    TokenArray_Add(tokenVectorOut, NU_Word_To_Token(word.buffer, word.length));
                    ParserWordClear(&word);
                }
                ctx=3; continue; // ^
            }

            if (c == '\t' || c == '\n' || c == '\r' || c == ' ') {
                // word ends
                if (word.length > 0) {
                    enum NU_XML_TOKEN t;
                    if (seenTagName == 0) {
                        seenTagName=1;
                        t = NU_Word_To_Tag_Token(word.buffer, word.length);
                    } 
                    else {
                        t = NU_Word_To_Property_Token(word.buffer, word.length);
                    }
                    TokenArray_Add(tokenVectorOut, t);
                    ParserWordClear(&word);
                }
            }
            else {
                // word accumulates
                ParserWordAppend(&word, c);
            }
            continue; // ^
        }

        // in property value space
        if (ctx == 3) {

            // property value ends
            if (c == '"') {
                if (word.length > 0) {
                    struct Text_Ref ref = { tokenVectorOut->size, i - word.length - 1, word.length };
                    Vector_Push(textRefsOut, &ref);
                    ParserWordClear(&word);
                }
                TokenArray_Add(tokenVectorOut, PROPERTY_VALUE);
                ctx=2; continue; // ^
            }
            ParserWordAppend(&word, c); continue; // ^
        }
    }
}
