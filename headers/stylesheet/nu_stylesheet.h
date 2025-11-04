#pragma once

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "nu_convert.h"
#include "nu_style_tokens.h"
#include "nu_stylesheet_structs.h"
#include "nu_style_grammar_assertions.h"




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
    // 0 == globalspace, 1 == commentspace, 2 == selectorspace, 
    // 3 == propertyspace, 4 == class selector namespace, 5 == id selector name space 
    // 6 == pseudo namespace, 
    // 7 == font creation namespace

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

        // Property value word completed
        if (ctx == 3  && c == ';')
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
                enum NU_Style_Token token = NU_Word_To_Property_Token(word, word_char_index);
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
    item.property_flags = 0;
    item.font_id = 0;

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
                void* found_font = String_Map_Get(&ss->font_name_index_map, create_font_name);
                if (found_font == NULL) {
                    NU_Font font;
                    Vector_Push(&ss->fonts, &font);
                    create_font_index = (uint8_t)(ss->fonts.size - 1);
                    create_font = Vector_Get(&ss->fonts, create_font_index);
                    String_Map_Set(&ss->font_name_index_map, create_font_name, &create_font_index);
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
                item.property_flags = 0;
                item.layout_flags = 0;
                item.font_id = 0;
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
                    curr_item->property_flags |= item.property_flags;
                    if (item.property_flags & (1 << 0) ) curr_item->layout_flags = (curr_item->layout_flags & ~(1 << 0)) | (item.layout_flags & (1 << 0)); // Layout direction
                    if (item.property_flags & (1 << 1) ) curr_item->layout_flags = (curr_item->layout_flags & ~((1 << 1) | (1 << 2))) | (item.layout_flags & ((1 << 1) | (1 << 2))); // Grow
                    if (item.property_flags & (1 << 2 )) curr_item->layout_flags = (curr_item->layout_flags & ~(1 << 3)) | (item.layout_flags & (1 << 3)); // Overflow vertical scroll (or not)
                    if (item.property_flags & (1 << 3 )) curr_item->layout_flags = (curr_item->layout_flags & ~(1 << 4)) | (item.layout_flags & (1 << 4)); // Overflow horizontal scroll (or not)
                    if (item.property_flags & (1 << 4 )) curr_item->layout_flags = (curr_item->layout_flags & ~(1 << 5)) | (item.layout_flags & (1 << 5)); // Position absolute or not
                    if (item.property_flags & (1 << 5 )) curr_item->layout_flags = (curr_item->layout_flags & ~(1 << 6)) | (item.layout_flags & (1 << 6)); // Hide or not
                    if (item.property_flags & (1 << 6 )) curr_item->gap = item.gap;
                    if (item.property_flags & (1 << 7 )) curr_item->preferred_width = item.preferred_width;
                    if (item.property_flags & (1 << 8 )) curr_item->min_width = item.min_width;
                    if (item.property_flags & (1 << 9 )) curr_item->max_width = item.max_width;
                    if (item.property_flags & (1 << 10)) curr_item->preferred_height = item.preferred_height;
                    if (item.property_flags & (1 << 11)) curr_item->min_height = item.min_height;
                    if (item.property_flags & (1 << 12)) curr_item->max_height = item.max_height;
                    if (item.property_flags & (1 << 13)) curr_item->horizontal_alignment = item.horizontal_alignment;
                    if (item.property_flags & (1 << 14)) curr_item->vertical_alignment = item.vertical_alignment;
                    if (item.property_flags & (1 << 15)) curr_item->horizontal_text_alignment = item.horizontal_text_alignment;
                    if (item.property_flags & (1 << 16)) curr_item->vertical_text_alignment = item.vertical_text_alignment;
                    if (item.property_flags & (1 << 17)) curr_item->left = item.left;
                    if (item.property_flags & (1 << 18)) curr_item->right = item.right;
                    if (item.property_flags & (1 << 19)) curr_item->top = item.top;
                    if (item.property_flags & (1 << 20)) curr_item->bottom = item.bottom;
                    if (item.property_flags & (1 << 21)) memcpy(&curr_item->background_r, &item.background_r, 3); // Copy rgb
                    if (item.property_flags & (1 << 22)) curr_item->hide_background = item.hide_background; // Copy rgb
                    if (item.property_flags & (1 << 23)) memcpy(&curr_item->border_r, &item.border_r, 3); // Copy rgb
                    if (item.property_flags & (1 << 24)) memcpy(&curr_item->text_r, &item.text_r, 3); // Copy rgb
                    if (item.property_flags & (1 << 25)) curr_item->border_top = item.border_top; 
                    if (item.property_flags & (1 << 26)) curr_item->border_bottom = item.border_bottom; 
                    if (item.property_flags & (1 << 27)) curr_item->border_left = item.border_left; 
                    if (item.property_flags & (1 << 28)) curr_item->border_right = item.border_right; 
                    if (item.property_flags & (1 << 29)) curr_item->border_radius_tl = item.border_radius_tl; 
                    if (item.property_flags & (1 << 30)) curr_item->border_radius_tr = item.border_radius_tr; 
                    if (item.property_flags & (1 << 31)) curr_item->border_radius_bl = item.border_radius_bl; 
                    if (item.property_flags & ((uint64_t)1 << 32)) curr_item->border_radius_br = item.border_radius_br; 
                    if (item.property_flags & ((uint64_t)1 << 33)) curr_item->pad_top = item.pad_top; 
                    if (item.property_flags & ((uint64_t)1 << 34)) curr_item->pad_bottom = item.pad_bottom; 
                    if (item.property_flags & ((uint64_t)1 << 35)) curr_item->pad_left = item.pad_left; 
                    if (item.property_flags & ((uint64_t)1 << 36)) curr_item->pad_right = item.pad_right; 
                    curr_item->font_id = item.font_id;
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
                    void* found = Hashmap_Get(&ss->tag_item_hashmap, &tag);
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
                        new_item.property_flags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                        Hashmap_Set(&ss->tag_item_hashmap, &tag, &selector_indexes[selector_count]); // Store item index
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
                        void* found = Hashmap_Get(&ss->tag_pseudo_item_hashmap, &key);

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
                            new_item.property_flags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                            Hashmap_Set(&ss->tag_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index

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
                    void* found = Hashmap_Get(&ss->class_item_hashmap, &stored_class);
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
                        new_item.property_flags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                        Hashmap_Set(&ss->class_item_hashmap, &stored_class, &selector_indexes[selector_count]); // Store item index
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
                        void* found = Hashmap_Get(&ss->class_pseudo_item_hashmap, &key);
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
                            new_item.property_flags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                            Hashmap_Set(&ss->class_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index
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
                    void* found = Hashmap_Get(&ss->id_item_hashmap, &stored_id);
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
                        new_item.property_flags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                        Hashmap_Set(&ss->id_item_hashmap, &stored_id, &selector_indexes[selector_count]); // Store item index
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
                        void* found = Hashmap_Get(&ss->id_pseudo_item_hashmap, &key);
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
                            new_item.property_flags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (uint32_t)(ss->items.size - 1);
                            Hashmap_Set(&ss->id_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index
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
                        if (c == 'v') item.layout_flags |= LAYOUT_VERTICAL;
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

                    // Relative/Absolute positioning
                    case STYLE_POSITION_PROPERTY:
                        if (text_ref->char_count == 8 && memcmp(text, "absolute", 8) == 0) {
                            item.layout_flags |= POSITION_ABSOLUTE;
                            item.property_flags |= 1 << 4;
                        }
                        break;

                    // Hide/show
                    case STYLE_HIDE_PROPERTY:
                        if (text_ref->char_count == 4 && memcmp(text, "hide", 4) == 0) {
                            item.layout_flags |= HIDDEN;
                            item.property_flags |= 1 << 5;
                        }
                        break;
                    
                    // Set gap
                    case STYLE_GAP_PROPERTY:
                        if (String_To_Float(&item.gap, text)) 
                            item.property_flags |= 1 << 6;
                        break;

                    // Set preferred width
                    case STYLE_WIDTH_PROPERTY:
                        if (String_To_Float(&item.preferred_width, text))
                            item.property_flags |= 1 << 7;
                        break;

                    // Set min width
                    case STYLE_MIN_WIDTH_PROPERTY:
                        if (String_To_Float(&item.min_width, text))
                            item.property_flags |= 1 << 8;
                        break;
                    
                    // Set max width
                    case STYLE_MAX_WIDTH_PROPERTY:
                        if (String_To_Float(&item.max_width, text))
                            item.property_flags |= 1 << 9;
                        break;

                    // Set preferred height
                    case STYLE_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.preferred_height, text)) 
                            item.property_flags |= 1 << 10;
                        break;

                    // Set min height
                    case STYLE_MIN_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.min_height, text)) 
                            item.property_flags |= 1 << 11;
                        break;

                    // Set max height
                    case STYLE_MAX_HEIGHT_PROPERTY:
                        if (String_To_Float(&item.max_height, text)) 
                            item.property_flags |= 1 << 12;
                        break;

                    // Set horizontal alignment
                    case STYLE_ALIGN_H_PROPERTY:
                        if (text_ref->char_count == 4 && memcmp(text, "left", 4) == 0) {
                            item.horizontal_alignment = 0;
                            item.property_flags |= 1 << 13;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.horizontal_alignment = 1;
                            item.property_flags |= 1 << 13;
                        }
                        if (text_ref->char_count == 5 && memcmp(text, "right", 5) == 0) {
                            item.horizontal_alignment = 2;
                            item.property_flags |= 1 << 13;
                        }
                        break;

                    // Set vertical alignment
                    case STYLE_ALIGN_V_PROPERTY:
                        if (text_ref->char_count == 3 && memcmp(text, "top", 3) == 0) {
                            item.vertical_alignment = 0;
                            item.property_flags |= 1 << 14;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.vertical_alignment = 1;
                            item.property_flags |= 1 << 14;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "bottom", 6) == 0) {
                            item.vertical_alignment = 2;
                            item.property_flags |= 1 << 14;
                        }
                        break;

                    // Set horizontal text alignment
                    case STYLE_TEXT_ALIGN_H_PROPERTY:
                        if (text_ref->char_count == 4 && memcmp(text, "left", 4) == 0) {
                            item.horizontal_text_alignment = 0;
                            item.property_flags |= 1 << 15;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.horizontal_text_alignment = 1;
                            item.property_flags |= 1 << 15;
                        }
                        if (text_ref->char_count == 5 && memcmp(text, "right", 5) == 0) {
                            item.horizontal_text_alignment = 2;
                            item.property_flags |= 1 << 15;
                        }
                        break;

                    // Set vertical text alignment
                    case STYLE_TEXT_ALIGN_V_PROPERTY:
                        if (text_ref->char_count == 3 && memcmp(text, "top", 3) == 0) {
                            item.vertical_text_alignment = 0;
                            item.property_flags |= 1 << 16;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "center", 6) == 0) {
                            item.vertical_text_alignment = 1;
                            item.property_flags |= 1 << 16;
                        }
                        if (text_ref->char_count == 6 && memcmp(text, "bottom", 6) == 0) {
                            item.vertical_text_alignment = 2;
                            item.property_flags |= 1 << 16;
                        }
                        break;

                    // Set absolute position properties
                    case STYLE_LEFT_PROPERTY:
                        float abs_position; 
                        if (String_To_Float(&abs_position, text)) {
                            item.left = abs_position;
                            item.property_flags |= 1 << 17;
                        }
                        break;

                    case STYLE_RIGHT_PROPERTY:
                        if (String_To_Float(&abs_position, text)) {
                            item.right = abs_position;
                            item.property_flags |= 1 << 18;
                        }
                        break;

                    case STYLE_TOP_PROPERTY:
                        if (String_To_Float(&abs_position, text)) {
                            item.top = abs_position;
                            item.property_flags |= 1 << 19;
                        }
                        break;

                    case STYLE_BOTTOM_PROPERTY:
                        if (String_To_Float(&abs_position, text)) {
                            item.bottom = abs_position;
                            item.property_flags |= 1 << 20;
                        }
                        break;

                    // Set background colour
                    case STYLE_BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.background_r = rgb.r;
                            item.background_g = rgb.g;
                            item.background_b = rgb.b;
                            item.property_flags |= 1 << 21;
                        } else if (text_ref->char_count == 4 && memcmp(text, "none", 4) == 0) {
                            item.hide_background = true;
                            item.property_flags |= 1 << 22;
                        }
                        break;

                    // Set border colour
                    case STYLE_BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.border_r = rgb.r;
                            item.border_g = rgb.g;
                            item.border_b = rgb.b;
                            item.property_flags |= 1 << 23;
                        }
                        break;

                    // Set text colour
                    case STYLE_TEXT_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.text_r = rgb.r;
                            item.text_g = rgb.g;
                            item.text_b = rgb.b;
                            item.property_flags |= 1 << 24;
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
                            item.property_flags |= 1 << 25;
                            item.property_flags |= 1 << 26;
                            item.property_flags |= 1 << 27;
                            item.property_flags |= 1 << 28;
                        }
                        break;
                    case STYLE_BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_top, text)) {
                            item.property_flags |= 1 << 25;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_bottom, text)) {
                            item.property_flags |= 1 << 26;
                        }
                        break;
                    case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_left, text)) {
                            item.property_flags |= 1 << 27;
                        }
                        break;
                    case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&item.border_right, text)) {
                            item.property_flags |= 1 << 28;
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
                            item.property_flags |= 1 << 29;
                            item.property_flags |= 1 << 30;
                            item.property_flags |= 1 << 31;
                            item.property_flags |= (uint64_t)1 << 32;
                        }
                        break;
                    case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_tl, text)) {
                            item.property_flags |= 1 << 29;
                        }
                        break;
                    case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_tr, text)) {
                            item.property_flags |= 1 << 30;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_bl, text)) {
                            item.property_flags |= 1 << 31;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&item.border_radius_br, text)) {
                            item.property_flags |= (uint64_t)1 << 32;
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
                            item.property_flags |= (uint64_t)1 << 33;
                            item.property_flags |= (uint64_t)1 << 34;
                            item.property_flags |= (uint64_t)1 << 35;
                            item.property_flags |= (uint64_t)1 << 36;
                        }
                        break;
                    case STYLE_PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&item.pad_top, text)) {
                            item.property_flags |= (uint64_t)1 << 33;
                        }
                        break;
                    case STYLE_PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&item.pad_bottom, text)) {
                            item.property_flags |= (uint64_t)1 << 34;
                        }
                        break;
                    case STYLE_PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&item.pad_left, text)) {
                            item.property_flags |= (uint64_t)1 << 35;
                        }
                        break;
                    case STYLE_PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&item.pad_right, text)) {
                            item.property_flags |= (uint64_t)1 << 36;
                        }
                        break;

                    case STYLE_FONT_PROPERTY:
                        void* found_font = String_Map_Get(&ss->font_name_index_map, text);
                        if (found_font != NULL) {
                            item.font_id = *(uint8_t*)found_font;
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

int NU_Internal_Stylesheet_Create(NU_Stylesheet* stylesheet, char* css_filepath)
{
    NU_Stylesheet_Init(stylesheet);

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
    Vector_Reserve(&tokens, sizeof(enum NU_Style_Token), 50000); // reserve ~200KB

    struct Vector text_ref_vector;
    Vector_Reserve(&text_ref_vector, sizeof(struct Style_Text_Ref), 4000);


    NU_Style_Tokenise(src_buffer, src_length, &tokens, &text_ref_vector);
    if (NU_Stylesheet_Parse(src_buffer, src_length, &tokens, stylesheet, &text_ref_vector) == -1)
    {
        printf("CSS parsing failed!");
    }

    Vector_Free(&tokens);
    return 1;
}




// -------------------------------------------------
// --- STYLESHEET APPLICATION TO NODES =============
// -------------------------------------------------
static void NU_Stylesheet_Find_Match(struct Node* node, NU_Stylesheet* ss, int* match_index_list)
{
    int count = 0;

    // Tag match first (lowest priority)
    void* tag_found = Hashmap_Get(&ss->tag_item_hashmap, &node->tag); 
    if (tag_found != NULL) { 
        match_index_list[count++] = (int)*(uint32_t*)tag_found;
    }

    // Class match second
    if (node->class != NULL) { 
        char* stored_class = String_Set_Get(&ss->class_string_set, node->class);
        if (stored_class != NULL) {
            void* class_found = Hashmap_Get(&ss->class_item_hashmap, &stored_class);
            if (class_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)class_found;
            }
        }
    }

    // ID match last (highest priority)
    if (node->id != NULL) { 
        char* stored_id = String_Set_Get(&ss->id_string_set, node->id);
        if (stored_id != NULL) {
            void* id_found = Hashmap_Get(&ss->id_item_hashmap, &stored_id);
            if (id_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)id_found;
            }
        }
    }
}

static void NU_Apply_Style_Item_To_Node(struct Node* node, NU_Stylesheet_Item* item)
{
    if (item->property_flags & (1 << 0) && !(node->inline_style_flags & (1 << 0))) {
        node->layout_flags = (node->layout_flags & ~(1 << 0)) | (item->layout_flags & (1 << 0)); // Layout direction
    }
    if (item->property_flags & (1 << 1) && !(node->inline_style_flags & (1 << 1))) {
        node->layout_flags = (node->layout_flags & ~(1 << 1)) | (item->layout_flags & (1 << 1)); // Grow horizontal
        node->layout_flags = (node->layout_flags & ~(1 << 2)) | (item->layout_flags & (1 << 2)); // Grow vertical
    }
    if (item->property_flags & (1 << 2) && !(node->inline_style_flags & (1 << 2))) {
        node->layout_flags = (node->layout_flags & ~(1 << 3)) | (item->layout_flags & (1 << 3)); // Overflow vertical scroll (or not)
    }
    if (item->property_flags & (1 << 3) && !(node->inline_style_flags & (1 << 3))) {
        node->layout_flags = (node->layout_flags & ~(1 << 4)) | (item->layout_flags & (1 << 4)); // Overflow horizontal scroll (or not)
    }
    if (item->property_flags & (1 << 4) && !(node->inline_style_flags & (1 << 4))) {
        node->layout_flags = (node->layout_flags & ~(1 << 5)) | (item->layout_flags & (1 << 5)); // Absolute positioning (or not)
    }
    if (item->property_flags & (1 << 5) && !(node->inline_style_flags & (1 << 5))) {
        node->layout_flags = (node->layout_flags & ~(1 << 6)) | (item->layout_flags & (1 << 6)); // Hidden or not
    }
    if (item->property_flags & (1 << 6) && !(node->inline_style_flags & (1 << 6))) {
        node->gap = item->gap;
    }
    if (item->property_flags & (1 << 7) && !(node->inline_style_flags & (1 << 7))) {
        node->preferred_width = item->preferred_width;
    }
    if (item->property_flags & (1 << 8) && !(node->inline_style_flags & (1 << 8))) {
        node->min_width = item->min_width;
    }
    if (item->property_flags & (1 << 9) && !(node->inline_style_flags & (1 << 9))) {
        node->max_width = item->max_width;
    }
    if (item->property_flags & (1 << 10) && !(node->inline_style_flags & (1 << 10))) {
        node->preferred_height = item->preferred_height;
    }
    if (item->property_flags & (1 << 11) && !(node->inline_style_flags & (1 << 11))) {
        node->min_height = item->min_height;
    }
    if (item->property_flags & (1 << 12) && !(node->inline_style_flags & (1 << 12))) {
        node->max_height = item->max_height;
    }
    if (item->property_flags & (1 << 13) && !(node->inline_style_flags & (1 << 13))) {
        node->horizontal_alignment = item->horizontal_alignment;
    }
    if (item->property_flags & (1 << 14) && !(node->inline_style_flags & (1 << 14))) {
        node->vertical_alignment = item->vertical_alignment;
    }
    if (item->property_flags & (1 << 15) && !(node->inline_style_flags & (1 << 15))) {
        node->horizontal_text_alignment = item->horizontal_text_alignment;
    }
    if (item->property_flags & (1 << 16) && !(node->inline_style_flags & (1 << 16))) {
        node->vertical_text_alignment = item->vertical_text_alignment;
    }
    if (item->property_flags & (1 << 17) && !(node->inline_style_flags & (1 << 17))) {
        node->left = item->left;
    }
    if (item->property_flags & (1 << 18) && !(node->inline_style_flags & (1 << 18))) {
        node->right = item->right;
    }
    if (item->property_flags & (1 << 19) && !(node->inline_style_flags & (1 << 19))) {
        node->top = item->top;
    }
    if (item->property_flags & (1 << 20) && !(node->inline_style_flags & (1 << 20))) {
        node->bottom = item->bottom;
    }
    if (item->property_flags & (1 << 21) && !(node->inline_style_flags & (1 << 21))) {
        node->background_r = item->background_r;
        node->background_g = item->background_g;
        node->background_b = item->background_b;
    }
    if (item->property_flags & (1 << 22) && !(node->inline_style_flags & (1 << 22))) {
        node->hide_background = item->hide_background;
    }
    if (item->property_flags & (1 << 23) && !(node->inline_style_flags & (1 << 23))) {
        node->border_r = item->border_r;
        node->border_g = item->border_g;
        node->border_b = item->border_b;
    }
    if (item->property_flags & (1 << 24) && !(node->inline_style_flags & (1 << 24))) {
        node->text_r = item->text_r;
        node->text_g = item->text_g;
        node->text_b = item->text_b;
    }
    if (item->property_flags & (1 << 25) && !(node->inline_style_flags & (1 << 25))) {
        node->border_top = item->border_top;
    }
    if (item->property_flags & (1 << 26) && !(node->inline_style_flags & (1 << 26))) {
        node->border_bottom = item->border_bottom;
    }
    if (item->property_flags & (1 << 27) && !(node->inline_style_flags & (1 << 27))) {
        node->border_left = item->border_left;
    }
    if (item->property_flags & (1 << 28) && !(node->inline_style_flags & (1 << 28))) {
        node->border_right = item->border_right;
    }
    if (item->property_flags & (1 << 29) && !(node->inline_style_flags & (1 << 29))) {
        node->border_radius_tl = item->border_radius_tl;
    }
    if (item->property_flags & (1 << 30) && !(node->inline_style_flags & (1 << 30))) {
        node->border_radius_tr = item->border_radius_tr;
    }
    if (item->property_flags & (1 << 31) && !(node->inline_style_flags & (1 << 31))) {
        node->border_radius_bl = item->border_radius_bl;
    }
    if (item->property_flags & ((uint64_t)1 << 32) && !(node->inline_style_flags & ((uint64_t)1 << 32))) {
        node->border_radius_br = item->border_radius_br;
    }
    if (item->property_flags & ((uint64_t)1 << 33) && !(node->inline_style_flags & ((uint64_t)1 << 33))) {
        node->pad_top = item->pad_top;
    }
    if (item->property_flags & ((uint64_t)1 << 34) && !(node->inline_style_flags & ((uint64_t)1 << 34))) {
        node->pad_bottom = item->pad_bottom;
    }
    if (item->property_flags & ((uint64_t)1 << 35) && !(node->inline_style_flags & ((uint64_t)1 << 35))) {
        node->pad_left = item->pad_left;
    }
    if (item->property_flags & ((uint64_t)1 << 36) && !(node->inline_style_flags & ((uint64_t)1 << 36))) {
        node->pad_right = item->pad_right;
    }
    node->font_id = item->font_id; // set font 
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

void NU_Apply_Pseudo_Style_To_Node(struct Node* node, NU_Stylesheet* ss, enum NU_Pseudo_Class pseudo)
{
    // Tag pseudo style match and apply
    struct NU_Stylesheet_Tag_Pseudo_Pair key = { node->tag, pseudo };
    void* tag_pseudo_found = Hashmap_Get(&ss->tag_pseudo_item_hashmap, &key);
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
            void* class_pseudo_found = Hashmap_Get(&ss->class_pseudo_item_hashmap, &key);
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
            void* id_pseudo_found = Hashmap_Get(&ss->id_pseudo_item_hashmap, &key);
            if (id_pseudo_found != NULL) {
                uint32_t index = *(uint32_t*)id_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }
}

void NU_Internal_Stylesheet_Apply(NU_Stylesheet* stylesheet)
{
    struct Node* root_window = NU_Tree_Get(&__nu_global_gui.tree, 0, 0);
    NU_Apply_Stylesheet_To_Node(root_window, stylesheet);

    // For each layer
    for (uint16_t l=0; l<=__nu_global_gui.deepest_layer; l++)
    {
        NU_Layer* layer = &__nu_global_gui.tree.layers[l];

        // Iterate over layer
        for (uint32_t i=0; i<layer->size; i++)
        {
            struct Node* node = NU_Layer_Get(layer, i);
            if (!node->node_present) continue;

            NU_Apply_Stylesheet_To_Node(node, stylesheet);
        }
    }

    __nu_global_gui.stylesheet = stylesheet;
}
