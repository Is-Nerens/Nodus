#pragma once
#include "nu_stylesheet_grammar_assertions.h"
#include <datastructures/linear_stringmap.h>
#include <datastructures/linear_stringset.h>
#include "../nu_token_array.h"

void NU_Stylesheet_Overwrite_Style_Item(NU_Stylesheet_Item* item, NU_Stylesheet_Item* overwriter)
{
    item->propertyFlags |= overwriter->propertyFlags;
    if (overwriter->propertyFlags & PROPERTY_FLAG_LAYOUT_VERTICAL  ) item->layoutFlags = (item->layoutFlags & ~LAYOUT_VERTICAL) | (overwriter->layoutFlags & LAYOUT_VERTICAL); // Layout direction
    if (overwriter->propertyFlags & PROPERTY_FLAG_GROW             ) item->layoutFlags = (item->layoutFlags & ~(GROW_HORIZONTAL | GROW_VERTICAL)) | (overwriter->layoutFlags & (GROW_HORIZONTAL | GROW_VERTICAL)); // Grow
    if (overwriter->propertyFlags & PROPERTY_FLAG_VERTICAL_SCROLL  ) item->layoutFlags = (item->layoutFlags & ~OVERFLOW_VERTICAL_SCROLL) | (overwriter->layoutFlags & OVERFLOW_VERTICAL_SCROLL); // Overflow vertical scroll (or not)
    if (overwriter->propertyFlags & PROPERTY_FLAG_HORIZONTAL_SCROLL) item->layoutFlags = (item->layoutFlags & ~OVERFLOW_HORIZONTAL_SCROLL) | (overwriter->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL); // Overflow horizontal scroll (or not)
    if (overwriter->propertyFlags & PROPERTY_FLAG_POSITION_ABSOLUTE) item->layoutFlags = (item->layoutFlags & ~POSITION_ABSOLUTE) | (overwriter->layoutFlags & POSITION_ABSOLUTE); // Position absolute or not
    if (overwriter->propertyFlags & PROPERTY_FLAG_HIDDEN           ) item->layoutFlags = (item->layoutFlags & ~HIDDEN) | (overwriter->layoutFlags & HIDDEN); // Hide or not
    if (overwriter->propertyFlags & PROPERTY_FLAG_IGNORE_MOUSE     ) item->layoutFlags = (item->layoutFlags & ~IGNORE_MOUSE) | (overwriter->layoutFlags & IGNORE_MOUSE); // Ignore mouse or not
    if (overwriter->propertyFlags & PROPERTY_FLAG_GAP              ) item->gap = overwriter->gap;
    if (overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_WIDTH  ) item->preferred_width = overwriter->preferred_width;
    if (overwriter->propertyFlags & PROPERTY_FLAG_MIN_WIDTH        ) item->minWidth = overwriter->minWidth;
    if (overwriter->propertyFlags & PROPERTY_FLAG_MAX_WIDTH        ) item->maxWidth = overwriter->maxWidth;
    if (overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_HEIGHT ) item->preferred_height = overwriter->preferred_height;
    if (overwriter->propertyFlags & PROPERTY_FLAG_MIN_HEIGHT       ) item->minHeight = overwriter->minHeight;
    if (overwriter->propertyFlags & PROPERTY_FLAG_MAX_HEIGHT       ) item->maxHeight = overwriter->maxHeight;
    if (overwriter->propertyFlags & PROPERTY_FLAG_ALIGN_H          ) item->horizontalAlignment = overwriter->horizontalAlignment;
    if (overwriter->propertyFlags & PROPERTY_FLAG_ALIGN_V          ) item->verticalAlignment = overwriter->verticalAlignment;
    if (overwriter->propertyFlags & PROPERTY_FLAG_TEXT_ALIGN_H     ) item->horizontalTextAlignment = overwriter->horizontalTextAlignment;
    if (overwriter->propertyFlags & PROPERTY_FLAG_TEXT_ALIGN_V     ) item->verticalTextAlignment = overwriter->verticalTextAlignment;
    if (overwriter->propertyFlags & PROPERTY_FLAG_LEFT             ) item->left = overwriter->left;
    if (overwriter->propertyFlags & PROPERTY_FLAG_RIGHT            ) item->right = overwriter->right;
    if (overwriter->propertyFlags & PROPERTY_FLAG_TOP              ) item->top = overwriter->top;
    if (overwriter->propertyFlags & PROPERTY_FLAG_BOTTOM           ) item->bottom = overwriter->bottom;
    if (overwriter->propertyFlags & PROPERTY_FLAG_BACKGROUND       ) memcpy(&item->backgroundR, &overwriter->backgroundR, 3); // Copy rgb
    if (overwriter->propertyFlags & PROPERTY_FLAG_HIDE_BACKGROUND  ) item->layoutFlags = (item->layoutFlags & ~HIDE_BACKGROUND) | (overwriter->layoutFlags & HIDE_BACKGROUND); // Hide background or not
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_COLOUR    ) memcpy(&item->borderR, &overwriter->borderR, 3); // Copy rgb
    if (overwriter->propertyFlags & PROPERTY_FLAG_TEXT_COLOUR      ) memcpy(&item->textR, &overwriter->textR, 3); // Copy rgb
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_TOP       ) item->borderTop = overwriter->borderTop; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_BOTTOM    ) item->borderBottom = overwriter->borderBottom; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_LEFT      ) item->borderLeft = overwriter->borderLeft; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RIGHT     ) item->borderRight = overwriter->borderRight; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_TL ) item->borderRadiusTl = overwriter->borderRadiusTl; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_TR ) item->borderRadiusTr = overwriter->borderRadiusTr; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_BL ) item->borderRadiusBl = overwriter->borderRadiusBl; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_BR ) item->borderRadiusBr = overwriter->borderRadiusBr; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_PAD_TOP          ) item->padTop = overwriter->padTop; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_PAD_BOTTOM       ) item->padBottom = overwriter->padBottom; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_PAD_LEFT         ) item->padLeft = overwriter->padLeft; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_PAD_RIGHT        ) item->padRight = overwriter->padRight; 
    if (overwriter->propertyFlags & PROPERTY_FLAG_IMAGE            ) item->glImageHandle = overwriter->glImageHandle;
    if (overwriter->propertyFlags & PROPERTY_FLAG_INPUT_TYPE       ) item->inputType = overwriter->inputType;
    item->fontId = overwriter->fontId;
}

static int NU_Stylesheet_Parse(char* src, TokenArray* tokens, NU_Stylesheet* ss, struct Vector* text_refs)
{
    u32 text_index = 0;
    struct Style_Text_Ref* text_ref;

    NU_Stylesheet_Item item;
    item.propertyFlags = 0;
    item.fontId = 0;

    // ---------------
    // (string -> int)
    // ---------------
    LinearStringmap imageFilepathToHandleMap;
    LinearStringmapInit(&imageFilepathToHandleMap, sizeof(GLuint), 20, 512);

    u32 selector_indexes[64];
    int selector_count = 0;

    int ctx = 0; // 0 == standard selector; 1 == font creation selector; 2 == default selector
    u8 createFontID = UINT8_MAX;
    NU_Font* createFont;
    int createFontSize = 18;
    int createFontWeight = 400;
    char* createFontName = NULL;
    char* createFontSrc = NULL;

    int i = 0;
    while(i < tokens->size)
    {
        const enum NU_Style_Token token = TokenArray_Get(tokens, i);

        if (token == STYLE_FONT_CREATION_SELECTOR)
        {
            if (AssertFontCreationSelectorGrammar(tokens, i)) {

                enum NU_Style_Token font_name_token = TokenArray_Get(tokens, i+1);

                // Get id string
                text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
                createFontName = &src[text_ref->src_index];
                src[text_ref->src_index + text_ref->char_count] = '\0';
                text_index += 1;

                // Create a new font
                void* found_font = LinearStringmapGet(&ss->fontNameIndexMap, createFontName);
                if (found_font == NULL) {
                    NU_Font font;
                    createFontID = Container_Add(&ss->fonts, &font);
                    createFont = Container_Get(&ss->fonts, createFontID);
                    LinearStringmapSet(&ss->fontNameIndexMap, createFontName, &createFontID);
                } 

                ctx = 1;
                i += 2;
                continue;
            }
            else {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
        }

        else if (token == STYLE_DEFAULT_SELECTOR)
        {
            if (i < tokens->size - 1 && TokenArray_Get(tokens, i+1) == STYLE_SELECTOR_OPEN_BRACE) {
                ctx = 2;
                i += 1;
                continue;
            }
            else {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
        }

        else if (token == STYLE_SELECTOR_OPEN_BRACE)
        {
            if (AssertSelectionOpeningBraceGrammar(tokens, i)) {
                item.propertyFlags = 0;
                item.layoutFlags = 0;
                item.fontId = ss->defaultStyleItem.fontId;
                i += 1;
                continue;
            } 
            else {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
        }

        else if (token == STYLE_SELECTOR_CLOSE_BRACE)
        {
            if (!AssertSelectionClosingBraceGrammar(tokens, i)) {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
                
            if (ctx == 0)
            {   
                for (int i=0; i<selector_count; i++) {
                    u32 item_index = selector_indexes[i];
                    NU_Stylesheet_Item* curr_item = Vector_Get(&ss->items, item_index);

                    // Update current item
                    NU_Stylesheet_Overwrite_Style_Item(curr_item, &item);
                }
                selector_count = 0;
            }
            else if (ctx == 1)
            {
                if (createFontSrc != NULL)
                {
                    NU_Font_Create(createFont, createFontSrc, createFontSize, true);
                    createFontID = UINT8_MAX;
                    createFontSize = 18;
                    createFontWeight = 400;
                    createFontName = NULL;
                    createFontSrc = NULL;
                }
                else 
                {
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&ss->fontNameIndexMap);
                    return 0;
                }
            }
            else // ctx == 2
            {
                NU_Stylesheet_Overwrite_Style_Item(&ss->defaultStyleItem, &item);
            }

            ctx = 0;
            i += 1;
            continue;
        }

        else if (NU_Is_Tag_Selector_Token(token))
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = TokenArray_Get(tokens, i+1);
                if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)    
                {
                    int tag = NU_Token_To_Tag(token);
                    void* found = HashmapGet(&ss->tag_item_hashmap, &tag);
                    if (found != NULL) // Style item exists
                    {
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                        selector_indexes[selector_count] = *(u32*)found;
                    } 
                    else // Style item does not exist -> add one
                    { 
                        NU_Stylesheet_Item new_item;
                        new_item.class = NULL;
                        new_item.id = NULL;
                        new_item.tag = tag;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (u32)(ss->items.size - 1);
                        HashmapSet(&ss->tag_item_hashmap, &tag, &selector_indexes[selector_count]); // Store item index
                    }

                    i += 1;
                    selector_count++;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size - 3) 
                {
                    enum NU_Style_Token following_token = TokenArray_Get(tokens, i+2);
                    enum NU_Style_Token third_token = TokenArray_Get(tokens, i+3);
                    if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) 
                    {
                        int tag = NU_Token_To_Tag(token);
                        enum NU_Pseudo_Class pseudo_class = Token_To_Pseudo_Class(following_token);
                        struct NU_Stylesheet_Tag_Pseudo_Pair key = { tag, pseudo_class };
                        void* found = HashmapGet(&ss->tag_pseudo_item_hashmap, &key);

                        if (found != NULL) // Style item exists
                        {
                            NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                            selector_indexes[selector_count] = *(u32*)found;
                        }
                        else
                        {
                            NU_Stylesheet_Item new_item;
                            new_item.class = NULL;
                            new_item.id = NULL;
                            new_item.tag = tag;
                            new_item.propertyFlags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (u32)(ss->items.size - 1);
                            HashmapSet(&ss->tag_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index

                        }
                    }
                    else 
                    {
                        LinearStringmapFree(&imageFilepathToHandleMap);
                        LinearStringmapFree(&ss->fontNameIndexMap);
                        return 0;
                    }

                    i += 3;
                    selector_count++;
                    continue;
                }
                else
                {
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&ss->fontNameIndexMap);
                    return 0;
                }
            }
            else
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
        }

        else if (token == STYLE_CLASS_SELECTOR)
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = TokenArray_Get(tokens, i+1);
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
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                        selector_indexes[selector_count] = *(u32*)found;
                    } 
                    else // does not exist -> add item
                    { 
                        NU_Stylesheet_Item new_item;
                        new_item.class = stored_class;
                        new_item.id = NULL;
                        new_item.tag = -1;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (u32)(ss->items.size - 1);
                        HashmapSet(&ss->class_item_hashmap, &stored_class, &selector_indexes[selector_count]); // Store item index
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size-3) 
                {
                    enum NU_Style_Token following_token = TokenArray_Get(tokens, i+2);
                    enum NU_Style_Token third_token = TokenArray_Get(tokens, i+3);
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
                            NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                            selector_indexes[selector_count] = *(u32*)found;
                        }
                        else // does not exist -> add item
                        {
                            NU_Stylesheet_Item new_item;
                            new_item.class = stored_class;
                            new_item.id = NULL;
                            new_item.tag = -1;
                            new_item.propertyFlags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (u32)(ss->items.size - 1);
                            HashmapSet(&ss->class_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index
                        }
                    }
                    else 
                    {
                        LinearStringmapFree(&imageFilepathToHandleMap);
                        LinearStringmapFree(&ss->fontNameIndexMap);
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
                    LinearStringmapFree(&ss->fontNameIndexMap);
                    return 0;
                }
            }
            else 
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
        }

        else if (token == STYLE_ID_SELECTOR)
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = TokenArray_Get(tokens, i+1);
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
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                        selector_indexes[selector_count] = *(u32*)found;
                    }
                    else // does not exist -> add item
                    {
                        NU_Stylesheet_Item new_item;
                        new_item.class = NULL;
                        new_item.id = stored_id;
                        new_item.tag = -1;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selector_indexes[selector_count] = (u32)(ss->items.size - 1);
                        HashmapSet(&ss->id_item_hashmap, &stored_id, &selector_indexes[selector_count]); // Store item index
                    }

                    i += 1;
                    selector_count++;
                    text_index += 1;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size-3) 
                {
                    enum NU_Style_Token following_token = TokenArray_Get(tokens, i+2);
                    enum NU_Style_Token third_token = TokenArray_Get(tokens, i+3);
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
                            NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                            selector_indexes[selector_count] = *(u32*)found;
                        }
                        else // does not exist -> add item
                        {
                            NU_Stylesheet_Item new_item;
                            new_item.class = NULL;
                            new_item.id = stored_id;
                            new_item.tag = -1;
                            new_item.propertyFlags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selector_indexes[selector_count] = (u32)(ss->items.size - 1);
                            HashmapSet(&ss->id_pseudo_item_hashmap, &key, &selector_indexes[selector_count]); // Store item index
                        }
                    }
                    else 
                    {
                        LinearStringmapFree(&imageFilepathToHandleMap);
                        LinearStringmapFree(&ss->fontNameIndexMap);
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
                    LinearStringmapFree(&ss->fontNameIndexMap);
                    return 0;
                }
            }
            else 
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
        }

        else if (token == STYLE_SELECTOR_COMMA)
        {
            if (AssertSelectorCommaGrammar(tokens, i)) {
                if (selector_count == 64) {
                    printf("%s", "[Generate Stylesheet] Error! Too many selectors in one list! max = 64");
                    LinearStringmapFree(&imageFilepathToHandleMap);
                    LinearStringmapFree(&ss->fontNameIndexMap);
                    return 0;
                }
                i += 1;
                continue;
            }
            else {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }
        }

        else if (NU_Is_Property_Identifier_Token(token))
        {
            if (!AssertPropertyIdentifierGrammar(tokens, i)) {
                LinearStringmapFree(&imageFilepathToHandleMap);
                LinearStringmapFree(&ss->fontNameIndexMap);
                return 0;
            }

            // get property value text
            text_ref = (struct Style_Text_Ref*)Vector_Get(text_refs, text_index);
            char c = src[text_ref->src_index];
            char* text = &src[text_ref->src_index];
            src[text_ref->src_index + text_ref->char_count] = '\0';
            text_index += 1;
            
            if (ctx == 0 || ctx == 2)
            {
                switch (token)
                {
                    // Set layout direction
                    case STYLE_LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'v') {
                            item.layoutFlags |= LAYOUT_VERTICAL;
                            item.propertyFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
                        }
                        else if (c == 'h') { 
                            item.propertyFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
                        }
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
                        item.propertyFlags |= PROPERTY_FLAG_GROW;
                        break;
                    
                    // Set overflow behaviour
                    case STYLE_OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            item.layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                            item.propertyFlags |= PROPERTY_FLAG_VERTICAL_SCROLL;
                        }
                        else if (c == 'h') {
                            item.propertyFlags |= PROPERTY_FLAG_VERTICAL_SCROLL;
                        }
                        break;
                    
                    case STYLE_OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            item.layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                            item.propertyFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL;
                        } 
                        else if (c == 'h') {
                            item.propertyFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL;
                        }
                        break;

                    // Relative/Absolute positioning
                    case STYLE_POSITION_PROPERTY:
                        if (strcmp(text, "absolute") == 0) {
                            item.layoutFlags |= POSITION_ABSOLUTE;
                            item.propertyFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
                        } else if (strcmp(text, "relative") == 0) {
                            item.propertyFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
                        }
                        break;

                    // Hide/show
                    case STYLE_HIDE_PROPERTY:
                        if (strcmp(text, "true") == 0) {
                            item.layoutFlags |= HIDDEN;
                            item.propertyFlags |= PROPERTY_FLAG_HIDDEN;
                        }
                        else if (strcmp(text, "false") == 0) {
                            item.propertyFlags |= PROPERTY_FLAG_HIDDEN;
                        }
                        break;

                    // Ignore mouse
                    case STYLE_IGNORE_MOUSE_PROPERTY:
                        if (strcmp(text, "true") == 0) {
                            item.layoutFlags |= IGNORE_MOUSE;
                            item.propertyFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
                        }
                        else if (strcmp(text, "false") == 0) {
                            item.propertyFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
                        }
                        break;
                    
                    // Set gap
                    case STYLE_GAP_PROPERTY:
                        if (String_To_u8(&item.gap, text)) 
                            item.propertyFlags |= PROPERTY_FLAG_GAP;
                        break;

                    // Set preferred width
                    case STYLE_WIDTH_PROPERTY:
                        if (String_To_Uint16(&item.preferred_width, text))
                            item.propertyFlags |= PROPERTY_FLAG_PREFERRED_WIDTH;
                        break;

                    // Set min width
                    case STYLE_MIN_WIDTH_PROPERTY:
                        if (String_To_Uint16(&item.minWidth, text))
                            item.propertyFlags |= PROPERTY_FLAG_MIN_WIDTH;
                        break;
                    
                    // Set max width
                    case STYLE_MAX_WIDTH_PROPERTY:
                        if (String_To_Uint16(&item.maxWidth, text))
                            item.propertyFlags |= PROPERTY_FLAG_MAX_WIDTH;
                        break;

                    // Set preferred height
                    case STYLE_HEIGHT_PROPERTY:
                        if (String_To_Uint16(&item.preferred_height, text)) 
                            item.propertyFlags |= PROPERTY_FLAG_PREFERRED_HEIGHT;
                        break;

                    // Set min height
                    case STYLE_MIN_HEIGHT_PROPERTY:
                        if (String_To_Uint16(&item.minHeight, text)) 
                            item.propertyFlags |= PROPERTY_FLAG_MIN_HEIGHT;
                        break;

                    // Set max height
                    case STYLE_MAX_HEIGHT_PROPERTY:
                        if (String_To_Uint16(&item.maxHeight, text)) 
                            item.propertyFlags |= PROPERTY_FLAG_MAX_HEIGHT;
                        break;

                    // Set horizontal alignment
                    case STYLE_ALIGN_H_PROPERTY:
                        if (strcmp(text, "left") == 0) {
                            item.horizontalAlignment = 0;
                            item.propertyFlags |= PROPERTY_FLAG_ALIGN_H;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.horizontalAlignment = 1;
                            item.propertyFlags |= PROPERTY_FLAG_ALIGN_H;
                        }
                        if (strcmp(text, "right") == 0) {
                            item.horizontalAlignment = 2;
                            item.propertyFlags |= PROPERTY_FLAG_ALIGN_H;
                        }
                        break;

                    // Set vertical alignment
                    case STYLE_ALIGN_V_PROPERTY:
                        if (strcmp(text, "top") == 0) {
                            item.verticalAlignment = 0;
                            item.propertyFlags |= PROPERTY_FLAG_ALIGN_V;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.verticalAlignment = 1;
                            item.propertyFlags |= PROPERTY_FLAG_ALIGN_V;
                        }
                        if (strcmp(text, "bottom") == 0) {
                            item.verticalAlignment = 2;
                            item.propertyFlags |= PROPERTY_FLAG_ALIGN_V;
                        }
                        break;

                    // Set horizontal text alignment
                    case STYLE_TEXT_ALIGN_H_PROPERTY:
                        if (strcmp(text, "left") == 0) {
                            item.horizontalTextAlignment = 0;
                            item.propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.horizontalTextAlignment = 1;
                            item.propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                        }
                        if (strcmp(text, "right") == 0) {
                            item.horizontalTextAlignment = 2;
                            item.propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                        }
                        break;

                    // Set vertical text alignment
                    case STYLE_TEXT_ALIGN_V_PROPERTY:
                        if (strcmp(text, "top") == 0) {
                            item.verticalTextAlignment = 0;
                            item.propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                        }
                        if (strcmp(text, "center") == 0) {
                            item.verticalTextAlignment = 1;
                            item.propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                        }
                        if (strcmp(text, "bottom") == 0) {
                            item.verticalTextAlignment = 2;
                            item.propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                        }
                        break;

                    // Set absolute position properties
                    case STYLE_LEFT_PROPERTY:
                        if (String_To_Int16(&item.left, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_LEFT;
                        }
                        break;

                    case STYLE_RIGHT_PROPERTY:
                        if (String_To_Int16(&item.right, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_RIGHT;
                        }
                        break;

                    case STYLE_TOP_PROPERTY:
                        if (String_To_Int16(&item.top, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_TOP;
                        }
                        break;

                    case STYLE_BOTTOM_PROPERTY:
                        if (String_To_Int16(&item.bottom, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BOTTOM;
                        }
                        break;

                    // Set background colour
                    case STYLE_BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.backgroundR = rgb.r;
                            item.backgroundG = rgb.g;
                            item.backgroundB = rgb.b;
                            item.propertyFlags |= PROPERTY_FLAG_BACKGROUND;
                        } else if (strcmp(text, "none") == 0) {
                            item.propertyFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
                            item.layoutFlags |= HIDE_BACKGROUND;
                        }
                        break;

                    // Set border colour
                    case STYLE_BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.borderR = rgb.r;
                            item.borderG = rgb.g;
                            item.borderB = rgb.b;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_COLOUR;
                        }
                        break;

                    // Set text colour
                    case STYLE_TEXT_COLOUR_PROPERTY:
                        if (Parse_Hexcode(text, text_ref->char_count, &rgb)) {
                            item.textR = rgb.r;
                            item.textG = rgb.g;
                            item.textB = rgb.b;
                            item.propertyFlags |= PROPERTY_FLAG_TEXT_COLOUR;
                        }
                        break;
                    
                    // Set border width
                    case STYLE_BORDER_WIDTH_PROPERTY:
                        u8 border_width;
                        if (String_To_u8(&border_width, text)) {
                            item.borderTop = border_width;
                            item.borderBottom = border_width;
                            item.borderLeft = border_width;
                            item.borderRight = border_width;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_TOP;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_LEFT;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RIGHT;
                        }
                        break;
                    case STYLE_BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_u8(&item.borderTop, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_TOP;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_u8(&item.borderBottom, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
                        }
                        break;
                    case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
                        if (String_To_u8(&item.borderLeft, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_LEFT;
                        }
                        break;
                    case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_u8(&item.borderRight, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RIGHT;
                        }
                        break;

                    // Set border radii
                    case STYLE_BORDER_RADIUS_PROPERTY:
                        u8 border_radius;
                        if (String_To_u8(&border_radius, text)) {
                            item.borderRadiusTl = border_radius;
                            item.borderRadiusTr = border_radius;
                            item.borderRadiusBl = border_radius;
                            item.borderRadiusBr = border_radius;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
                        }
                        break;
                    case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_u8(&item.borderRadiusTl, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
                        }
                        break;
                    case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_u8(&item.borderRadiusTr, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_u8(&item.borderRadiusBl, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
                        }
                        break;
                    case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_u8(&item.borderRadiusBr, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
                        }
                        break;

                    // Set padding
                    case STYLE_PADDING_PROPERTY:
                        u8 pad;
                        if (String_To_u8(&pad, text)) {
                            item.padTop = pad;
                            item.padBottom = pad;
                            item.padLeft = pad;
                            item.padRight = pad;
                            item.propertyFlags |= PROPERTY_FLAG_PAD_TOP;
                            item.propertyFlags |= PROPERTY_FLAG_PAD_BOTTOM;
                            item.propertyFlags |= PROPERTY_FLAG_PAD_LEFT;
                            item.propertyFlags |= PROPERTY_FLAG_PAD_RIGHT;
                        }
                        break;
                    case STYLE_PADDING_TOP_PROPERTY:
                        if (String_To_u8(&item.padTop, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_PAD_TOP;
                        }
                        break;
                    case STYLE_PADDING_BOTTOM_PROPERTY:
                        if (String_To_u8(&item.padBottom, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_PAD_BOTTOM;
                        }
                        break;
                    case STYLE_PADDING_LEFT_PROPERTY:
                        if (String_To_u8(&item.padLeft, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_PAD_LEFT;
                        }
                        break;
                    case STYLE_PADDING_RIGHT_PROPERTY:
                        if (String_To_u8(&item.padRight, text)) {
                            item.propertyFlags |= PROPERTY_FLAG_PAD_RIGHT;
                        }
                        break;
                    
                    case STYLE_IMAGE_SOURCE_PROPERTY:
                        void* found = LinearStringmapGet(&imageFilepathToHandleMap, text);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(text);
                            if (image_handle) {
                                item.glImageHandle = image_handle;
                                LinearStringmapSet(&imageFilepathToHandleMap, text, &image_handle);
                                item.propertyFlags |= PROPERTY_FLAG_IMAGE;
                            }
                        } 
                        else {
                            item.propertyFlags |= PROPERTY_FLAG_IMAGE;
                            item.glImageHandle = *(GLuint*)found;
                        }
                        break;

                    case STYLE_FONT_PROPERTY:
                        void* found_font = LinearStringmapGet(&ss->fontNameIndexMap, text);
                        if (found_font != NULL) {
                            item.fontId = *(u8*)found_font;
                        }
                        break;

                    case STYLE_INPUT_TYPE_PROPERTY:
                        item.propertyFlags |= PROPERTY_FLAG_INPUT_TYPE;
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
                        createFontSrc = text;
                        break;

                    case STYLE_FONT_SIZE:
                        int size = 0;
                        if (String_To_Int(&size, text)) {
                            createFontSize = size;
                        }
                        break;

                    case STYLE_FONT_WEIGHT:
                        int weight = 0;
                        if (String_To_Int(&weight, text)) {
                            createFontWeight = weight;
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        i += 3;
    }

    // No fonts loaded -> default load embedded font
    if (ss->fonts.size == 0) {
        NU_Font font; 
        if (!NU_Font_Create_Default(&font, 14, true)) {
            LinearStringmapFree(&imageFilepathToHandleMap);
            LinearStringmapFree(&ss->fontNameIndexMap);
            return 0;
        }
        Container_Add(&ss->fonts, &font); // FontID will be 0
    }

    LinearStringmapFree(&imageFilepathToHandleMap);
    LinearStringmapFree(&ss->fontNameIndexMap);
    return 1;
}
