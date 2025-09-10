#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "nu_convert.h"
#include "nu_stylesheet.h"

#define STYLE_PROPERTY_COUNT 30
#define STYLE_KEYWORD_COUNT 36

static const char* style_keywords[] = {
    "dir",
    "grow",
    "overflowV",
    "overflowH",
    "gap",
    "width",
    "minWidth",
    "maxWidth",
    "height", 
    "minHeight",
    "maxHeight",
    "alignH",
    "alignV",
    "background",
    "borderColour",
    "border", "borderTop", "borderBottom", "borderLeft", "borderRight",
    "borderRadius", "borderRadiusTopLeft", "borderRadiusTopRight", "borderRadiusBottomLeft", "borderRadiusBottomRight",
    "pad", "padTop", "padBottom", "padLeft", "padRight",
    "window", "rect", "button", "grid", "text", "image"
};
static const uint8_t style_keyword_lengths[] = { 
    3, 4, 9, 9, 3, 5, 8, 8, 6, 9, 9, 6, 6, 10, 12,
    6, 9, 12, 10, 11,      // border width
    12, 19, 20, 22, 23,    // border radius
    3, 6, 9, 7, 11,        // padding
    6, 4, 6, 4, 4, 5       // selectors
};
enum NU_Style_Token 
{
    STYLE_LAYOUT_DIRECTION_PROPERTY,
    STYLE_GROW_PROPERTY,
    STYLE_OVERFLOW_V_PROPERTY,
    STYLE_OVERFLOW_H_PROPERTY,
    STYLE_GAP_PROPERTY,
    STYLE_WIDTH_PROPERTY,
    STYLE_MIN_WIDTH_PROPERTY,
    STYLE_MAX_WIDTH_PROPERTY,
    STYLE_HEIGHT_PROPERTY,
    STYLE_MIN_HEIGHT_PROPERTY,
    STYLE_MAX_HEIGHT_PROPERTY,
    STYLE_ALIGN_H_PROPERTY,
    STYLE_ALIGN_V_PROPERTY,
    STYLE_BACKGROUND_COLOUR_PROPERTY,
    STYLE_BORDER_COLOUR_PROPERTY,
    STYLE_BORDER_WIDTH_PROPERTY,
    STYLE_BORDER_TOP_WIDTH_PROPERTY,
    STYLE_BORDER_BOTTOM_WIDTH_PROPERTY,
    STYLE_BORDER_LEFT_WIDTH_PROPERTY,
    STYLE_BORDER_RIGHT_WIDTH_PROPERTY,
    STYLE_BORDER_RADIUS_PROPERTY,
    STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY,
    STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY,
    STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY,
    STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY,
    STYLE_PADDING_PROPERTY,
    STYLE_PADDING_TOP_PROPERTY,
    STYLE_PADDING_BOTTOM_PROPERTY,
    STYLE_PADDING_LEFT_PROPERTY,
    STYLE_PADDING_RIGHT_PROPERTY,
    STYLE_WINDOW_SELECTOR,
    STYLE_RECT_SELECTOR,
    STYLE_BUTTON_SELECTOR,
    STYLE_GRID_SELECTOR,
    STYLE_TEXT_SELECTOR,
    STYLE_IMAGE_SELECTOR,
    STYLE_ID_SELECTOR,
    STYLE_CLASS_SELECTOR,
    STYLE_SELECTOR_COMMA,
    STYLE_SELECTOR_OPEN_BRACE,
    STYLE_SELECTOR_CLOSE_BRACE,
    STYLE_PROPERTY_ASSIGNMENT,
    STYLE_PROPERTY_VALUE,
    STYLE_UNDEFINED
};

static uint32_t Property_Token_To_Flag(enum NU_Style_Token token)
{
    return (1u << token);
}

struct Style_Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count; 
};

static void printfToken(enum NU_Style_Token token)
{
    switch (token)
    {
        case STYLE_LAYOUT_DIRECTION_PROPERTY:      printf("LAYOUT_DIRECTION_PROPERTY\n"); break;
        case STYLE_GROW_PROPERTY:                  printf("GROW_PROPERTY\n"); break;
        case STYLE_OVERFLOW_V_PROPERTY:            printf("OVERFLOW_V_PROPERTY\n"); break;
        case STYLE_OVERFLOW_H_PROPERTY:            printf("OVERFLOW_H_PROPERTY\n"); break;
        case STYLE_GAP_PROPERTY:                   printf("GAP_PROPERTY\n"); break;
        case STYLE_WIDTH_PROPERTY:                 printf("WIDTH_PROPERTY\n"); break;
        case STYLE_MIN_WIDTH_PROPERTY:             printf("MIN_WIDTH_PROPERTY\n"); break;
        case STYLE_MAX_WIDTH_PROPERTY:             printf("MAX_WIDTH_PROPERTY\n"); break;
        case STYLE_HEIGHT_PROPERTY:                printf("HEIGHT_PROPERTY\n"); break;
        case STYLE_MIN_HEIGHT_PROPERTY:            printf("MIN_HEIGHT_PROPERTY\n"); break;
        case STYLE_MAX_HEIGHT_PROPERTY:            printf("MAX_HEIGHT_PROPERTY\n"); break;
        case STYLE_ALIGN_H_PROPERTY:               printf("ALIGN_H_PROPERTY\n"); break;
        case STYLE_ALIGN_V_PROPERTY:               printf("ALIGN_V_PROPERTY\n"); break;
        case STYLE_BACKGROUND_COLOUR_PROPERTY:     printf("BACKGROUND_COLOUR_PROPERTY\n"); break;
        case STYLE_BORDER_COLOUR_PROPERTY:         printf("BORDER_COLOUR_PROPERTY\n"); break;
        case STYLE_BORDER_WIDTH_PROPERTY:          printf("BORDER_WIDTH_PROPERTY\n"); break;
        case STYLE_BORDER_TOP_WIDTH_PROPERTY:      printf("BORDER_TOP_WIDTH_PROPERTY\n"); break;
        case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:   printf("BORDER_BOTTOM_WIDTH_PROPERTY\n"); break;
        case STYLE_BORDER_LEFT_WIDTH_PROPERTY:     printf("BORDER_LEFT_WIDTH_PROPERTY\n"); break;
        case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:    printf("BORDER_RIGHT_WIDTH_PROPERTY\n"); break;
        case STYLE_BORDER_RADIUS_PROPERTY:         printf("BORDER_RADIUS_PROPERTY\n"); break;
        case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:    printf("BORDER_TOP_LEFT_RADIUS_PROPERTY\n"); break;
        case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:   printf("BORDER_TOP_RIGHT_RADIUS_PROPERTY\n"); break;
        case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY: printf("BORDER_BOTTOM_LEFT_RADIUS_PROPERTY\n"); break;
        case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:printf("BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY\n"); break;
        case STYLE_PADDING_PROPERTY:               printf("PADDING_PROPERTY\n"); break;
        case STYLE_PADDING_TOP_PROPERTY:           printf("PADDING_TOP_PROPERTY\n"); break;
        case STYLE_PADDING_BOTTOM_PROPERTY:        printf("PADDING_BOTTOM_PROPERTY\n"); break;
        case STYLE_PADDING_LEFT_PROPERTY:          printf("PADDING_LEFT_PROPERTY\n"); break;
        case STYLE_PADDING_RIGHT_PROPERTY:         printf("PADDING_RIGHT_PROPERTY\n"); break;
        case STYLE_WINDOW_SELECTOR:                printf("WINDOW_SELECTOR\n"); break;
        case STYLE_RECT_SELECTOR:                  printf("RECT_SELECTOR\n"); break;
        case STYLE_BUTTON_SELECTOR:                printf("BUTTON_SELECTOR\n"); break;
        case STYLE_GRID_SELECTOR:                  printf("GRID_SELECTOR\n"); break;
        case STYLE_TEXT_SELECTOR:                  printf("TEXT_SELECTOR\n"); break;
        case STYLE_IMAGE_SELECTOR:                 printf("IMAGE_SELECTOR\n"); break;
        case STYLE_ID_SELECTOR:                    printf("ID_SELECTOR\n"); break;
        case STYLE_CLASS_SELECTOR:                 printf("CLASS_SELECTOR\n"); break;
        case STYLE_SELECTOR_COMMA:                 printf("SELECTOR_COMMA\n"); break;
        case STYLE_SELECTOR_OPEN_BRACE:            printf("SELECTOR_OPEN_BRACE\n"); break;
        case STYLE_SELECTOR_CLOSE_BRACE:           printf("SELECTOR_CLOSE_BRACE\n"); break;
        case STYLE_PROPERTY_ASSIGNMENT:            printf("PROPERTY_ASSIGNMENT\n"); break;
        case STYLE_PROPERTY_VALUE:                 printf("PROPERTY_VALUE\n"); break;
        case STYLE_UNDEFINED:                      printf("UNDEFINED\n"); break;
        default:                             printf("UNKNOWN TOKEN (%d)\n", token); break;
    }
}

static enum NU_Style_Token NU_Word_To_Style_Token(char* word, uint8_t word_char_count)
{
    for (uint8_t i=0; i<STYLE_KEYWORD_COUNT; i++) {
        size_t len = style_keyword_lengths[i];
        if (len == word_char_count && memcmp(word, style_keywords[i], len) == 0) {
            return i;
        } 
    }
    return STYLE_UNDEFINED;
}

static enum NU_Style_Token NU_Word_To_Property_Token(char* word, uint8_t word_char_count)
{
    for (int i=0; i<30; i++)
    {
        size_t len = style_keyword_lengths[i];
        if (len == word_char_count && memcmp(word, style_keywords[i], len) == 0) {
            return i;
        }
    }
    return STYLE_UNDEFINED;
}

static enum NU_Style_Token NU_Word_To_Tag_Selector_Token(char* word, uint8_t word_char_count)
{
    for (int i=30; i<36; i++)
    {
        size_t len = style_keyword_lengths[i];
        if (len == word_char_count && memcmp(word, style_keywords[i], len) == 0) {
            return i;
        }
    }
    return STYLE_UNDEFINED;
}

static void NU_Style_Tokenise(char* src_buffer, uint32_t src_length, struct Vector* NU_Token_vector, struct Vector* text_ref_vector)
{
    // Store current NU_Token word;
    uint8_t word_char_index = 0;
    char word[24];

    // Context
    uint8_t ctx = 0; // 0 == globalspace, 1 == commentspace, 2 == selectorspace, 3 == propertyspace, 4 == class selector namespace, 5 == id selector name space 

    // Iterate over src file
    uint32_t i = 0;
    while (i < src_length)
    {
        char c = src_buffer[i];

        // Comment begins
        if (ctx == 0 && i < src_length - 1 && c == '/' && src_buffer[i+1] == '*') 
        {
            ctx = 1;
            i += 2;
            continue;
        }

        // Comment ends 
        if (ctx == 1 && i < src_length - 1 && c == '*' && src_buffer[i+1] == '/')
        {
            ctx = 0;
            i += 2;
            continue;
        }

        // In comment -> skip rest
        if (ctx == 1)
        {
            i += 1;
            continue;
        }

        // Enter class selector name space
        if (ctx == 0 && c == '.')
        {
            word_char_index = 0;
            ctx = 4;
            i += 1;
            continue;
        }

        // Enter id selector name space
        if (ctx == 0 && c == '#')
        {
            word_char_index = 0;
            ctx = 5;
            i += 1;
            continue;
        }

        // Property assignment colon
        if (ctx == 2 && c == ':')
        {
            // Property identifier word completed
            if (word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Property_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            enum NU_Style_Token token = STYLE_PROPERTY_ASSIGNMENT;
            Vector_Push(NU_Token_vector, &token);

            ctx = 3;
            i += 1;
            continue;
        }

        // Property value word completed
        if (ctx == 3 && c == ';')
        {   
            // Property value word completed
            if (word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);

                enum NU_Style_Token token = STYLE_PROPERTY_VALUE;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            ctx = 2;
            i += 1;
            continue;
        }

        // Entering selectorspace
        if (c == '{')
        {
            // Tag selector word completed
            if (ctx == 0 && word_char_index > 0) {
                
                enum NU_Style_Token token = NU_Word_To_Tag_Selector_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Class selector word completed
            if (ctx == 4 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);

                enum NU_Style_Token token = STYLE_CLASS_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Id selector word completed
            if (ctx == 5 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);

                enum NU_Style_Token token = STYLE_ID_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            enum NU_Style_Token token = STYLE_SELECTOR_OPEN_BRACE;
            Vector_Push(NU_Token_vector, &token);
            ctx = 2;
            i += 1;
            continue;
        }

        // Exiting selectorspace
        if (ctx == 2 && c == '}')
        {   
            // If word is present -> word completed
            if (word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Style_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            enum NU_Style_Token token = STYLE_SELECTOR_CLOSE_BRACE;
            Vector_Push(NU_Token_vector, &token);
            ctx = 0;
            i += 1;
            continue;
        }

        // Encountered separation character -> word is completed
        if (c == ' ' || c == '\t' || c == '\n' || c == ',')
        {
            // Tag selector word completed
            if (ctx == 0 && word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Tag_Selector_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Class selector word completed
            if (ctx == 4 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);

                enum NU_Style_Token token = STYLE_CLASS_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Id selector word completed
            if (ctx == 5 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);

                enum NU_Style_Token token = STYLE_ID_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Property identifier word completed
            if (ctx == 2 && word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Property_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Property value word completed
            if (ctx == 3 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);

                enum NU_Style_Token token = STYLE_PROPERTY_VALUE;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            if (c == ',') {
                enum NU_Style_Token token = STYLE_SELECTOR_COMMA;
                Vector_Push(NU_Token_vector, &token);
            }

            i += 1;
            continue;
        }

        // Add char to word (if ctx == 0 || 2 || 3 || 4 || 5)
        if (ctx != 1)
        {
            word[word_char_index] = c;
            word_char_index += 1;
        }

        i += 1;
    }
}

static int NU_Is_Property_Identifier_Token(enum NU_Style_Token token)
{
    if (token < 30) return 1;
    return 0;
}

static int NU_Is_Tag_Selector_Token(enum NU_Style_Token token)
{
    return token > 29 && token < 36;
}

static int AssertSelectionOpeningBraceGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A PROPERTY INDENTIFIER OR CLOSING BRACE
    if (i < tokens->size - 1)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        if (NU_Is_Property_Identifier_Token(next_token) || next_token == STYLE_SELECTOR_CLOSE_BRACE) return 1;
    }
    printf("%s", "[Generate Stylesheet] Error! Expected property identifier!");
    return 0;
}

static int AssertSelectionClosingBraceGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: MUST BE LAST TOKEN OR NEXT TOKEN MUST BE A SELECTOR
    if (i == tokens->size - 1) return 1;
    enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
    if (next_token == STYLE_CLASS_SELECTOR || next_token == STYLE_ID_SELECTOR || NU_Is_Tag_Selector_Token(next_token)) return 1;
    printf("%s", "[Generate Stylesheet] Error! Expected selector or end of file!");
    return 0;
}

static int AssertSelectorGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A COMMA OR SELECTION OPENING BRACE
    if (i < tokens->size - 1)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        return next_token == STYLE_SELECTOR_COMMA || STYLE_SELECTOR_OPEN_BRACE;
    }
    return 0;
}

static int AssertSelectorCommaGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A SELECTOR
    if (i < tokens->size - 1)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        if (next_token == STYLE_CLASS_SELECTOR || next_token == STYLE_ID_SELECTOR || NU_Is_Tag_Selector_Token(next_token)) return 1;
    }
    printf("%s", "[Generate Stylesheet] Error! Expected selector after selector comma!");
    return 0;
}

static int AssertPropertyIdentifierGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A PROPERTY ASSIGNMENT ':' 
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE A PROPERTY VALUE
    if (i < tokens->size - 3)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        enum NU_Style_Token following_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+2));
        enum NU_Style_Token third_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+3));
        return next_token == STYLE_PROPERTY_ASSIGNMENT && following_token == STYLE_PROPERTY_VALUE && (third_token == STYLE_SELECTOR_CLOSE_BRACE || NU_Is_Property_Identifier_Token(third_token));
    }
    return 0;
}





static int NU_Generate_Stylesheet(char* src_buffer, uint32_t src_length, struct Vector* tokens, struct NU_Stylesheet* ss, struct Vector* text_refs)
{
    uint32_t text_index = 0;
    struct Style_Text_Ref* text_ref;

    struct NU_Stylesheet_Item item;
    item.property_flags = 0;

    char* selector_classes[64];
    char* selector_ids[64];
    int selector_tags[64];
    int selector_count = 0;

    int i = 0;
    while(i < tokens->size)
    {
        const enum NU_Style_Token token = *((enum NU_Style_Token*) Vector_Get(tokens, i));

        if (token == STYLE_SELECTOR_OPEN_BRACE)
        {
            if (AssertSelectionOpeningBraceGrammar(tokens, i)) {
                item.property_flags = 0;
                i += 1;
                continue;
            } 
            else {
                return -1;
            }
        }

        if (token == STYLE_SELECTOR_CLOSE_BRACE)
        {
            if (AssertSelectionClosingBraceGrammar(tokens, i)) {
                for (int i=0; i<selector_count; i++) {
                    item.class = selector_classes[i];
                    item.id = selector_ids[i];
                    item.tag = selector_tags[i];
                    item.item_index = ss->items.size;
                    Vector_Push(&ss->items, &item);
                }
                selector_count = 0;
                i += 1;
                continue;
            } 
            else {
                return -1;
            }
        }

        if (NU_Is_Tag_Selector_Token(token))
        {
            if (AssertSelectorGrammar(tokens, i)) {
                i += 1;
                selector_classes[selector_count] = NULL;
                selector_ids[selector_count] = NULL;
                selector_tags[selector_count] = token - 30; // convert tag selector to tag
                selector_count++;
                continue;
            }
            else
            {
                return -1;
            }
        }

        if (token == STYLE_CLASS_SELECTOR)
        {
            if (AssertSelectorGrammar(tokens, i)) {
                i += 1;

                // Add class name to style sheet
                text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                char* classname = &src_buffer[text_ref->src_index];
                src_buffer[text_ref->src_index + text_ref->char_count] = '\0';

                selector_ids[selector_count] = NULL;
                selector_classes[selector_count] = NU_Stylesheet_Add_Classname(ss, classname);
                selector_tags[selector_count] = -1;
                selector_count++;

                text_index += 1;
                continue;
            }
            else {
                return -1;
            }
        }

        if (token == STYLE_ID_SELECTOR)
        {
            if (AssertSelectorGrammar(tokens, i)) {
                i += 1;

                // Add id to style sheet
                text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                char* id = &src_buffer[text_ref->src_index];
                src_buffer[text_ref->src_index + text_ref->char_count] = '\0';
     
                selector_ids[selector_count] = NU_Stylesheet_Add_Id(ss, id);
                selector_classes[selector_count] = NULL;
                selector_tags[selector_count] = -1;
                selector_count++;

                text_index += 1;
                continue;
            }
            else {
                return -1;
            }
        }

        if (token == STYLE_SELECTOR_COMMA)
        {
            if (AssertSelectorCommaGrammar(tokens, i)) {
                if (selector_count == 64) {
                    printf("%s", "[Generate Stylesheet] Error! Too many selectors in one list! max = 64");
                    return -1;
                }
                i += 1;
                continue;
            }
            else {
                return -1;
            }
        }

        if (NU_Is_Property_Identifier_Token(token))
        {
            if (AssertPropertyIdentifierGrammar(tokens, i)) {
                text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                char c = src_buffer[text_ref->src_index];
                char* text = &src_buffer[text_ref->src_index];
                char temp = src_buffer[text_ref->src_index + text_ref->char_count];
                src_buffer[text_ref->src_index + text_ref->char_count] = '\0';
                text_index += 1;

                switch (token)
                {
                    // Set layout direction
                    case STYLE_LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'h') 
                            item.layout_flags |= LAYOUT_HORIZONTAL;
                        else 
                            item.layout_flags |= LAYOUT_VERTICAL;
                        item.property_flags |= 1 << 0;
                        break;

                    // Set growth 
                    case STYLE_GROW_PROPERTY:
                        switch(c)
                        {
                            case 'v':
                                item.layout_flags |= GROW_VERTICAL;
                                break;
                            case 'h':
                                item.layout_flags |= GROW_HORIZONTAL;
                                break;
                            case 'b':
                                item.layout_flags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                                break;
                        }
                        item.property_flags |= 1 << 1;
                        break;
                    
                    // Set overflow behaviour
                    case STYLE_OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            item.layout_flags |= OVERFLOW_VERTICAL_SCROLL;
                            item.property_flags |= 1 << 2;
                        }
                        break;
                    
                    case STYLE_OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            item.layout_flags |= OVERFLOW_HORIZONTAL_SCROLL;
                            item.property_flags |= 1 << 3;
                        }
                        break;
                    
                    // Set gap
                    case STYLE_GAP_PROPERTY:
                        if (String_To_Float(&item.gap, text) == 0)
                            item.property_flags |= 1 << 4;
                        break;

                    // Set preferred width
                    case STYLE_WIDTH_PROPERTY:
                        if (String_To_Float(&item.preferred_width, text) == 0)
                            item.property_flags |= 1 << 5;
                        break;

                    // Set min width
                    case STYLE_MIN_WIDTH_PROPERTY:
                        if (String_To_Float(&item.min_width, text) == 0)
                            item.property_flags |= 1 << 6;
                        break;
                    
                    // Set max width
                    case STYLE_MAX_WIDTH_PROPERTY:
                        if (String_To_Float(&item.max_width, text) == 0)
                            item.property_flags |= 1 << 7;
                        break;

                    // Set preferred height
                    case STYLE_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.preferred_height, text) == 0) 
                            item.property_flags |= 1 << 8;
                        break;

                    // Set min height
                    case STYLE_MIN_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.min_height, text) == 0) 
                            item.property_flags |= 1 << 9;
                        break;

                    // Set max height
                    case STYLE_MAX_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.max_height, text) == 0) 
                            item.property_flags |= 1 << 10;
                        break;

                    // Set horizontal alignment
                    case STYLE_ALIGN_H_PROPERTY:
                        if (text_ref->char_count == 4 && memcmp(text, "left", 4) == 0) {
                            item.horizontal_alignment = 0;
                            item.property_flags |= 1 << 11;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.horizontal_alignment = 1;
                            item.property_flags |= 1 << 11;
                        }
                        if (text_ref->char_count == 5 && memcmp(text, "right", 5) == 0) {
                            item.horizontal_alignment = 2;
                            item.property_flags |= 1 << 11;
                        }
                        break;

                    // Set vertical alignment
                    case STYLE_ALIGN_V_PROPERTY:
                        if (text_ref->char_count == 3 && memcmp(text, "top", 3) == 0) {
                            item.vertical_alignment = 0;
                            item.property_flags |= 1 << 12;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.vertical_alignment = 1;
                            item.property_flags |= 1 << 12;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "bottom", 6) == 0) {
                            item.vertical_alignment = 2;
                            item.property_flags |= 1 << 12;
                        }
                        break;

                    // Set background colour
                    case STYLE_BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.background_r = rgb.r;
                            item.background_g = rgb.g;
                            item.background_b = rgb.b;
                            item.property_flags |= 1 << 13;
                        }
                        break;

                    // Set border colour
                    case STYLE_BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.border_r = rgb.r;
                            item.border_g = rgb.g;
                            item.border_b = rgb.b;
                            item.property_flags |= 1 << 14;
                        }
                        break;
                    
                    // Set border width
                    case STYLE_BORDER_WIDTH_PROPERTY:
                        uint8_t border_width;
                        if (String_To_uint8_t(&border_width, text)) {
                            item.border_top = border_width;
                            item.border_bottom = border_width;
                            item.border_left = border_width;
                            item.border_right = border_width;
                            item.property_flags |= 1 << 15;
                            item.property_flags |= 1 << 16;
                            item.property_flags |= 1 << 17;
                            item.property_flags |= 1 << 18;
                        }
                        break;
                    case STYLE_BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_top, text)) {
                            item.property_flags |= 1 << 15;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_bottom, text)) {
                            item.property_flags |= 1 << 16;
                        }
                        break;
                    case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_left, text)) {
                            item.property_flags |= 1 << 17;
                        }
                        break;
                    case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_right, text)) {
                            item.property_flags |= 1 << 18;
                        }
                        break;

                    // Set border radii
                    case STYLE_BORDER_RADIUS_PROPERTY:
                        uint8_t border_radius;
                        if (String_To_uint8_t(&border_radius, text)) {
                            item.border_radius_tl = border_radius;
                            item.border_radius_tr = border_radius;
                            item.border_radius_bl = border_radius;
                            item.border_radius_br = border_radius;
                            item.property_flags |= 1 << 19;
                            item.property_flags |= 1 << 20;
                            item.property_flags |= 1 << 21;
                            item.property_flags |= 1 << 22;
                        }
                        break;
                    case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_tl, text)) {
                            item.property_flags |= 1 << 19;
                        }
                        break;
                    case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_tr, text)) {
                            item.property_flags |= 1 << 20;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_bl, text)) {
                            item.property_flags |= 1 << 21;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_br, text)) {
                            item.property_flags |= 1 << 22;
                        }
                        break;

                    // Set padding
                    case STYLE_PADDING_PROPERTY:
                        uint8_t pad;
                        if (String_To_uint8_t(&pad, text)) {
                            item.pad_top = pad;
                            item.pad_bottom = pad;
                            item.pad_left = pad;
                            item.pad_right = pad;
                            item.property_flags |= 1 << 23;
                            item.property_flags |= 1 << 24;
                            item.property_flags |= 1 << 25;
                            item.property_flags |= 1 << 26;
                        }
                        break;
                    case STYLE_PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&item.pad_top, text)) {
                            item.property_flags |= 1 << 23;
                        }
                        break;
                    case STYLE_PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&item.pad_bottom, text)) {
                            item.property_flags |= 1 << 24;
                        }
                        break;
                    case STYLE_PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&item.pad_left, text)) {
                            item.property_flags |= 1 << 25;
                        }
                        break;
                    case STYLE_PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&item.pad_right, text)) {
                            item.property_flags |= 1 << 26;
                        }
                        break;

                    default:
                        break;
                }

                i += 3;
                continue;
            }
            else {
                return -1;
            }
        }

        i += 1;
    }

    return 0;
}

static void NU_Stylesheet_Find_Match(struct Node* node, struct NU_Stylesheet* ss, int* match_index_list)
{
    int tag_match_index   = -1;
    int class_match_index = -1;
    int id_match_index    = -1;

    for (int i = 0; i < ss->items.size; i++) { // Tag match
        struct NU_Stylesheet_Item* item = (struct NU_Stylesheet_Item*)Vector_Get(&ss->items, i);
        if (node->tag == item->tag) {
            tag_match_index = i;
            break; // stop at first match (assuming unique)
        }
    }

    if (node->class != NULL) { // Class match
        class_match_index = NU_Stylesheet_Get_Item_Index_From_Classname(ss, node->class);
    }

    if (node->id != NULL) { // Id match
        id_match_index = NU_Stylesheet_Get_Item_Index_From_Id(ss, node->id);
    }

    // Collect non -1 results
    int tmp[3];
    int count = 0;
    if (tag_match_index   != -1) tmp[count++] = tag_match_index;
    if (class_match_index != -1) tmp[count++] = class_match_index;
    if (id_match_index    != -1) tmp[count++] = id_match_index;

    if (count == 0) return; // nothing matched

    // Sort manually (since count ≤ 3)
    if (count == 2) {
        if (tmp[0] > tmp[1]) {
            int t = tmp[0]; tmp[0] = tmp[1]; tmp[1] = t;
        }
    } else if (count == 3) {
        if (tmp[0] > tmp[1]) { int t = tmp[0]; tmp[0] = tmp[1]; tmp[1] = t; }
        if (tmp[1] > tmp[2]) { int t = tmp[1]; tmp[1] = tmp[2]; tmp[2] = t; }
        if (tmp[0] > tmp[1]) { int t = tmp[0]; tmp[0] = tmp[1]; tmp[1] = t; }
    }
    for (int i = 0; i < count; i++) {
        match_index_list[i] = tmp[i];
    }
    for (int i = count; i < 3; i++) {
        match_index_list[i] = -1;
    }
}


static void NU_Apply_Stylesheet_To_Node(struct Node* node, struct NU_Stylesheet* ss)
{
    int match_index_list[3] = {-1, -1, -1};
    NU_Stylesheet_Find_Match(node, ss, &match_index_list[0]);

    int i = 0;
    while (match_index_list[i] != -1) {
        struct NU_Stylesheet_Item* item = (struct NU_Stylesheet_Item*)Vector_Get(&ss->items, (uint32_t)match_index_list[i]);
        i += 1;

        // --- Apply style ---
        if (item->property_flags & (1 << 0)) node->layout_flags = (node->layout_flags & ~(1 << 0)) | (item->layout_flags & (1 << 0)); // Layout direction
        if (item->property_flags & (1 << 1)) {
            node->layout_flags = (node->layout_flags & ~(1 << 1)) | (item->layout_flags & (1 << 1)); // Grow horizontal
            node->layout_flags = (node->layout_flags & ~(1 << 2)) | (item->layout_flags & (1 << 2)); // Grow vertical
        }
        if (item->property_flags & (1 << 2)) {
            node->layout_flags = (node->layout_flags & ~(1 << 3)) | (item->layout_flags & (1 << 3)); // Overflow vertical scroll (or not)
        }
        if (item->property_flags & (1 << 3)) {
            node->layout_flags = (node->layout_flags & ~(1 << 4)) | (item->layout_flags & (1 << 4)); // Overflow horizontal scroll (or not)
        }
        if (item->property_flags & (1 << 4)) {
            node->gap = item->gap;
        }
        if (item->property_flags & (1 << 5)) {
            node->preferred_width = item->preferred_width;
        }
        if (item->property_flags & (1 << 6)) {
            node->min_width = item->min_width;
        }
        if (item->property_flags & (1 << 7)) {
            node->max_width = item->max_width;
        }
        if (item->property_flags & (1 << 8)) {
            node->preferred_height = item->preferred_height;
        }
        if (item->property_flags & (1 << 9)) {
            node->min_height = item->min_height;
        }
        if (item->property_flags & (1 << 10)) {
            node->max_height = item->max_height;
        }
        if (item->property_flags & (1 << 11)) {
            node->horizontal_alignment = item->horizontal_alignment;
        }
        if (item->property_flags & (1 << 12)) {
            node->vertical_alignment = item->vertical_alignment;
        }
        if (item->property_flags & (1 << 13)) {
            node->background_r = item->background_r;
            node->background_g = item->background_g;
            node->background_b = item->background_b;
        }
        if (item->property_flags & (1 << 14)) {
            node->border_r = item->border_r;
            node->border_g = item->border_g;
            node->border_b = item->border_b;
        }
        if (item->property_flags & (1 << 15)) {
            node->border_top = item->border_top;
        }
        if (item->property_flags & (1 << 16)) {
            node->border_bottom = item->border_bottom;
        }
        if (item->property_flags & (1 << 17)) {
            node->border_left = item->border_left;
        }
        if (item->property_flags & (1 << 18)) {
            node->border_right = item->border_right;
        }
        if (item->property_flags & (1 << 19)) {
            node->border_radius_tl = item->border_radius_tl;
        }
        if (item->property_flags & (1 << 20)) {
            node->border_radius_tr = item->border_radius_tr;
        }
        if (item->property_flags & (1 << 21)) {
            node->border_radius_bl = item->border_radius_bl;
        }
        if (item->property_flags & (1 << 22)) {
            node->border_radius_br = item->border_radius_br;
        }
        if (item->property_flags & (1 << 23)) {
            node->pad_top = item->pad_top;
        }
        if (item->property_flags & (1 << 24)) {
            node->pad_bottom = item->pad_bottom;
        }
        if (item->property_flags & (1 << 25)) {
            node->pad_left = item->pad_left;
        }
        if (item->property_flags & (1 << 26)) {
            node->pad_right = item->pad_right;
        }
    }
}

static void NU_Apply_Stylesheet(struct UI_Tree* ui_tree, struct NU_Stylesheet* ss)
{
    // For each layer
    for (int l=0; l<=ui_tree->deepest_layer; l++)
    {
        struct Vector* parent_layer = &ui_tree->tree_stack[l];
        struct Vector* child_layer = &ui_tree->tree_stack[l+1];

        for (int p=0; p<parent_layer->size; p++)
        {
            struct Node* parent = Vector_Get(parent_layer, p);
            for (int i=parent->first_child_index; i<parent->first_child_index + parent->child_count; i++)
            {
                struct Node* child = Vector_Get(child_layer, i);
                NU_Apply_Stylesheet_To_Node(child, ss);
            }
        }
    }
}

int NU_Set_Style(struct UI_Tree* ui_tree, char* css_filepath)
{
    // Open XML source file and load into buffer
    FILE* f = fopen(css_filepath, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", css_filepath, strerror(errno));
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long file_size_long = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size_long > UINT32_MAX) {
        printf("%s", "Src file is too large! It must be < 4 294 967 295 Bytes");
        return 0;
    }
    uint32_t src_length = (uint32_t)file_size_long;
    char* src_buffer = malloc(src_length + 1);
    src_length = fread(src_buffer, 1, file_size_long, f);
    src_buffer[src_length] = '\0'; 
    fclose(f);

    // Init Token vector and reserve ~1MB
    struct Vector tokens;
    Vector_Reserve(&tokens, sizeof(enum NU_Token), 50000); // reserve ~200KB

    struct Vector text_ref_vector;
    Vector_Reserve(&text_ref_vector, sizeof(struct Style_Text_Ref), 4000);

    struct NU_Stylesheet ss;
    NU_Stylesheet_Init(&ss);

    
    NU_Style_Tokenise(src_buffer, src_length, &tokens, &text_ref_vector);
    if (NU_Generate_Stylesheet(src_buffer, src_length, &tokens, &ss, &text_ref_vector) == -1)
    {
        printf("CSS parsing failed!");
    }

    NU_Apply_Stylesheet(ui_tree, &ss);

    Vector_Free(&tokens);
    Vector_Free(&ss.items);

    return 1;
}
