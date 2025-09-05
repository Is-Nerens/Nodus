#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "nu_convert.h"

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
    "borderRadius", "borderTopLeftRadius", "borderTopRightRadius", "borderBottomLeftRadius", "borderBottomRightRadius",
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

static const size_t property_data_sizes[] = {
    1, // layout direction
    1, // grow direction
    1, // overflow v
    1, // overflow h
    4, // gap
    4, // width
    4, // min width
    4, // max width
    4, // height
    4, // min height
    4, // max height
    1, // aligh h
    1, // align v
    3, // background colour
    3, // border colour
    1, // border top
    1, // border bottom
    1, // border left
    1, // border right
    1, // border top left radius
    1, // border top right radius
    1, // border bottom left radius
    1, // border bottom right radius
    1, // pad top
    1, // pad bottom
    1, // pad left
    1  // pad right
};

static size_t Property_Token_To_Data_Size(enum NU_Style_Token token)
{
    return property_data_sizes[token];
}

static uint32_t Property_Token_To_Flag(enum NU_Style_Token token)
{
    return (1u << token);
}

struct NU_Stylesheet
{
    void* items; 
    void* property_data;
    uint32_t item_count;
    uint32_t item_capacity;
    uint32_t item_size;
    uint32_t property_data_consumed;
    uint32_t property_data_capacity;
};

struct NU_Stylesheet_Item
{
    uint32_t property_flags;
    uint32_t property_data_index;
    uint32_t property_data_size;
    uint8_t selector; // 0 = tag, 1 = class, 2 = id
};

static inline int property_bit_index(uint32_t flag) {
    return __builtin_ctz(flag); // GCC/Clang, maps flag -> bit index
}

void NU_Stylesheet_Init(struct NU_Stylesheet* ss)
{
    // item data
    ss->item_size = (uint32_t)sizeof(struct NU_Stylesheet_Item);
    ss->items = malloc(ss->item_size * 500); // ~6KB
    ss->item_count = 0;
    ss->item_capacity = 500;

    // property data 
    ss->property_data = malloc(32784); // 32KB
    ss->property_data_consumed = 0;
    ss->property_data_capacity = 32784;
}

void Stylesheet_New_Item(struct NU_Stylesheet* ss)
{
    ss->item_count += 1;
    if (ss->item_count > ss->item_capacity) {
        ss->item_capacity *= 2;
        ss->items = realloc(ss->items, ss->item_capacity * ss->item_size);
    }
}

void Stylesheet_Push_Property(struct NU_Stylesheet* ss, enum NU_Style_Token property_token, void* data)
{
    size_t size = Property_Token_To_Data_Size(property_token);
    printf("%llu\n", size);

    // --- Ensure we have enough space in the property_data buffer
    if (ss->property_data_consumed + size > ss->property_data_capacity) {
        // grow capacity (double strategy)
        uint32_t new_capacity = ss->property_data_capacity * 2;
        while (ss->property_data_consumed + size > new_capacity) {
            new_capacity *= 2;
        }

        void* new_block = realloc(ss->property_data, new_capacity);
        if (!new_block) {
            fprintf(stderr, "Stylesheet realloc failed\n");
            exit(1);
        }

        ss->property_data = new_block;
        ss->property_data_capacity = new_capacity;
    }

    uint32_t property_flag = Property_Token_To_Flag(property_token);
    struct NU_Stylesheet_Item* items = (struct NU_Stylesheet_Item*)ss->items;
    struct NU_Stylesheet_Item* item  = &items[ss->item_count - 1];

    uint8_t* base = (uint8_t*)ss->property_data;
    uint8_t* item_start = base + item->property_data_index;
    int new_bit = property_bit_index(property_flag);
    uint32_t flags = item->property_flags;
    uint32_t mask = 1u << new_bit;
    if (flags & mask) {
        return;
    }

    // Walk existing properties to find insertion offset
    int insert_offset = 0;
    uint8_t* cursor = item_start;
    for (int bit = 0; bit < 32; bit++) {
        if (flags & (1u << bit)) {
            // measure property size somehow
            // (requires knowing sizes per property type)
            size_t existing_size = Property_Token_To_Data_Size(property_token);
            if (bit > new_bit) break; // found insertion point
            insert_offset += existing_size;
            cursor += existing_size;
        }
    }

    uint32_t tail_size = item->property_data_size - insert_offset;
    memmove(cursor + size, cursor, tail_size);
    memcpy(cursor, data, size);
    item->property_flags |= property_flag;
    ss->property_data_consumed += size;
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


    int i = 0;
    while(i < tokens->size)
    {
        const enum NU_Style_Token token = *((enum NU_Style_Token*) Vector_Get(tokens, i));

        if (token == STYLE_SELECTOR_OPEN_BRACE)
        {
            if (AssertSelectionOpeningBraceGrammar(tokens, i)) {
                Stylesheet_New_Item(ss);
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
                        uint8_t layout_dir;
                        if (c == 'h') 
                            layout_dir = 0;    
                        else 
                            layout_dir = 1;
                            Stylesheet_Push_Property(ss, token, &layout_dir);
                        break;

                    // Set growth 
                    case STYLE_GROW_PROPERTY:
                        uint8_t grow;
                        switch(c)
                        {
                            case 'v':
                                grow = 0;
                                break;
                            case 'h':
                                grow = 1;
                                break;
                            case 'b':
                                grow = 3;
                                break;
                        }
                        Stylesheet_Push_Property(ss, token, &grow);
                        break;
                    
                    // Set overflow behaviour
                    case STYLE_OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            uint8_t overflow_v = 1;
                            Stylesheet_Push_Property(ss, token, &overflow_v);
                        }
                        break;
                    
                    case STYLE_OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            uint8_t overflow_h = 1;
                            Stylesheet_Push_Property(ss, token, &overflow_h);
                        }
                        break;
                    
                    // Set gap
                    case STYLE_GAP_PROPERTY:
                        float gap;
                        if (String_To_Float(&gap, text) == 0)
                            Stylesheet_Push_Property(ss, token, &gap);
                        break;

                    // Set preferred width
                    case STYLE_WIDTH_PROPERTY:
                        float width;
                        if (String_To_Float(&width, text) == 0)
                            Stylesheet_Push_Property(ss, token, &width);
                        break;

                    // Set min width
                    case STYLE_MIN_WIDTH_PROPERTY:
                        float min_width;
                        if (String_To_Float(&min_width, text) == 0)
                            Stylesheet_Push_Property(ss, token, &min_width);
                        break;
                    
                    // Set max width
                    case STYLE_MAX_WIDTH_PROPERTY:
                        float max_width;
                        if (String_To_Float(&max_width, text) == 0)
                            Stylesheet_Push_Property(ss, token, &max_width);
                        break;

                    // Set preferred height
                    case STYLE_HEIGHT_PROPERTY:
                        float height;
                        if (String_To_Float(&height, text) == 0) 
                            Stylesheet_Push_Property(ss, token, &height);
                        break;

                    // Set min height
                    case STYLE_MIN_HEIGHT_PROPERTY:
                        float min_height;
                        if (String_To_Float(&min_height, text) == 0) 
                            Stylesheet_Push_Property(ss, token, &min_height);
                        break;

                    // Set max height
                    case STYLE_MAX_HEIGHT_PROPERTY:
                        float max_height;
                        if (String_To_Float(&max_height, text) == 0) 
                            Stylesheet_Push_Property(ss, token, &max_height);
                        break;

                    // Set horizontal alignment
                    case STYLE_ALIGN_H_PROPERTY:
                        uint8_t horizontal_alignment;
                        if (text_ref->char_count == 4 && memcmp(text, "left", 4) == 0) {
                            horizontal_alignment = 0;
                            Stylesheet_Push_Property(ss, token, &horizontal_alignment);
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            horizontal_alignment = 1;
                            Stylesheet_Push_Property(ss, token, &horizontal_alignment);
                        }
                        if (text_ref->char_count == 5 && memcmp(text, "right", 5) == 0) {
                            horizontal_alignment = 2;
                            Stylesheet_Push_Property(ss, token, &horizontal_alignment);
                        }
                        break;

                    // Set vertical alignment
                    case STYLE_ALIGN_V_PROPERTY:
                        uint8_t vertical_alignment;
                        if (text_ref->char_count == 3 && memcmp(text, "top", 3) == 0) {
                            vertical_alignment = 0;
                            Stylesheet_Push_Property(ss, token, &vertical_alignment);
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            vertical_alignment = 1;
                            Stylesheet_Push_Property(ss, token, &vertical_alignment);
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "bottom", 6) == 0) {
                            vertical_alignment = 2;
                            Stylesheet_Push_Property(ss, token, &vertical_alignment);
                        }
                        break;

                    // Set background colour
                    case STYLE_BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            Stylesheet_Push_Property(ss, token, &rgb);
                        }
                        break;

                    // Set border colour
                    case STYLE_BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            Stylesheet_Push_Property(ss, token, &rgb);
                        }
                        break;
                    
                    // Set border width
                    case STYLE_BORDER_WIDTH_PROPERTY:
                        uint8_t border_width;
                        if (String_To_uint8_t(&border_width, text)) {
                            Stylesheet_Push_Property(ss, STYLE_BORDER_TOP_WIDTH_PROPERTY, &border_width);
                            Stylesheet_Push_Property(ss, STYLE_BORDER_BOTTOM_WIDTH_PROPERTY, &border_width);
                            Stylesheet_Push_Property(ss, STYLE_BORDER_LEFT_WIDTH_PROPERTY, &border_width);
                            Stylesheet_Push_Property(ss, STYLE_BORDER_RIGHT_WIDTH_PROPERTY, &border_width);
                        }
                        break;
                    case STYLE_BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&border_width, text)) {
                            Stylesheet_Push_Property(ss, token, &border_width);
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&border_width, text)) {
                            Stylesheet_Push_Property(ss, token, &border_width);
                        }
                        break;
                    case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&border_width, text)) {
                            Stylesheet_Push_Property(ss, token, &border_width);
                        }
                        break;
                    case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&border_width, text)) {
                            Stylesheet_Push_Property(ss, token, &border_width);
                        }
                        break;

                    // Set border radii
                    case STYLE_BORDER_RADIUS_PROPERTY:
                        uint8_t border_radius;
                        if (String_To_uint8_t(&border_radius, text)) {
                            Stylesheet_Push_Property(ss, STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY, &border_radius);
                            Stylesheet_Push_Property(ss, STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY, &border_radius);
                            Stylesheet_Push_Property(ss, STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY, &border_radius);
                            Stylesheet_Push_Property(ss, STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY, &border_radius);
                        }
                        break;
                    case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&border_radius, text)) {
                            Stylesheet_Push_Property(ss, token, &border_radius);
                        }
                        break;
                    case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&border_radius, text)) {
                            Stylesheet_Push_Property(ss, token, &border_radius);
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&border_radius, text)) {
                            Stylesheet_Push_Property(ss, token, &border_radius);
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&border_radius, text)) {
                            Stylesheet_Push_Property(ss, token, &border_radius);
                        }
                        break;

                    // Set padding
                    case STYLE_PADDING_PROPERTY:
                        uint8_t pad;
                        if (String_To_uint8_t(&border_radius, text)) {
                            Stylesheet_Push_Property(ss, STYLE_PADDING_TOP_PROPERTY, &pad);
                            Stylesheet_Push_Property(ss, STYLE_PADDING_BOTTOM_PROPERTY, &pad);
                            Stylesheet_Push_Property(ss, STYLE_PADDING_LEFT_PROPERTY, &pad);
                            Stylesheet_Push_Property(ss, STYLE_PADDING_RIGHT_PROPERTY, &pad);
                        }
                        break;
                    case STYLE_PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&pad, text)) {
                            Stylesheet_Push_Property(ss, token, &pad);
                        }
                        break;
                    case STYLE_PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&pad, text)) {
                            Stylesheet_Push_Property(ss, token, &pad);
                        }
                        break;
                    case STYLE_PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&pad, text)) {
                            Stylesheet_Push_Property(ss, token, &pad);
                        }
                        break;
                    case STYLE_PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&pad, text)) {
                            Stylesheet_Push_Property(ss, token, &pad);
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

static void NU_Apply_Stylesheet(struct UI_Tree* ui_tree, struct NU_Stylesheet ss)
{
    for (node)
    {
        check Tag
        check _CLASS
        check id
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

    for (int i=0; i<tokens.size; i++)
    {
        enum NU_Style_Token* token = Vector_Get(&tokens, i); 
        printfToken(*token);
    }

    for (int i=0; i<ss.property_data_consumed; i++)
    {
        printf("%c", *(char*)&ss.property_data[i]);
    }

    Vector_Free(&tokens);

    return 1;
}
