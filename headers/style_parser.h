#pragma once

#include <string.h>
#include <stdint.h>
#include <stdio.h>

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

static enum NU_Style_Token NU_Word_To_Token(char* word, uint8_t word_char_count)
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



static void NU_Style_Tokenise(char* src_buffer, uint32_t src_length, struct Vector* NU_Token_vector)
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
                enum NU_Style_Token token = STYLE_CLASS_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Id selector word completed
            if (ctx == 5 && word_char_index > 0) {
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
                enum NU_Style_Token token = NU_Word_To_Token(word, word_char_index);
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
                enum NU_Style_Token token = STYLE_CLASS_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Id selector word completed
            if (ctx == 5 && word_char_index > 0) {
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



void NU_Parse_CSS(char* filepath)
{
    // Open XML source file and load into buffer
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filepath, strerror(errno));
        return;
    }
    fseek(f, 0, SEEK_END);
    long file_size_long = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size_long > UINT32_MAX) {
        printf("%s", "Src file is too large! It must be < 4 294 967 295 Bytes");
        return;
    }
    uint32_t src_length = (uint32_t)file_size_long;
    char* src_buffer = malloc(src_length + 1);
    src_length = fread(src_buffer, 1, file_size_long, f);
    src_buffer[src_length] = '\0'; 
    fclose(f);

    // Init Token vector and reserve ~1MB
    struct Vector tokens;
    Vector_Reserve(&tokens, sizeof(enum NU_Token), 250000); // reserve ~1MB


    NU_Style_Tokenise(src_buffer, src_length, &tokens);

    for (int i=0; i<tokens.size; i++)
    {
        enum NU_Style_Token* token = Vector_Get(&tokens, i); 
        printfToken(*token);
    }
}