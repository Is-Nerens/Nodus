#pragma once
#include <datastructures/string.h>
#include <datastructures/utf8_parser_word.h>
#include "../nu_token_array.h"


static void NU_Style_Tokenise(String src, TokenArray* tokens, struct Vector* textRefs)
{
    ParserWord word;
    ParserWordInit(&word);

    // Context
    uint8_t ctx = 0; 
    // 0 == globalspace, 1 == global commentspace, 2 == selectorspace, 10 == selector commentspace
    // 3 == property value space, 8 == property value string space 9 == property value string space escape char (\)
    // 4 == class selector namespace, 5 == id selector name space 
    // 6 == pseudo namespace, 
    // 7 == font creation namespace

    // Iterate over src file
    uint32_t srcLen = StringLen(src);
    int i = 0;
    while (i < srcLen)
    {
        uint32_t c = NextUTF8Codepoint(src, &i);
        

        // Globalspace comment begins
        int peekI = i;
        if (ctx == 0 && i < srcLen - 1 && c == '/' && NextUTF8Codepoint(src, &peekI) == '*') {
            i=peekI; ctx=1; continue; // ^
        }

        // Globalspace comment ends 
        peekI = i;
        if (ctx == 1 && i < srcLen - 1 && c == '*' && NextUTF8Codepoint(src, &peekI) == '/') {
            i=peekI; ctx=0; continue; // ^
        }

        // Selectorspace comment begins
        peekI = i;
        if (ctx == 2 && i < srcLen - 1 && c == '/' && NextUTF8Codepoint(src, &peekI) == '*') {
            i=peekI; ctx=10; continue; // ^
        }

        // Selectorspace comment ends 
        peekI = i;
        if (ctx == 10 && i < srcLen - 1 && c == '*' && NextUTF8Codepoint(src, &peekI) == '/') {
            i=peekI; ctx=2; continue; // ^
        }

        // In comment -> skip rest
        if (ctx == 1 || ctx == 10) {
            continue; // ^
        }

        // Enter class selector name space
        if (ctx == 0 && c == '.') {
            ParserWordClear(&word); ctx=4; continue; // ^
        }

        // Enter id selector name space
        if (ctx == 0 && c == '#') {
            ParserWordClear(&word); ctx=5; continue; // ^
        }

        // Property value word completed
        if (ctx == 3 && c == ';') {   

            // Add property text reference
            if (word.length > 0) {
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_PROPERTY_VALUE);
                ParserWordClear(&word);
            }
            ctx=2; continue; // ^
        }

        // Property value quotes string started
        if (ctx == 3 && c == '"') {
            ParserWordClear(&word); ctx=8; continue; // ^
        }
        if (ctx == 8 && c =='\\') {
            ctx=9; continue; // ^
        }
        if (ctx == 9) {
            ctx=8; continue; // ^
        }
        // Property value quotes string completed
        if (ctx == 8 && c == '"') {
            // Add property text reference
            if (word.length > 0) {
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_PROPERTY_VALUE);
                ParserWordClear(&word);
            }
            ctx=3; continue; // ^
        }

        // Enter selectorspace
        if (c == '{')
        {
            // Tag selector word completed
            if (ctx == 0 && word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Tag_Selector_Token(word.buffer, word.length));
                ParserWordClear(&word);
            }

            // Class selector word completed
            else if (ctx == 4 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_CLASS_SELECTOR);
                ParserWordClear(&word);
            }

            // Id selector word completed
            else if (ctx == 5 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_ID_SELECTOR);
                ParserWordClear(&word);
            }

            // Font name word completed
            else if (ctx == 7 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_FONT_NAME);
                ParserWordClear(&word);
            }

            // Add open brace token
            TokenArray_Add(tokens, STYLE_SELECTOR_OPEN_BRACE);
            ParserWordClear(&word); ctx=2; continue; // ^
        }

        // Exiting selectorspace
        if (c == '}')
        {   
            // If word is present -> word completed (also an error)
            if (word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Style_Token(word.buffer, word.length));
                ParserWordClear(&word);
            }

            TokenArray_Add(tokens, STYLE_SELECTOR_CLOSE_BRACE);
            ctx=0; continue; // ^
        }

        // Encountered separation character -> word is completed
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' || c == ':')
        {
            // Tag selector word completed
            if (ctx == 0 && word.length > 0) {
                enum NU_Style_Token token = NU_Word_To_Any_Selector_Token(word.buffer, word.length);
                TokenArray_Add(tokens, token);
                ParserWordClear(&word);

                if (token == STYLE_FONT_CREATION_SELECTOR) { // Special selector context
                    ctx = 7;
                }
            }

            // Class selector word completed
            else if (ctx == 4 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_CLASS_SELECTOR);
                ctx=0;
            }

            // Id selector word completed
            else if (ctx == 5 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_ID_SELECTOR);
                ctx=0;
            }

            // Property identifier word completed
            else if (ctx == 2 && word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Style_Property_Token(word.buffer, word.length));
            }

            // Property value word completed
            else if (ctx == 3 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_PROPERTY_VALUE);
                ctx=2;
            }

            // Pseudo class word completed
            else if (ctx == 6 && word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Pseudo_Token(word.buffer, word.length));
                ctx=0;
            }

            // Special font name word completed
            else if (ctx == 7 && word.length > 0) {

                // Add text reference
                struct Style_Text_Ref ref = { tokens->size, i - word.length - 1, word.length };
                Vector_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_FONT_NAME);
            }

            if (c == ',') {
                TokenArray_Add(tokens, STYLE_SELECTOR_COMMA);
            }

            if (c == ':') {
                // Style property assignment token
                if (ctx == 2) {
                    TokenArray_Add(tokens, STYLE_PROPERTY_ASSIGNMENT);
                    ctx=3;
                }
                // Pseudo class assignment token
                else if (ctx == 0 || ctx == 4 || ctx == 5 ) {
                    TokenArray_Add(tokens, STYLE_PSEUDO_COLON);
                    ctx=6;
                }
            }

            ParserWordClear(&word);
            continue; // ^
        }

        // Add char to word
        ParserWordAppend(&word, c);
    }
}