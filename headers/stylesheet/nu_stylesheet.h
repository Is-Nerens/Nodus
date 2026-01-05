#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "nu_convert.h"
#include "nu_style_tokens.h"
#include "nu_stylesheet_structs.h"
#include "nu_style_grammar_assertions.h"
#include "image/nu_image.h"




// -------------------------------------------------
// --- STYLESHEET INIT AND FREE ====================
// -------------------------------------------------
void NU_Stylesheet_Init(NU_Stylesheet* ss)
{
    Vector_Reserve(&ss->items, sizeof(NU_Stylesheet_Item), 500);
    String_Set_Init(&ss->class_string_set, 1024, 100);
    String_Set_Init(&ss->id_string_set, 1024, 100);
    HashmapInit(&ss->class_item_hashmap, sizeof(char*), sizeof(uint32_t), 100);
    HashmapInit(&ss->id_item_hashmap, sizeof(char*), sizeof(uint32_t), 100);
    HashmapInit(&ss->tag_item_hashmap, sizeof(int), sizeof(uint32_t), 20);
    HashmapInit(&ss->tag_pseudo_item_hashmap, sizeof(struct NU_Stylesheet_Tag_Pseudo_Pair), sizeof(uint32_t), 20);
    HashmapInit(&ss->class_pseudo_item_hashmap, sizeof(struct NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 20);
    HashmapInit(&ss->id_pseudo_item_hashmap, sizeof(struct NU_Stylesheet_String_Pseudo_Pair), sizeof(uint32_t), 20);
    NU_Stringmap_Init(&ss->image_filepath_to_handle_hmap, sizeof(GLuint), 512, 20);
    NU_Stringmap_Init(&ss->font_name_index_map, sizeof(uint8_t), 128, 12);
    Vector_Reserve(&ss->fonts, sizeof(NU_Font), 4);
}

void NU_Stylesheet_Free(NU_Stylesheet* ss)
{
    Vector_Free(&ss->items);
    String_Set_Free(&ss->class_string_set);
    String_Set_Free(&ss->id_string_set);
    HashmapFree(&ss->class_item_hashmap);
    HashmapFree(&ss->id_item_hashmap);
    HashmapFree(&ss->tag_item_hashmap);
    HashmapFree(&ss->tag_pseudo_item_hashmap);
    HashmapFree(&ss->class_pseudo_item_hashmap);
    HashmapFree(&ss->id_pseudo_item_hashmap);
    NU_Stringmap_Free(&ss->font_name_index_map);
    Vector_Free(&ss->fonts);
}


// -------------------------------------------------
// --- STYLESHEET PARSING AND CREATION =============
// -------------------------------------------------
struct Style_Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count; 
};

static void NU_Style_Tokenise(char* src_buffer, uint32_t src_length, struct Vector* NU_Token_vector, struct Vector* text_ref_vector)
{
    // Store current NU_Token word;
    uint8_t word_char_index = 0;
    char word[64];

    // Context
    uint8_t ctx = 0; 
    // 0 == globalspace, 1 == global commentspace, 2 == selectorspace, 10 == selector commentspace
    // 3 == property value space, 8 == property value string space, 9 == property value string space escape char (\)
    // 4 == class selector namespace, 5 == id selector name space 
    // 6 == pseudo namespace, 
    // 7 == font creation namespace

    // Iterate over src file
    uint32_t i = 0;
    while (i < src_length)
    {
        char c = src_buffer[i];

        // Globalspace comment begins
        if (ctx == 0 && i < src_length - 1 && c == '/' && src_buffer[i+1] == '*') 
        {
            ctx = 1;
            i += 2;
            continue;
        }

        // Globalspace comment ends 
        if (ctx == 1 && i < src_length - 1 && c == '*' && src_buffer[i+1] == '/')
        {
            ctx = 0;
            i += 2;
            continue;
        }

        // Selectorspace comment begins
        if (ctx == 2 && i < src_length - 1 && c == '/' && src_buffer[i+1] == '*') 
        {
            ctx = 10;
            i += 2;
            continue;
        }

        // Selectorspace comment ends 
        if (ctx == 10 && i < src_length - 1 && c == '*' && src_buffer[i+1] == '/')
        {
            ctx = 2;
            i += 2;
            continue;
        }

        // In comment -> skip rest
        if (ctx == 1 || ctx == 10)
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

        // Property value word completed
        if (ctx == 3 && c == ';'){   
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

        // Property value quotes string completed
        if (ctx == 8 && c == '"') {
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

            ctx = 3;
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
            else if (ctx == 4 && word_char_index > 0) {

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
            else if (ctx == 5 && word_char_index > 0) {

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

            else if (ctx == 7 && word_char_index > 0) {
                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);
                enum NU_Style_Token token = STYLE_FONT_NAME;
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
        if (c == '}')
        {   
            // If word is present -> word completed
            if (word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Style_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
            }

            enum NU_Style_Token token = STYLE_SELECTOR_CLOSE_BRACE;
            Vector_Push(NU_Token_vector, &token);
            ctx = 0;
            i += 1;
            continue;
        }

        // Encountered separation character -> word is completed
        if (c == ' ' || c == '\t' || c == '\n' || c == ',' || c == ':')
        {
            // Tag selector word completed
            if (ctx == 0 && word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Any_Selector_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;

                if (token == STYLE_FONT_CREATION_SELECTOR) { // Special selector context
                    ctx = 7;
                }
            }

            // Class selector word completed
            else if (ctx == 4 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);

                enum NU_Style_Token token = STYLE_CLASS_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
                ctx = 0;
            }

            // Id selector word completed
            else if (ctx == 5 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.NU_Token_index = NU_Token_vector->size;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);
                enum NU_Style_Token token = STYLE_ID_SELECTOR;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
                ctx = 0;
            }

            // Property identifier word completed
            else if (ctx == 2 && word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Style_Property_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            // Property value word completed
            else if (ctx == 3 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);
                enum NU_Style_Token token = STYLE_PROPERTY_VALUE;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
                ctx = 2;
            }

            // Pseudo class word completed
            else if (ctx == 6 && word_char_index > 0) {
                enum NU_Style_Token token = NU_Word_To_Pseudo_Token(word, word_char_index);
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
                ctx = 0;
            }

            // Special font name word completed
            else if (ctx == 7 && word_char_index > 0) {

                // Add text reference
                struct Style_Text_Ref ref;
                ref.src_index = i - (uint32_t)word_char_index;
                ref.char_count = word_char_index;
                Vector_Push(text_ref_vector, &ref);
                enum NU_Style_Token token = STYLE_FONT_NAME;
                Vector_Push(NU_Token_vector, &token);
                word_char_index = 0;
            }

            if (c == ',') {
                enum NU_Style_Token token = STYLE_SELECTOR_COMMA;
                Vector_Push(NU_Token_vector, &token);
            }

            if (c == ':') {
                // Style property assignment token
                if (ctx == 2) {
                    enum NU_Style_Token token = STYLE_PROPERTY_ASSIGNMENT;
                    Vector_Push(NU_Token_vector, &token);
                    ctx = 3;
                }
                // Pseudo class assignment token
                else if (ctx == 0 || ctx == 4 || ctx == 5 ) {
                    enum NU_Style_Token token = STYLE_PSEUDO_COLON;
                    Vector_Push(NU_Token_vector, &token);
                    ctx = 6;
                }
            }

            i += 1;
            continue;
        }

        // Support for "" property values
        if (ctx == 3 && c == '"') {
            ctx = 8;
            i += 1;
            continue;
        }
        if (ctx == 8 && c =='\\') {
            ctx = 9;
            i += 1;
            continue;
        }
        if (ctx == 9) {
            ctx = 8;
            i += 1;
            continue;
        }

        // Add char to word (if ctx == 0 || 2 || 3 || 4 || 5 || 6)
        if (ctx != 1)
        {
            word[word_char_index] = c;
            word_char_index += 1;
        }

        i += 1;
    }
}

static int NU_Stylesheet_Parse(char* src_buffer, uint32_t src_length, struct Vector* tokens, NU_Stylesheet* ss, struct Vector* text_refs)
{
    uint32_t text_index = 0;
    struct Style_Text_Ref* text_ref;

    NU_Stylesheet_Item item;
    item.propertyFlags = 0;
    item.fontId = 0;

    uint32_t selector_indexes[64];
    int selector_count = 0;

    int ctx = 0; // 0 == standard selector; 1 == font creation selector
    uint8_t create_font_index = UINT8_MAX;
    NU_Font* create_font;
    int create_font_size = 18;
    int create_font_weight = 400;
    char* create_font_name = NULL;
    char* create_font_src = NULL;

    int i = 0;
    while(i < tokens->size)
    {
        const enum NU_Style_Token token = *((enum NU_Style_Token*) Vector_Get(tokens, i));

        if (token == STYLE_FONT_CREATION_SELECTOR)
        {
            if (AssertFontCreationSelectorGrammar(tokens, i)) {

                enum NU_Style_Token font_name_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));

                // Get id string
                text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                create_font_name = &src_buffer[text_ref->src_index];
                src_buffer[text_ref->src_index + text_ref->char_count] = '\0';
                text_index += 1;

                // Create a new font
                void* found_font = NU_Stringmap_Get(&ss->font_name_index_map, create_font_name);
                if (found_font == NULL) {
                    NU_Font font;
                    Vector_Push(&ss->fonts, &font);
                    create_font_index = (uint8_t)(ss->fonts.size - 1);
                    create_font = Vector_Get(&ss->fonts, create_font_index);
                    NU_Stringmap_Set(&ss->font_name_index_map, create_font_name, &create_font_index);
                } 

                ctx = 1;
                i += 2;
                continue;
            }
            else {
                return -1;
            }
        }

        if (token == STYLE_SELECTOR_OPEN_BRACE)
        {
            if (AssertSelectionOpeningBraceGrammar(tokens, i)) {
                item.propertyFlags = 0;
                item.layoutFlags = 0;
                item.fontId = 0;
                i += 1;
                continue;
            } 
            else {
                return -1;
            }
        }

        if (token == STYLE_SELECTOR_CLOSE_BRACE)
        {
            if (!AssertSelectionClosingBraceGrammar(tokens, i)) {
                return -1;
            }
                
            if (ctx == 0)
            {   
                for (int i=0; i<selector_count; i++) {
                    uint32_t item_index = selector_indexes[i];
                    NU_Stylesheet_Item* curr_item = Vector_Get(&ss->items, item_index);
                    curr_item->propertyFlags |= item.propertyFlags;
                    if (item.propertyFlags & (1ULL << 0) ) curr_item->layoutFlags = (curr_item->layoutFlags & ~(1ULL << 0)) | (item.layoutFlags & (1ULL << 0)); // Layout direction
                    if (item.propertyFlags & (1ULL << 1) ) curr_item->layoutFlags = (curr_item->layoutFlags & ~((1ULL << 1) | (1ULL << 2))) | (item.layoutFlags & ((1ULL << 1) | (1ULL << 2))); // Grow
                    if (item.propertyFlags & (1ULL << 2 )) curr_item->layoutFlags = (curr_item->layoutFlags & ~(1ULL << 3)) | (item.layoutFlags & (1ULL << 3)); // Overflow vertical scroll (or not)
                    if (item.propertyFlags & (1ULL << 3 )) curr_item->layoutFlags = (curr_item->layoutFlags & ~(1ULL << 4)) | (item.layoutFlags & (1ULL << 4)); // Overflow horizontal scroll (or not)
                    if (item.propertyFlags & (1ULL << 4 )) curr_item->layoutFlags = (curr_item->layoutFlags & ~(1ULL << 5)) | (item.layoutFlags & (1ULL << 5)); // Position absolute or not
                    if (item.propertyFlags & (1ULL << 5 )) curr_item->layoutFlags = (curr_item->layoutFlags & ~(1ULL << 6)) | (item.layoutFlags & (1ULL << 6)); // Hide or not
                    if (item.propertyFlags & (1ULL << 6 )) curr_item->gap = item.gap;
                    if (item.propertyFlags & (1ULL << 7 )) curr_item->preferred_width = item.preferred_width;
                    if (item.propertyFlags & (1ULL << 8 )) curr_item->minWidth = item.minWidth;
                    if (item.propertyFlags & (1ULL << 9 )) curr_item->maxWidth = item.maxWidth;
                    if (item.propertyFlags & (1ULL << 10)) curr_item->preferred_height = item.preferred_height;
                    if (item.propertyFlags & (1ULL << 11)) curr_item->minHeight = item.minHeight;
                    if (item.propertyFlags & (1ULL << 12)) curr_item->maxHeight = item.maxHeight;
                    if (item.propertyFlags & (1ULL << 13)) curr_item->horizontalAlignment = item.horizontalAlignment;
                    if (item.propertyFlags & (1ULL << 14)) curr_item->verticalAlignment = item.verticalAlignment;
                    if (item.propertyFlags & (1ULL << 15)) curr_item->horizontalTextAlignment = item.horizontalTextAlignment;
                    if (item.propertyFlags & (1ULL << 16)) curr_item->verticalTextAlignment = item.verticalTextAlignment;
                    if (item.propertyFlags & (1ULL << 17)) curr_item->left = item.left;
                    if (item.propertyFlags & (1ULL << 18)) curr_item->right = item.right;
                    if (item.propertyFlags & (1ULL << 19)) curr_item->top = item.top;
                    if (item.propertyFlags & (1ULL << 20)) curr_item->bottom = item.bottom;
                    if (item.propertyFlags & (1ULL << 21)) memcpy(&curr_item->backgroundR, &item.backgroundR, 3); // Copy rgb
                    if (item.propertyFlags & (1ULL << 22)) curr_item->hideBackground = item.hideBackground; // Copy rgb
                    if (item.propertyFlags & (1ULL << 23)) memcpy(&curr_item->borderR, &item.borderR, 3); // Copy rgb
                    if (item.propertyFlags & (1ULL << 24)) memcpy(&curr_item->textR, &item.textR, 3); // Copy rgb
                    if (item.propertyFlags & (1ULL << 25)) curr_item->borderTop = item.borderTop; 
                    if (item.propertyFlags & (1ULL << 26)) curr_item->borderBottom = item.borderBottom; 
                    if (item.propertyFlags & (1ULL << 27)) curr_item->borderLeft = item.borderLeft; 
                    if (item.propertyFlags & (1ULL << 28)) curr_item->borderRight = item.borderRight; 
                    if (item.propertyFlags & (1ULL << 29)) curr_item->borderRadiusTl = item.borderRadiusTl; 
                    if (item.propertyFlags & (1ULL << 30)) curr_item->borderRadiusTr = item.borderRadiusTr; 
                    if (item.propertyFlags & (1ULL << 31)) curr_item->borderRadiusBl = item.borderRadiusBl; 
                    if (item.propertyFlags & (1ULL << 32)) curr_item->borderRadiusBr = item.borderRadiusBr; 
                    if (item.propertyFlags & (1ULL << 33)) curr_item->padTop = item.padTop; 
                    if (item.propertyFlags & (1ULL << 34)) curr_item->padBottom = item.padBottom; 
                    if (item.propertyFlags & (1ULL << 35)) curr_item->padLeft = item.padLeft; 
                    if (item.propertyFlags & (1ULL << 36)) curr_item->padRight = item.padRight; 
                    if (item.propertyFlags & (1ULL << 37)) curr_item->glImageHandle = item.glImageHandle;
                    curr_item->fontId = item.fontId;
                }
                selector_count = 0;
            }
            else // ctx == 1
            {
                if (create_font_src != NULL)
                {
                    NU_Font_Create(create_font, create_font_src, create_font_size, true);
                    create_font_index = UINT8_MAX;
                    create_font_size = 18;
                    create_font_weight = 400;
                    create_font_name = NULL;
                    create_font_src = NULL;
                }
                else 
                {
                    printf("[NU_Generate_Stylesheet] Error! created font \"%s\" must have a src!", create_font_name);
                    return -1;
                }

                ctx = 0;
            }

            i += 1;
            continue;
        }

        if (NU_Is_Tag_Selector_Token(token))
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
                if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)    
                {
                    int tag = token - STYLE_PROPERTY_COUNT;
                    void* found = HashmapGet(&ss->tag_item_hashmap, &tag);
                    if (found != NULL) // Style item exists
                    {
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(uint32_t*)found);
                        selector_indexes[selector_count] = *(uint32_t*)found;
                    } 
                    else // Style item does not exist -> add one
                    { 
                        NU_Stylesheet_Item new_item;
                        new_item.class = NULL;
                        new_item.id = NULL;
                        new_item.tag = tag;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                        HashmapSet(&ss->tag_item_hashmap, &tag, &selector_indexes[selector_count]); // Store item index
                    }

                    i += 1;
                    selector_count++;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size - 3) 
                {
                    enum NU_Style_Token following_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+2));
                    enum NU_Style_Token third_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+3));
                    if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) 
                    {
                        int tag = token - STYLE_PROPERTY_COUNT;
                        enum NU_Pseudo_Class pseudo_class = Token_To_Pseudo_Class(following_token);
                        struct NU_Stylesheet_Tag_Pseudo_Pair key = { tag, pseudo_class };
                        void* found = HashmapGet(&ss->tag_pseudo_item_hashmap, &key);

                        if (found != NULL) // Style item exists
                        {
                            NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(uint32_t*)found);
                            selector_indexes[selector_count] = *(uint32_t*)found;
                        }
                        else
                        {
                            NU_Stylesheet_Item new_item;
                            new_item.class = NULL;
                            new_item.id = NULL;
                            new_item.tag = tag;
                            new_item.propertyFlags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                            HashmapSet(&ss->tag_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index

                        }
                    }
                    else 
                    {
                        return -1;
                    }

                    i += 3;
                    selector_count++;
                    continue;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }

        if (token == STYLE_CLASS_SELECTOR)
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
                if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)  
                {
                    // Get class string
                    text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                    char* src_class = &src_buffer[text_ref->src_index];
                    src_buffer[text_ref->src_index + text_ref->char_count] = '\0';

                    // Add class to set
                    char* stored_class = String_Set_Add(&ss->class_string_set, src_class);

                    // If style item exists
                    void* found = HashmapGet(&ss->class_item_hashmap, &stored_class);
                    if (found != NULL) 
                    {
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(uint32_t*)found);
                        selector_indexes[selector_count] = *(uint32_t*)found;
                    } 
                    else // does not exist -> add item
                    { 
                        NU_Stylesheet_Item new_item;
                        new_item.class = stored_class;
                        new_item.id = NULL;
                        new_item.tag = -1;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                        HashmapSet(&ss->class_item_hashmap, &stored_class, &selector_indexes[selector_count]); // Store item index
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size-3) 
                {
                    enum NU_Style_Token following_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+2));
                    enum NU_Style_Token third_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+3));
                    if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) 
                    {
                        enum NU_Pseudo_Class pseudo_class = Token_To_Pseudo_Class(following_token);

                        // Get class string
                        text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                        char* src_class = &src_buffer[text_ref->src_index];
                        src_buffer[text_ref->src_index + text_ref->char_count] = '\0';

                        // Add class to set
                        char* stored_class = String_Set_Add(&ss->class_string_set, src_class);
                        struct NU_Stylesheet_String_Pseudo_Pair key = { stored_class, pseudo_class };
                        void* found = HashmapGet(&ss->class_pseudo_item_hashmap, &key);
                        if (found != NULL) 
                        {
                            NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(uint32_t*)found);
                            selector_indexes[selector_count] = *(uint32_t*)found;
                        }
                        else // does not exist -> add item
                        {
                            NU_Stylesheet_Item new_item;
                            new_item.class = stored_class;
                            new_item.id = NULL;
                            new_item.tag = -1;
                            new_item.propertyFlags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                            HashmapSet(&ss->class_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index
                        }
                    }
                    else 
                    {
                        return -1;
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else
                {
                    return -1;
                }
            }
            else 
            {
                return -1;
            }
        }

        if (token == STYLE_ID_SELECTOR)
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
                if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)  
                {
                    // Get id string
                    text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                    char* src_id = &src_buffer[text_ref->src_index];
                    src_buffer[text_ref->src_index + text_ref->char_count] = '\0';

                    // Add id to set
                    char* stored_id = String_Set_Add(&ss->id_string_set, src_id);

                    // If style item exists
                    void* found = HashmapGet(&ss->id_item_hashmap, &stored_id);
                    if (found != NULL)
                    {
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(uint32_t*)found);
                        selector_indexes[selector_count] = *(uint32_t*)found;
                    }
                    else // does not exist -> add item
                    {
                        NU_Stylesheet_Item new_item;
                        new_item.class = NULL;
                        new_item.id = stored_id;
                        new_item.tag = -1;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                        HashmapSet(&ss->id_item_hashmap, &stored_id, &selector_indexes[selector_count]); // Store item index
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size-3) 
                {
                    enum NU_Style_Token following_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+2));
                    enum NU_Style_Token third_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+3));
                    if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) 
                    {
                        enum NU_Pseudo_Class pseudo_class = Token_To_Pseudo_Class(following_token);

                        // Get id string
                        text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                        char* src_id = &src_buffer[text_ref->src_index];
                        src_buffer[text_ref->src_index + text_ref->char_count] = '\0';

                        // Add id to set
                        char* stored_id = String_Set_Add(&ss->id_string_set, src_id);
                        struct NU_Stylesheet_String_Pseudo_Pair key = { stored_id, pseudo_class };
                        void* found = HashmapGet(&ss->id_pseudo_item_hashmap, &key);
                        if (found != NULL) 
                        {
                            NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(uint32_t*)found);
                            selector_indexes[selector_count] = *(uint32_t*)found;
                        }
                        else // does not exist -> add item
                        {
                            NU_Stylesheet_Item new_item;
                            new_item.class = NULL;
                            new_item.id = stored_id;
                            new_item.tag = -1;
                            new_item.propertyFlags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                            HashmapSet(&ss->id_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index
                        }
                    }
                    else 
                    {
                        return -1;
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else
                {
                    return -1;
                }
            }
            else 
            {
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
            if (!AssertPropertyIdentifierGrammar(tokens, i)) {
                return -1;
            }

            // get property value text
            text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
            char c = src_buffer[text_ref->src_index];
            char* text = &src_buffer[text_ref->src_index];
            src_buffer[text_ref->src_index + text_ref->char_count] = '\0';
            text_index += 1;
            
            if (ctx == 0)
            {
                switch (token)
                {
                    // Set layout direction
                    case STYLE_LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'v') item.layoutFlags |= LAYOUT_VERTICAL;
                        item.propertyFlags |= 1ULL << 0;
                        break;

                    // Set growth 
                    case STYLE_GROW_PROPERTY:
                        switch(c)
                        {
                            case 'v':
                                item.layoutFlags |= GROW_VERTICAL;
                                break;
                            case 'h':
                                item.layoutFlags |= GROW_HORIZONTAL;
                                break;
                            case 'b':
                                item.layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                                break;
                        }
                        item.propertyFlags |= 1ULL << 1;
                        break;
                    
                    // Set overflow behaviour
                    case STYLE_OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            item.layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                            item.propertyFlags |= 1ULL << 2;
                        }
                        break;
                    
                    case STYLE_OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            item.layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                            item.propertyFlags |= 1ULL << 3;
                        }
                        break;

                    // Relative/Absolute positioning
                    case STYLE_POSITION_PROPERTY:
                        if (text_ref->char_count == 8 && memcmp(text, "absolute", 8) == 0) {
                            item.layoutFlags |= POSITION_ABSOLUTE;
                            item.propertyFlags |= 1ULL << 4;
                        }
                        break;

                    // Hide/show
                    case STYLE_HIDE_PROPERTY:
                        if (text_ref->char_count == 4 && memcmp(text, "hide", 4) == 0) {
                            item.layoutFlags |= HIDDEN;
                            item.propertyFlags |= 1ULL << 5;
                        }
                        break;
                    
                    // Set gap
                    case STYLE_GAP_PROPERTY:
                        if (String_To_Float(&item.gap, text)) 
                            item.propertyFlags |= 1ULL << 6;
                        break;

                    // Set preferred width
                    case STYLE_WIDTH_PROPERTY:
                        if (String_To_Float(&item.preferred_width, text))
                            item.propertyFlags |= 1ULL << 7;
                        break;

                    // Set min width
                    case STYLE_MIN_WIDTH_PROPERTY:
                        if (String_To_Float(&item.minWidth, text))
                            item.propertyFlags |= 1ULL << 8;
                        break;
                    
                    // Set max width
                    case STYLE_MAX_WIDTH_PROPERTY:
                        if (String_To_Float(&item.maxWidth, text))
                            item.propertyFlags |= 1ULL << 9;
                        break;

                    // Set preferred height
                    case STYLE_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.preferred_height, text)) 
                            item.propertyFlags |= 1ULL << 10;
                        break;

                    // Set min height
                    case STYLE_MIN_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.minHeight, text)) 
                            item.propertyFlags |= 1ULL << 11;
                        break;

                    // Set max height
                    case STYLE_MAX_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.maxHeight, text)) 
                            item.propertyFlags |= 1ULL << 12;
                        break;

                    // Set horizontal alignment
                    case STYLE_ALIGN_H_PROPERTY:
                        if (text_ref->char_count == 4 && memcmp(text, "left", 4) == 0) {
                            item.horizontalAlignment = 0;
                            item.propertyFlags |= 1ULL << 13;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.horizontalAlignment = 1;
                            item.propertyFlags |= 1ULL << 13;
                        }
                        if (text_ref->char_count == 5 && memcmp(text, "right", 5) == 0) {
                            item.horizontalAlignment = 2;
                            item.propertyFlags |= 1ULL << 13;
                        }
                        break;

                    // Set vertical alignment
                    case STYLE_ALIGN_V_PROPERTY:
                        if (text_ref->char_count == 3 && memcmp(text, "top", 3) == 0) {
                            item.verticalAlignment = 0;
                            item.propertyFlags |= 1ULL << 14;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.verticalAlignment = 1;
                            item.propertyFlags |= 1ULL << 14;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "bottom", 6) == 0) {
                            item.verticalAlignment = 2;
                            item.propertyFlags |= 1ULL << 14;
                        }
                        break;

                    // Set horizontal text alignment
                    case STYLE_TEXT_ALIGN_H_PROPERTY:
                        if (text_ref->char_count == 4 && memcmp(text, "left", 4) == 0) {
                            item.horizontalTextAlignment = 0;
                            item.propertyFlags |= 1ULL << 15;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.horizontalTextAlignment = 1;
                            item.propertyFlags |= 1ULL << 15;
                        }
                        if (text_ref->char_count == 5 && memcmp(text, "right", 5) == 0) {
                            item.horizontalTextAlignment = 2;
                            item.propertyFlags |= 1ULL << 15;
                        }
                        break;

                    // Set vertical text alignment
                    case STYLE_TEXT_ALIGN_V_PROPERTY:
                        if (text_ref->char_count == 3 && memcmp(text, "top", 3) == 0) {
                            item.verticalTextAlignment = 0;
                            item.propertyFlags |= 1ULL << 16;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.verticalTextAlignment = 1;
                            item.propertyFlags |= 1ULL << 16;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "bottom", 6) == 0) {
                            item.verticalTextAlignment = 2;
                            item.propertyFlags |= 1ULL << 16;
                        }
                        break;

                    // Set absolute position properties
                    case STYLE_LEFT_PROPERTY:
                        float abs_position; 
                        if (String_To_Float(&abs_position, text)) {
                            item.left = abs_position;
                            item.propertyFlags |= 1ULL << 17;
                        }
                        break;

                    case STYLE_RIGHT_PROPERTY:
                        if (String_To_Float(&abs_position, text)) {
                            item.right = abs_position;
                            item.propertyFlags |= 1ULL << 18;
                        }
                        break;

                    case STYLE_TOP_PROPERTY:
                        if (String_To_Float(&abs_position, text)) {
                            item.top = abs_position;
                            item.propertyFlags |= 1ULL << 19;
                        }
                        break;

                    case STYLE_BOTTOM_PROPERTY:
                        if (String_To_Float(&abs_position, text)) {
                            item.bottom = abs_position;
                            item.propertyFlags |= 1ULL << 20;
                        }
                        break;

                    // Set background colour
                    case STYLE_BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.backgroundR = rgb.r;
                            item.backgroundG = rgb.g;
                            item.backgroundB = rgb.b;
                            item.propertyFlags |= 1ULL << 21;
                        } else if (text_ref->char_count == 4 && memcmp(text, "none", 4) == 0) {
                            item.hideBackground = true;
                            item.propertyFlags |= 1ULL << 22;
                        }
                        break;

                    // Set border colour
                    case STYLE_BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.borderR = rgb.r;
                            item.borderG = rgb.g;
                            item.borderB = rgb.b;
                            item.propertyFlags |= 1ULL << 23;
                        }
                        break;

                    // Set text colour
                    case STYLE_TEXT_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.textR = rgb.r;
                            item.textG = rgb.g;
                            item.textB = rgb.b;
                            item.propertyFlags |= 1ULL << 24;
                        }
                        break;
                    
                    // Set border width
                    case STYLE_BORDER_WIDTH_PROPERTY:
                        uint8_t border_width;
                        if (String_To_uint8_t(&border_width, text)) {
                            item.borderTop = border_width;
                            item.borderBottom = border_width;
                            item.borderLeft = border_width;
                            item.borderRight = border_width;
                            item.propertyFlags |= 1ULL << 25;
                            item.propertyFlags |= 1ULL << 26;
                            item.propertyFlags |= 1ULL << 27;
                            item.propertyFlags |= 1ULL << 28;
                        }
                        break;
                    case STYLE_BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.borderTop, text)) {
                            item.propertyFlags |= 1ULL << 25;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.borderBottom, text)) {
                            item.propertyFlags |= 1ULL << 26;
                        }
                        break;
                    case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.borderLeft, text)) {
                            item.propertyFlags |= 1ULL << 27;
                        }
                        break;
                    case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.borderRight, text)) {
                            item.propertyFlags |= 1ULL << 28;
                        }
                        break;

                    // Set border radii
                    case STYLE_BORDER_RADIUS_PROPERTY:
                        uint8_t border_radius;
                        if (String_To_uint8_t(&border_radius, text)) {
                            item.borderRadiusTl = border_radius;
                            item.borderRadiusTr = border_radius;
                            item.borderRadiusBl = border_radius;
                            item.borderRadiusBr = border_radius;
                            item.propertyFlags |= 1ULL << 29;
                            item.propertyFlags |= 1ULL << 30;
                            item.propertyFlags |= 1ULL << 31;
                            item.propertyFlags |= 1ULL << 32;
                        }
                        break;
                    case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.borderRadiusTl, text)) {
                            item.propertyFlags |= 1ULL << 29;
                        }
                        break;
                    case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.borderRadiusTr, text)) {
                            item.propertyFlags |= 1ULL << 30;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.borderRadiusBl, text)) {
                            item.propertyFlags |= 1ULL << 31;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.borderRadiusBr, text)) {
                            item.propertyFlags |= 1ULL << 32;
                        }
                        break;

                    // Set padding
                    case STYLE_PADDING_PROPERTY:
                        uint8_t pad;
                        if (String_To_uint8_t(&pad, text)) {
                            item.padTop = pad;
                            item.padBottom = pad;
                            item.padLeft = pad;
                            item.padRight = pad;
                            item.propertyFlags |= 1ULL << 33;
                            item.propertyFlags |= 1ULL << 34;
                            item.propertyFlags |= 1ULL << 35;
                            item.propertyFlags |= 1ULL << 36;
                        }
                        break;
                    case STYLE_PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&item.padTop, text)) {
                            item.propertyFlags |= 1ULL << 33;
                        }
                        break;
                    case STYLE_PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&item.padBottom, text)) {
                            item.propertyFlags |= 1ULL << 34;
                        }
                        break;
                    case STYLE_PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&item.padLeft, text)) {
                            item.propertyFlags |= 1ULL << 35;
                        }
                        break;
                    case STYLE_PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&item.padRight, text)) {
                            item.propertyFlags |= 1ULL << 36;
                        }
                        break;
                    
                    case STYLE_IMAGE_SOURCE_PROPERTY:
                        void* found = NU_Stringmap_Get(&ss->image_filepath_to_handle_hmap, text);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(text);
                            if (image_handle) {
                                item.glImageHandle = image_handle;
                                NU_Stringmap_Set(&ss->image_filepath_to_handle_hmap, text, &image_handle);
                                item.propertyFlags |= 1ULL << 37;
                            }
                        } 
                        else {
                            item.propertyFlags |= 1ULL << 37;
                            item.glImageHandle = *(GLuint*)found;
                        }
                        break;

                    case STYLE_FONT_PROPERTY:
                        void* found_font = NU_Stringmap_Get(&ss->font_name_index_map, text);
                        if (found_font != NULL) {
                            item.fontId = *(uint8_t*)found_font;
                        }
                        break;

                    default:
                        break;
                }
            }
            else // ctx == 1
            {
                switch (token)
                {
                    case STYLE_FONT_SRC:
                        create_font_src = text;
                        break;

                    case STYLE_FONT_SIZE:
                        int size = 0;
                        if (String_To_Int(&size, text)) {
                            create_font_size = size;
                        }
                        break;

                    case STYLE_FONT_WEIGHT:
                        int weight = 0;
                        if (String_To_Int(&weight, text)) {
                            create_font_weight = weight;
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        i += 3;
    }

    // Ensure that at least one font is present
    if (ss->fonts.size == 0) {
        printf("[NU_Generate_Stylesheet] Error! at least one font must be provided!\n");
        return -1;
    }

    return 0;
}

int NU_Stylesheet_Create(NU_Stylesheet* stylesheet, char* css_filepath)
{
    NU_Stylesheet_Init(stylesheet);

    // Open XML source file and load into buffer
    FILE* f = fopen(css_filepath, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file '%s': %s\n", css_filepath, strerror(errno)); return 0;
    }
    fseek(f, 0, SEEK_END);
    long file_size_long = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size_long > UINT32_MAX) {
        printf("%s", "Src file is too large! It must be < 4 294 967 295 Bytes"); return 0;
    }
    uint32_t src_length = (uint32_t)file_size_long;
    char* src_buffer = malloc(src_length + 1);
    src_length = fread(src_buffer, 1, file_size_long, f);
    src_buffer[src_length] = '\0'; 
    fclose(f);

    // Init token and text ref vectors and reserve
    struct Vector tokens;
    struct Vector text_ref_vector;
    Vector_Reserve(&tokens, sizeof(enum NU_Style_Token), 10000);
    Vector_Reserve(&text_ref_vector, sizeof(struct Style_Text_Ref), 4000);

    // Tokenise and generate stylesheet
    NU_Style_Tokenise(src_buffer, src_length, &tokens, &text_ref_vector);
    if (NU_Stylesheet_Parse(src_buffer, src_length, &tokens, stylesheet, &text_ref_vector) == -1) {
        printf("CSS parsing failed!"); return 0;
    }

    // Free token and text reference memory
    Vector_Free(&tokens);
    Vector_Free(&text_ref_vector);
    
    return 1;// Success
}




// -------------------------------------------------
// --- STYLESHEET APPLICATION TO NODES =============
// -------------------------------------------------
static void NU_Stylesheet_Find_Match(struct Node* node, NU_Stylesheet* ss, int* match_index_list)
{
    int count = 0;

    // Tag match first (lowest priority)
    void* tag_found = HashmapGet(&ss->tag_item_hashmap, &node->tag); 
    if (tag_found != NULL) { 
        match_index_list[count++] = (int)*(uint32_t*)tag_found;
    }

    // Class match second
    if (node->class != NULL) { 
        char* stored_class = String_Set_Get(&ss->class_string_set, node->class);
        if (stored_class != NULL) {
            void* class_found = HashmapGet(&ss->class_item_hashmap, &stored_class);
            if (class_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)class_found;
            }
        }
    }

    // ID match last (highest priority)
    if (node->id != NULL) { 
        char* stored_id = String_Set_Get(&ss->id_string_set, node->id);
        if (stored_id != NULL) {
            void* id_found = HashmapGet(&ss->id_item_hashmap, &stored_id);
            if (id_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)id_found;
            }
        }
    }
}

static void NU_Apply_Style_Item_To_Node(struct Node* node, NU_Stylesheet_Item* item)
{
    if (item->propertyFlags & (1ULL << 0) && !(node->inlineStyleFlags & (1ULL << 0))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 0)) | (item->layoutFlags & (1ULL << 0)); // Layout direction
    if (item->propertyFlags & (1ULL << 1) && !(node->inlineStyleFlags & (1ULL << 1))) {
        node->layoutFlags = (node->layoutFlags & ~(1ULL << 1)) | (item->layoutFlags & (1ULL << 1)); // Grow horizontal
        node->layoutFlags = (node->layoutFlags & ~(1ULL << 2)) | (item->layoutFlags & (1ULL << 2)); // Grow vertical
    }
    if (item->propertyFlags & (1ULL << 2) && !(node->inlineStyleFlags & (1ULL << 2))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 3)) | (item->layoutFlags & (1ULL << 3)); // Overflow vertical scroll (or not)
    if (item->propertyFlags & (1ULL << 3) && !(node->inlineStyleFlags & (1ULL << 3))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 4)) | (item->layoutFlags & (1ULL << 4)); // Overflow horizontal scroll (or not)
    if (item->propertyFlags & (1ULL << 4) && !(node->inlineStyleFlags & (1ULL << 4))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 5)) | (item->layoutFlags & (1ULL << 5)); // Absolute positioning (or not)
    if (item->propertyFlags & (1ULL << 5) && !(node->inlineStyleFlags & (1ULL << 5))) node->layoutFlags = (node->layoutFlags & ~(1ULL << 6)) | (item->layoutFlags & (1ULL << 6)); // Hidden or not
    if (item->propertyFlags & (1ULL << 6) && !(node->inlineStyleFlags & (1ULL << 6))) node->gap = item->gap;
    if (item->propertyFlags & (1ULL << 7) && !(node->inlineStyleFlags & (1ULL << 7))) node->preferred_width = item->preferred_width;
    if (item->propertyFlags & (1ULL << 8) && !(node->inlineStyleFlags & (1ULL << 8))) node->minWidth = item->minWidth;
    if (item->propertyFlags & (1ULL << 9) && !(node->inlineStyleFlags & (1ULL << 9))) node->maxWidth = item->maxWidth;
    if (item->propertyFlags & (1ULL << 10) && !(node->inlineStyleFlags & (1ULL << 10))) node->preferred_height = item->preferred_height;
    if (item->propertyFlags & (1ULL << 11) && !(node->inlineStyleFlags & (1ULL << 11))) node->minHeight = item->minHeight;
    if (item->propertyFlags & (1ULL << 12) && !(node->inlineStyleFlags & (1ULL << 12))) node->maxHeight = item->maxHeight;
    if (item->propertyFlags & (1ULL << 13) && !(node->inlineStyleFlags & (1ULL << 13))) node->horizontalAlignment = item->horizontalAlignment;
    if (item->propertyFlags & (1ULL << 14) && !(node->inlineStyleFlags & (1ULL << 14))) node->verticalAlignment = item->verticalAlignment;
    if (item->propertyFlags & (1ULL << 15) && !(node->inlineStyleFlags & (1ULL << 15))) node->horizontalTextAlignment = item->horizontalTextAlignment;
    if (item->propertyFlags & (1ULL << 16) && !(node->inlineStyleFlags & (1ULL << 16))) node->verticalTextAlignment = item->verticalTextAlignment;
    if (item->propertyFlags & (1ULL << 17) && !(node->inlineStyleFlags & (1ULL << 17))) node->left = item->left;
    if (item->propertyFlags & (1ULL << 18) && !(node->inlineStyleFlags & (1ULL << 18))) node->right = item->right;
    if (item->propertyFlags & (1ULL << 19) && !(node->inlineStyleFlags & (1ULL << 19))) node->top = item->top;
    if (item->propertyFlags & (1ULL << 20) && !(node->inlineStyleFlags & (1ULL << 20))) node->bottom = item->bottom;
    if (item->propertyFlags & (1ULL << 21) && !(node->inlineStyleFlags & (1ULL << 21))) {
        node->backgroundR = item->backgroundR;
        node->backgroundG = item->backgroundG;
        node->backgroundB = item->backgroundB;
    }
    if (item->propertyFlags & (1ULL << 22) && !(node->inlineStyleFlags & (1ULL << 22))) node->hideBackground = item->hideBackground;
    if (item->propertyFlags & (1ULL << 23) && !(node->inlineStyleFlags & (1ULL << 23))) {
        node->borderR = item->borderR;
        node->borderG = item->borderG;
        node->borderB = item->borderB;
    }
    if (item->propertyFlags & (1ULL << 24) && !(node->inlineStyleFlags & (1ULL << 24))) {
        node->textR = item->textR;
        node->textG = item->textG;
        node->textB = item->textB;
    }
    if (item->propertyFlags & (1ULL << 25) && !(node->inlineStyleFlags & (1ULL << 25))) node->borderTop = item->borderTop;
    if (item->propertyFlags & (1ULL << 26) && !(node->inlineStyleFlags & (1ULL << 26))) node->borderBottom = item->borderBottom;
    if (item->propertyFlags & (1ULL << 27) && !(node->inlineStyleFlags & (1ULL << 27))) node->borderLeft = item->borderLeft;
    if (item->propertyFlags & (1ULL << 28) && !(node->inlineStyleFlags & (1ULL << 28))) node->borderRight = item->borderRight;
    if (item->propertyFlags & (1ULL << 29) && !(node->inlineStyleFlags & (1ULL << 29))) node->borderRadiusTl = item->borderRadiusTl;
    if (item->propertyFlags & (1ULL << 30) && !(node->inlineStyleFlags & (1ULL << 30))) node->borderRadiusTr = item->borderRadiusTr;
    if (item->propertyFlags & (1ULL << 31) && !(node->inlineStyleFlags & (1ULL << 31))) node->borderRadiusBl = item->borderRadiusBl;
    if (item->propertyFlags & (1ULL << 32) && !(node->inlineStyleFlags & (1ULL << 32))) node->borderRadiusBr = item->borderRadiusBr;
    if (item->propertyFlags & (1ULL << 33) && !(node->inlineStyleFlags & (1ULL << 33))) node->padTop = item->padTop;
    if (item->propertyFlags & (1ULL << 34) && !(node->inlineStyleFlags & (1ULL << 34))) node->padBottom = item->padBottom;
    if (item->propertyFlags & (1ULL << 35) && !(node->inlineStyleFlags & (1ULL << 35))) node->padLeft = item->padLeft;
    if (item->propertyFlags & (1ULL << 36) && !(node->inlineStyleFlags & (1ULL << 36))) node->padRight = item->padRight;
    if (item->propertyFlags & (1ULL << 37) && !(node->inlineStyleFlags & (1ULL << 37))) node->glImageHandle = item->glImageHandle;
    node->fontId = item->fontId; // set font 
}

void NU_Apply_Stylesheet_To_Node(struct Node* node, NU_Stylesheet* ss)
{
    int match_index_list[3] = {-1, -1, -1};
    NU_Stylesheet_Find_Match(node, ss, &match_index_list[0]);

    int i = 0;
    while (match_index_list[i] != -1) {
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, (uint32_t)match_index_list[i]);
        i += 1;

        // --- Apply style ---
        NU_Apply_Style_Item_To_Node(node, item);
    }
}

void NU_Apply_Tag_Style_To_Node(struct Node* node, NU_Stylesheet* ss)
{
    // Tag match first (lowest priority)
    void* tag_found = HashmapGet(&ss->tag_item_hashmap, &node->tag); 
    if (tag_found != NULL) { 
        uint32_t index = *(uint32_t*)tag_found;
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
        NU_Apply_Style_Item_To_Node(node, item);
    }
}

void NU_Apply_Pseudo_Style_To_Node(struct Node* node, NU_Stylesheet* ss, enum NU_Pseudo_Class pseudo)
{
    // Tag pseudo style match and apply
    struct NU_Stylesheet_Tag_Pseudo_Pair key = { node->tag, pseudo };
    void* tag_pseudo_found = HashmapGet(&ss->tag_pseudo_item_hashmap, &key);
    if (tag_pseudo_found != NULL) {
        uint32_t index = *(uint32_t*)tag_pseudo_found;
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
        NU_Apply_Style_Item_To_Node(node, item);
    }

    // Class pseudo style match and apply
    if (node->class != NULL) {
        char* stored_class = String_Set_Get(&ss->class_string_set, node->class);
        if (stored_class != NULL) {
            struct NU_Stylesheet_String_Pseudo_Pair key = { stored_class, pseudo };
            void* class_pseudo_found = HashmapGet(&ss->class_pseudo_item_hashmap, &key);
            if (class_pseudo_found != NULL) {
                uint32_t index = *(uint32_t*)class_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }

    // Id pseudo style match and apply
    if (node->id != NULL) {
        char* stored_id = String_Set_Get(&ss->id_string_set, node->id);
        if (stored_id != NULL) {
            struct NU_Stylesheet_String_Pseudo_Pair key = { stored_id, pseudo };
            void* id_pseudo_found = HashmapGet(&ss->id_pseudo_item_hashmap, &key);
            if (id_pseudo_found != NULL) {
                uint32_t index = *(uint32_t*)id_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }
}

int NU_Internal_Apply_Stylesheet(uint32_t stylesheet_handle)
{
    // Passing in a corrupted stylesheet
    if (stylesheet_handle == -1) return 0;

    NU_Stylesheet* stylesheet = Vector_Get(&__NGUI.stylesheets, stylesheet_handle - 1);   

    struct Node* root_window = NU_Tree_Get(&__NGUI.tree, 0, 0);
    NU_Apply_Stylesheet_To_Node(root_window, stylesheet);

    // For each layer
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++)
    {
        NU_Layer* layer = &__NGUI.tree.layers[l];
        for (uint32_t i=0; i<layer->size; i++)
        {
            struct Node* node = NU_Layer_Get(layer, i); if (!node->nodeState) continue;
            NU_Apply_Stylesheet_To_Node(node, stylesheet);
        }
    }

    __NGUI.stylesheet = stylesheet;
    return 1; // success
}
