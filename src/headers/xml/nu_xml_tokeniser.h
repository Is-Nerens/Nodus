#pragma once
#include <stdint.h>
#include "nu_xml_tokens.h"
#include "nu_xml_grammar_assertions.h"

void NU_Tokenise(char* src, uint32_t srcLen, Vector* tokenVectorOut, Vector* textRefsOut)
{
    char word[256];
    uint8_t  wordLen     = 0;
    uint32_t textLen     = 0;
    uint32_t trailingWS  = 0;
    uint8_t  ctx         = 0; // 0: globalspace, 1: commentspace, 2: tagspace, 3: property valuespace
    uint8_t  seenTagName = 0;
    
    
    uint32_t i = 0;
    while (i < srcLen)
    {
        char c = src[i];

        // comment begins
        if (ctx == 0 && i < srcLen-3 && c == '<' && src[i+1] == '!' && src[i+2] == '-' && src[i+3] == '-') {
            ctx=1; i+=4; continue; // ^
        }

        // comment ends
        if (ctx == 1 && i < srcLen-2 && c == '-' && src[i+1] == '-' && src[i+2] == '>') {
            ctx=0; i+=3; continue; // ^
        }

        // in comment
        if (ctx == 1) {
            i+=1; continue; // ^
        }

        // in globalspace
        if (ctx == 0) 
        {
            // open end tag </
            if (i < srcLen-1 && c == '<' && src[i+1] == '/') {
                // add text content token and text reference
                if (textLen > 0) {
                    enum NU_XML_TOKEN t = TEXT_CONTENT;
                    Vector_Push(tokenVectorOut, &t);
                    Text_Ref ref = { tokenVectorOut->size, i - textLen, textLen };
                    Vector_Push(textRefsOut, &ref);
                    textLen=0;
                }

                enum NU_XML_TOKEN t = OPEN_END_TAG;
                Vector_Push(tokenVectorOut, &t);
                wordLen=0; ctx=2; i+=2; seenTagName=0; continue; // ^
            }

            // tag opens
            if (c == '<') {
                // add text content token and text reference
                if (textLen > 0) {
                    enum NU_XML_TOKEN t = TEXT_CONTENT;
                    Vector_Push(tokenVectorOut, &t);
                    Text_Ref ref = { tokenVectorOut->size, i - textLen, textLen };
                    Vector_Push(textRefsOut, &ref);
                    textLen=0;
                }

                enum NU_XML_TOKEN t = OPEN_TAG;
                Vector_Push(tokenVectorOut, &t);
                wordLen=0; ctx=2; i+=1; seenTagName=0; continue; // ^
            }

            // text starts
            if (textLen == 0) {
                if (c != ' ' && c != '\t' && c != '\n') {
                    textLen = 1;
                    trailingWS = 0;
                }
            }
            // text continues
            else {
                if (c == ' ' || c == '\t' || c == '\n') {
                    trailingWS += 1;  // temporarily count as trailing
                } else {
                    // a meaningful char after some trailing spaces
                    textLen += 1 + trailingWS;  // include the spaces
                    trailingWS = 0;
                }
            }
            i+=1; continue; // ^
        }

        // in tagspace
        if (ctx == 2)
        {
            // self closing tag end
            if (i < srcLen-1 && c == '/' && src[i+1] == '>') 
            {
                // word ends -> convert to property token
                if (wordLen > 0) {
                    enum NU_XML_TOKEN t = NU_Word_To_Token(word, wordLen);
                    Vector_Push(tokenVectorOut, &t);
                    wordLen=0;
                }

                enum NU_XML_TOKEN t = CLOSE_END_TAG;
                Vector_Push(tokenVectorOut, &t);
                ctx=0; i+=2; continue; // ^
            }

            // tag ends
            if (c == '>') 
            {
                // word ends -> convert to property token
                if (wordLen > 0) {
                    enum NU_XML_TOKEN t = NU_Word_To_Token(word, wordLen);
                    Vector_Push(tokenVectorOut, &t);
                    wordLen=0;
                }

                enum NU_XML_TOKEN t = CLOSE_TAG;
                Vector_Push(tokenVectorOut, &t);
                ctx=0; i+=1; continue; // ^
            }
            
            // property assignment
            if (c == '=') {
                // word ends -> convert to tag token
                if (wordLen > 0) {
                    enum NU_XML_TOKEN t = NU_Word_To_Property_Token(word, wordLen);
                    Vector_Push(tokenVectorOut, &t);
                    wordLen=0;
                }

                enum NU_XML_TOKEN t = PROPERTY_ASSIGNMENT;
                Vector_Push(tokenVectorOut, &t);
                i+=1; continue; // ^
            }

            // property value begins
            if (c == '"') {
                if (wordLen > 0) { // word ends (there shouldn't be a word here, so this will trigger an error in the parser)
                    enum NU_XML_TOKEN t = NU_Word_To_Token(word, wordLen);
                    Vector_Push(tokenVectorOut, &t);
                    wordLen=0;
                }
                textLen=0; ctx=3; i+=1; continue; // ^
            }

            if (c == '\t' || c == '\n' || c == ' ') {
                // word ends
                if (wordLen > 0) {
                    enum NU_XML_TOKEN t;
                    if (seenTagName == 0) {
                        seenTagName=1;
                        t = NU_Word_To_Tag_Token(word, wordLen);
                    } 
                    else {
                        t = NU_Word_To_Property_Token(word, wordLen);
                    }
                    Vector_Push(tokenVectorOut, &t);
                    wordLen=0;
                }
            }
            else {
                // word accumulates
                word[wordLen]=c; wordLen+=1;
            }
            i+=1; continue; // ^
        }

        // in property value space
        if (ctx == 3) {

            // property value ends
            if (c == '"') {
                if (textLen > 0) {
                    struct Text_Ref ref = { tokenVectorOut->size, i-textLen, textLen };
                    Vector_Push(textRefsOut, &ref);
                    textLen=0;
                }
                enum NU_XML_TOKEN t = PROPERTY_VALUE;
                Vector_Push(tokenVectorOut, &t);
                ctx=2; i+=1; continue; // ^
            }

            textLen+=1; i+=1; continue; // ^
        }
    }
}
