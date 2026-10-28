#pragma once
#include <utils/nu_utf8_parser_word.h>
#include "../nu_token_array.h"
#include "nu_stylesheet_tokens.h"

static void Stylesheet_Tokenise(String src, TokenArray* tokens, Array* textRefs)
{
    ParserWord word;
    ParserWord_Init(&word);

    // Context
    u8 ctx = CSS_TOKENISE_CTX_GLOBAL;

    // Iterate over src file
    u32 srcLen = StringLen(src);
    int i = 0;
    while (i < srcLen)
    {
        u32 c = StringNextUTF8Codepoint(src, &i);

        // Globalspace comment begins
        int peekI = i;
        if (ctx == CSS_TOKENISE_CTX_GLOBAL && i < srcLen - 1 && c == '/' && StringNextUTF8Codepoint(src, &peekI) == '*') {
            i=peekI; ctx = CSS_TOKENISE_CTX_GLOBAL_COMMENT; continue; // ^
        }

        // Globalspace comment ends
        if (ctx == CSS_TOKENISE_CTX_GLOBAL_COMMENT && i < srcLen - 1 && c == '*' && StringNextUTF8Codepoint(src, &peekI) == '/') {
            i=peekI; ctx = CSS_TOKENISE_CTX_GLOBAL; continue; // ^
        }

        // Selectorspace comment begins
        if (ctx == CSS_TOKENISE_CTX_SELECTOR && i < srcLen - 1 && c == '/' && StringNextUTF8Codepoint(src, &peekI) == '*') {
            i=peekI; ctx = CSS_TOKENISE_CTX_SELECTOR_COMMENT; continue; // ^
        }

        // Selectorspace comment ends
        if (ctx == CSS_TOKENISE_CTX_SELECTOR_COMMENT && i < srcLen - 1 && c == '*' && StringNextUTF8Codepoint(src, &peekI) == '/') {
            i=peekI; ctx = CSS_TOKENISE_CTX_SELECTOR; continue; // ^
        }

        // In comment -> skip rest
        if (ctx == CSS_TOKENISE_CTX_GLOBAL_COMMENT || ctx == CSS_TOKENISE_CTX_SELECTOR_COMMENT) {
            continue; // ^
        }

        // Variable name definition space
        if (ctx == CSS_TOKENISE_CTX_SELECTOR && i < srcLen - 1 && c == '-' && StringNextUTF8Codepoint(src, &peekI) == '-') {
            ParserWord_Append(&word, '-'); ParserWord_Append(&word, '-');
            i=peekI; ctx = CSS_TOKENISE_CTX_VARIABLE_NAME_DEFINE; continue; // ^
        }

        // Enter class selector name space
        if (ctx == CSS_TOKENISE_CTX_GLOBAL && c == '.') {
            ParserWord_Clear(&word); ctx = CSS_TOKENISE_CTX_CLASS_SELECTOR_NAME; continue; // ^
        }

        // Enter id selector name space
        if (ctx == CSS_TOKENISE_CTX_GLOBAL && c == '#') {
            ParserWord_Clear(&word); ctx = CSS_TOKENISE_CTX_ID_SELECTOR_NAME; continue; // ^
        }

        // Property value word completed
        if (ctx == CSS_TOKENISE_CTX_PROPERTY_VALUE && c == ';') {

            // Add property text reference
            if (word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);

                // Variable property value
                if (word.length > 2 && word.buffer[0] == '-' && word.buffer[1] == '-') {
                    TokenArray_Add(tokens, STYLE_VARIABLE_PROPERTY_VALUE);
                }
                // Hard-coded property value
                else {
                    TokenArray_Add(tokens, STYLE_PROPERTY_VALUE);
                }
                ParserWord_Clear(&word);
            }
            ctx = CSS_TOKENISE_CTX_SELECTOR; continue; // ^
        }

        // Property value quotes string started
        if (ctx == CSS_TOKENISE_CTX_PROPERTY_VALUE && c == '"') {
            ParserWord_Clear(&word); ctx = CSS_TOKENISE_CTX_PROPERTY_STRING_VALUE; continue; // ^
        }
        if (ctx == CSS_TOKENISE_CTX_PROPERTY_STRING_VALUE && c =='\\') {
            ctx = CSS_TOKENISE_CTX_PROPERTY_VALUE_STRING_ESCAPE_CHAR; continue; // ^
        }
        if (ctx == CSS_TOKENISE_CTX_PROPERTY_VALUE_STRING_ESCAPE_CHAR) {
            ctx = CSS_TOKENISE_CTX_PROPERTY_STRING_VALUE; continue; // ^
        }
        // Property value quotes string completed
        if (ctx == CSS_TOKENISE_CTX_PROPERTY_STRING_VALUE && c == '"') {

            // Add property text reference
            if (word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);

                // Variable property value
                if (word.length > 2 && word.buffer[0] == '-' && word.buffer[1] == '-') {
                    TokenArray_Add(tokens, STYLE_VARIABLE_PROPERTY_VALUE);
                }
                // Hard-coded property value
                else {
                    TokenArray_Add(tokens, STYLE_PROPERTY_VALUE);
                }
                ParserWord_Clear(&word);
            }
            ctx = CSS_TOKENISE_CTX_PROPERTY_VALUE; continue; // ^
        }

        // Enter selectorspace
        if (c == '{')
        {
            // Tag selector word completed
            if (ctx == CSS_TOKENISE_CTX_GLOBAL && word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Tag_Selector_Token(word.buffer, word.length));
            }

            // Class selector word completed -> add text reference and class selector token
            else if (ctx == CSS_TOKENISE_CTX_CLASS_SELECTOR_NAME && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_CLASS_SELECTOR);
            }

            // Id selector word completed -> add text reference and id selector token
            else if (ctx == CSS_TOKENISE_CTX_ID_SELECTOR_NAME && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_ID_SELECTOR);
            }

            // Font name word completed > add text reference and font name token
            else if (ctx == CSS_TOKENISE_CTX_FONT_CREATION_NAME && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_FONT_NAME);
            }

            // Add open brace token
            TokenArray_Add(tokens, STYLE_SELECTOR_OPEN_BRACE);

            // Clear word and enter selector ctx
            ParserWord_Clear(&word); ctx = CSS_TOKENISE_CTX_SELECTOR; continue; // ^
        }

        // Exiting selectorspace
        if (c == '}')
        {
            // If word is present -> word completed (also an error)
            if (word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Style_Token(word.buffer, word.length));
                ParserWord_Clear(&word);
            }

            // Add close brace token and enter global ctx
            TokenArray_Add(tokens, STYLE_SELECTOR_CLOSE_BRACE);
            ctx = CSS_TOKENISE_CTX_GLOBAL; continue; // ^
        }

        // Encountered separation character -> word is completed
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' || c == ':')
        {
            // Property identifier word completed
            if (ctx == CSS_TOKENISE_CTX_SELECTOR && word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Style_Property_Token(word.buffer, word.length));
            }

            // Property value word completed (most common first)
            else if (ctx == CSS_TOKENISE_CTX_PROPERTY_VALUE && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);

                // Determine if is variable property value or normal property value
                if (word.length > 2 && word.buffer[0] == '-' && word.buffer[1] == '-') {
                    TokenArray_Add(tokens, STYLE_VARIABLE_PROPERTY_VALUE);
                }
                else {
                    TokenArray_Add(tokens, STYLE_PROPERTY_VALUE);
                }
                ctx = CSS_TOKENISE_CTX_SELECTOR;
            }

            // Class selector word completed
            else if (ctx == CSS_TOKENISE_CTX_CLASS_SELECTOR_NAME && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_CLASS_SELECTOR);
                ctx = CSS_TOKENISE_CTX_GLOBAL;
            }

            // Id selector word completed
            else if (ctx == CSS_TOKENISE_CTX_ID_SELECTOR_NAME && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_ID_SELECTOR);
                ctx = CSS_TOKENISE_CTX_GLOBAL;
            }

            // Pseudo class word completed
            else if (ctx == CSS_TOKENISE_CTX_PSEUDO_NAME && word.length > 0) {
                TokenArray_Add(tokens, NU_Word_To_Pseudo_Token(word.buffer, word.length));
                ctx = CSS_TOKENISE_CTX_GLOBAL;
            }

            // Any selector word completed
            else if (ctx == CSS_TOKENISE_CTX_GLOBAL && word.length > 0) {
                StylesheetToken token = NU_Word_To_Any_Selector_Token(word.buffer, word.length);
                TokenArray_Add(tokens, token);
                ParserWord_Clear(&word);

                if (token == STYLE_FONT_CREATION_SELECTOR) { // Special selector context
                    ctx = CSS_TOKENISE_CTX_FONT_CREATION_NAME;
                }
                else if (token == STYLE_SCREEN_SELECTOR) {
                    ctx = CSS_TOKENISE_CTX_SCREEN_SELECTOR_NAME;
                }
            }

            // Variable name definition word completed
            else if (ctx == CSS_TOKENISE_CTX_VARIABLE_NAME_DEFINE && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_VARIABLE_NAME);
                ctx = CSS_TOKENISE_CTX_SELECTOR;
            }

            // Special font name word completed
            else if (ctx == CSS_TOKENISE_CTX_FONT_CREATION_NAME && word.length > 0) {
                StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                Array_Push(textRefs, &ref);
                TokenArray_Add(tokens, STYLE_FONT_NAME);
            }

            // Screen query name space
            else if (ctx == CSS_TOKENISE_CTX_SCREEN_SELECTOR_NAME && word.length > 0) {

                // Check for comparators
                if (word.length == 1 && word.buffer[0] == '>') {
                    TokenArray_Add(tokens, STYLE_GREATER);
                }
                else if (word.length == 1 && word.buffer[0] == '<') {
                    TokenArray_Add(tokens, STYLE_LESS);
                }
                if (word.length == 2 && word.buffer[0] == '>' && word.buffer[1] == '=') {
                    TokenArray_Add(tokens, STYLE_GREATER_EQUAL);
                }
                else if (word.length == 2 && word.buffer[0] == '<'  && word.buffer[1] == '=') {
                    TokenArray_Add(tokens, STYLE_LESS_EQUAL);
                }
                // Tag as screen width value
                else {
                    StyleTextRef ref = { tokens->size, i - word.length - 1, word.length };
                    StringCstr(src)[ref.srcIndex + ref.len] = '\0';
                    Array_Push(textRefs, &ref);
                    TokenArray_Add(tokens, STYLE_SCREEN_QUERY_WIDTH);
                }
            }

            // Selector comma
            if (c == ',') {
                TokenArray_Add(tokens, STYLE_SELECTOR_COMMA);
            }

            // Colon
            if (c == ':') {
                // Style property assignment token
                if (ctx == CSS_TOKENISE_CTX_SELECTOR) {
                    TokenArray_Add(tokens, STYLE_PROPERTY_ASSIGNMENT);
                    ctx = CSS_TOKENISE_CTX_PROPERTY_VALUE;
                }
                // Pseudo class assignment token
                else if (ctx == CSS_TOKENISE_CTX_GLOBAL || ctx == CSS_TOKENISE_CTX_CLASS_SELECTOR_NAME || ctx == CSS_TOKENISE_CTX_ID_SELECTOR_NAME ) {
                    TokenArray_Add(tokens, STYLE_PSEUDO_COLON);
                    ctx = CSS_TOKENISE_CTX_PSEUDO_NAME;
                }
            }

            ParserWord_Clear(&word);
            continue; // ^
        }

        // Add char to word
        ParserWord_Append(&word, c);
    }
}
