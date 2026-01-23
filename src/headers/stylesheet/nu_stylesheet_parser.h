#pragma once
#include "nu_stylesheet_grammar_assertions.h"
#include <datastructures/linear_stringmap.h>
#include <datastructures/linear_stringset.h>

static int NU_Stylesheet_Parse(char* src, struct Vector* tokens, NU_Stylesheet* ss, struct Vector* text_refs)
{
    uint32_t text_index = 0;
    struct Style_Text_Ref* text_ref;

    NU_Stylesheet_Item item;
    item.propertyFlags = 0;
    item.fontId = 0;

    // ---------------
    // (string -> int)
    // ---------------
    LinearStringmap imageFilepathToHandleMap;
    LinearStringmap fontNameIndexMap;
    LinearStringmapInit(&imageFilepathToHandleMap, sizeof(GLuint), 20, 512);
    LinearStringmapInit(&fontNameIndexMap, sizeof(GLuint), 12, 128);

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
                create_font_name = &src[text_ref->src_index];
                src[text_ref->src_index + text_ref->char_count] = '\0';
                text_index += 1;

                // Create a new font
                void* found_font = LinearStringmapGet(&fontNameIndexMap, create_font_name);
                if (found_font == NULL) {
                    NU_Font font;
                    Vector_Push(&ss->fonts, &font);
                    create_font_index = (uint8_t)(ss->fonts.size - 1);
                    create_font = Vector_Get(&ss->fonts, create_font_index);
                    LinearStringmapSet(&fontNameIndexMap, create_font_name, &create_font_index);
                } 

                ctx = 1;
                i += 2;
                continue;
            }
            else {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
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
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
            }
        }

        if (token == STYLE_SELECTOR_CLOSE_BRACE)
        {
            if (!AssertSelectionClosingBraceGrammar(tokens, i)) {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
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
                    if (item.propertyFlags & (1ULL << 38)) curr_item->glImageHandle = item.inputType;
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
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&fontNameIndexMap);
                    return 0;
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
                        LinearStringmapFree(&imageFilepathToHandleMap);
                        LinearStringmapFree(&fontNameIndexMap);
                        return 0;
                    }

                    i += 3;
                    selector_count++;
                    continue;
                }
                else
                {
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&fontNameIndexMap);
                    return 0;
                }
            }
            else
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
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
                    char* src_class = &src[text_ref->src_index];
                    src[text_ref->src_index + text_ref->char_count] = '\0';

                    // Add class to set
                    char* stored_class = LinearStringsetAdd(&ss->class_string_set, src_class);

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
                        char* src_class = &src[text_ref->src_index];
                        src[text_ref->src_index + text_ref->char_count] = '\0';

                        // Add class to set
                        char* stored_class = LinearStringsetAdd(&ss->class_string_set, src_class);
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
                        LinearStringmapFree(&imageFilepathToHandleMap);
                        LinearStringmapFree(&fontNameIndexMap);
                        return 0;
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else
                {
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&fontNameIndexMap);
                    return 0;
                }
            }
            else 
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
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
                    char* src_id = &src[text_ref->src_index];
                    src[text_ref->src_index + text_ref->char_count] = '\0';

                    // Add id to set
                    char* stored_id = LinearStringsetAdd(&ss->id_string_set, src_id);

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
                        char* src_id = &src[text_ref->src_index];
                        src[text_ref->src_index + text_ref->char_count] = '\0';

                        // Add id to set
                        char* stored_id = LinearStringsetAdd(&ss->id_string_set, src_id);
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
                        LinearStringmapFree(&imageFilepathToHandleMap);
                        LinearStringmapFree(&fontNameIndexMap);
                        return 0;
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else
                {
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&fontNameIndexMap);
                    return 0;
                }
            }
            else 
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
            }
        }

        if (token == STYLE_SELECTOR_COMMA)
        {
            if (AssertSelectorCommaGrammar(tokens, i)) {
                if (selector_count == 64) {
                    printf("%s", "[Generate Stylesheet] Error! Too many selectors in one list! max = 64");
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&fontNameIndexMap);
                    return 0;
                }
                i += 1;
                continue;
            }
            else {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
            }
        }

        if (NU_Is_Property_Identifier_Token(token))
        {
            if (!AssertPropertyIdentifierGrammar(tokens, i)) {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&fontNameIndexMap);
                return 0;
            }

            // get property value text
            text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
            char c = src[text_ref->src_index];
            char* text = &src[text_ref->src_index];
            src[text_ref->src_index + text_ref->char_count] = '\0';
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
                        if (strcmp(text, "absolute") == 0) {
                            item.layoutFlags |= POSITION_ABSOLUTE;
                            item.propertyFlags |= 1ULL << 4;
                        }
                        break;

                    // Hide/show
                    case STYLE_HIDE_PROPERTY:
                        if (strcmp(text, "hide") == 0) {
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
                        if (strcmp(text, "left") == 0) {
                            item.horizontalAlignment = 0;
                            item.propertyFlags |= 1ULL << 13;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.horizontalAlignment = 1;
                            item.propertyFlags |= 1ULL << 13;
                        }
                        if (strcmp(text, "right") == 0) {
                            item.horizontalAlignment = 2;
                            item.propertyFlags |= 1ULL << 13;
                        }
                        break;

                    // Set vertical alignment
                    case STYLE_ALIGN_V_PROPERTY:
                        if (strcmp(text, "top") == 0) {
                            item.verticalAlignment = 0;
                            item.propertyFlags |= 1ULL << 14;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.verticalAlignment = 1;
                            item.propertyFlags |= 1ULL << 14;
                        }
                        if (strcmp(text, "bottom") == 0) {
                            item.verticalAlignment = 2;
                            item.propertyFlags |= 1ULL << 14;
                        }
                        break;

                    // Set horizontal text alignment
                    case STYLE_TEXT_ALIGN_H_PROPERTY:
                        if (strcmp(text, "left") == 0) {
                            item.horizontalTextAlignment = 0;
                            item.propertyFlags |= 1ULL << 15;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.horizontalTextAlignment = 1;
                            item.propertyFlags |= 1ULL << 15;
                        }
                        if (strcmp(text, "right") == 0) {
                            item.horizontalTextAlignment = 2;
                            item.propertyFlags |= 1ULL << 15;
                        }
                        break;

                    // Set vertical text alignment
                    case STYLE_TEXT_ALIGN_V_PROPERTY:
                        if (strcmp(text, "top") == 0) {
                            item.verticalTextAlignment = 0;
                            item.propertyFlags |= 1ULL << 16;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.verticalTextAlignment = 1;
                            item.propertyFlags |= 1ULL << 16;
                        }
                        if (strcmp(text, "bottom") == 0) {
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
                        } else if (strcmp(text, "none") == 0) {
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
                        if (item.tag == NU_INPUT) break;
                        void* found = LinearStringmapGet(&imageFilepathToHandleMap, text);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(text);
                            if (image_handle) {
                                item.glImageHandle = image_handle;
                                LinearStringmapSet(&imageFilepathToHandleMap, text, &image_handle);
                                item.propertyFlags |= 1ULL << 37;
                            }
                        } 
                        else {
                            item.propertyFlags |= 1ULL << 37;
                            item.glImageHandle = *(GLuint*)found;
                        }
                        break;

                    case STYLE_FONT_PROPERTY:
                        void* found_font = LinearStringmapGet(&fontNameIndexMap, text);
                        if (found_font != NULL) {
                            item.fontId = *(uint8_t*)found_font;
                        }
                        break;

                    case STYLE_INPUT_TYPE_PROPERTY:
                        if (item.tag != NU_INPUT) break;
                        if (strcmp(text, "number") == 0) {
                            item.inputType = 1;
                        } else {
                            item.inputType = 0;
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
        LinearStringmapFree(&imageFilepathToHandleMap);
        LinearStringmapFree(&fontNameIndexMap);
        return 0;
    }

    LinearStringmapFree(&imageFilepathToHandleMap);
    LinearStringmapFree(&fontNameIndexMap);
    return 1;
}
