#pragma once
#include <datastructures/string.h>
#include <datastructures/utf8_parser_word.h>


static void NU_Style_Tokenise(String src, struct Vector* tokens, struct Vector* textRefs)
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
                struct Style_Text_Ref ref;
                ref.NU_Token_index = tokens->size;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_PROPERTY_VALUE;
                Vector_Push(tokens, &token);
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
                struct Style_Text_Ref ref;
                ref.NU_Token_index = tokens->size;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_PROPERTY_VALUE;
                Vector_Push(tokens, &token);
                ParserWordClear(&word);
            }
            ctx=3; continue; // ^
        }

        // Enter selectorspace
        if (c == '{')
        {
            // Tag selector word completed
            if (ctx == 0 && word.length > 0) {
                enum NU_Style_Token token = NU_Word_To_Tag_Selector_Token(word.buffer, word.length);
                Vector_Push(tokens, &token);
                ParserWordClear(&word);
            }

            // Class selector word completed
            else if (ctx == 4 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = tokens->size;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_CLASS_SELECTOR;
                Vector_Push(tokens, &token);
                ParserWordClear(&word);
            }

            // Id selector word completed
            else if (ctx == 5 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = tokens->size;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_ID_SELECTOR;
                Vector_Push(tokens, &token);
                ParserWordClear(&word);
            }

            // Font name word completed
            else if (ctx == 7 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = tokens->size;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_FONT_NAME;
                Vector_Push(tokens, &token);
                ParserWordClear(&word);
            }

            // Add open brace token
            enum NU_Style_Token token = STYLE_SELECTOR_OPEN_BRACE;
            Vector_Push(tokens, &token);
            ParserWordClear(&word); ctx=2; continue; // ^
        }

        // Exiting selectorspace
        if (c == '}')
        {   
            // If word is present -> word completed (also an error)
            if (word.length > 0) {
                enum NU_Style_Token token = NU_Word_To_Style_Token(word.buffer, word.length);
                Vector_Push(tokens, &token);
                ParserWordClear(&word);
            }

            enum NU_Style_Token token = STYLE_SELECTOR_CLOSE_BRACE;
            Vector_Push(tokens, &token);
            ctx=0; continue; // ^
        }

        // Encountered separation character -> word is completed
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' || c == ':')
        {
            // Tag selector word completed
            if (ctx == 0 && word.length > 0) {
                enum NU_Style_Token token = NU_Word_To_Any_Selector_Token(word.buffer, word.length);
                Vector_Push(tokens, &token);
                ParserWordClear(&word);

                if (token == STYLE_FONT_CREATION_SELECTOR) { // Special selector context
                    ctx = 7;
                }
            }

            // Class selector word completed
            else if (ctx == 4 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = tokens->size;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_CLASS_SELECTOR;
                Vector_Push(tokens, &token);
                ctx=0;
            }

            // Id selector word completed
            else if (ctx == 5 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = tokens->size;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_ID_SELECTOR;
                Vector_Push(tokens, &token);
                ctx=0;
            }

            // Property identifier word completed
            else if (ctx == 2 && word.length > 0) {
                enum NU_Style_Token token = NU_Word_To_Style_Property_Token(word.buffer, word.length);
                Vector_Push(tokens, &token);
            }

            // Property value word completed
            else if (ctx == 3 && word.length > 0) {
                // Add text reference
                struct Style_Text_Ref ref;
                ref.src_index = i - word.length - 1;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_PROPERTY_VALUE;
                Vector_Push(tokens, &token);
                ctx=2;
            }

            // Pseudo class word completed
            else if (ctx == 6 && word.length > 0) {
                enum NU_Style_Token token = NU_Word_To_Pseudo_Token(word.buffer, word.length);
                Vector_Push(tokens, &token);
                ctx=0;
            }

            // Special font name word completed
            else if (ctx == 7 && word.length > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.src_index = i - word.length;
                ref.char_count = word.length;
                Vector_Push(textRefs, &ref);
                enum NU_Style_Token token = STYLE_FONT_NAME;
                Vector_Push(tokens, &token);
            }

            if (c == ',') {
                enum NU_Style_Token token = STYLE_SELECTOR_COMMA;
                Vector_Push(tokens, &token);
            }

            if (c == ':') {
                // Style property assignment token
                if (ctx == 2) {
                    enum NU_Style_Token token = STYLE_PROPERTY_ASSIGNMENT;
                    Vector_Push(tokens, &token);
                    ctx=3;
                }
                // Pseudo class assignment token
                else if (ctx == 0 || ctx == 4 || ctx == 5 ) {
                    enum NU_Style_Token token = STYLE_PSEUDO_COLON;
                    Vector_Push(tokens, &token);
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