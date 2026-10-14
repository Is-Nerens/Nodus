#pragma once
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/nu_convert.h>
#include <filesystem/nu_file.h>
#include "nu_stylesheet_tokens.h"
#include "nu_stylesheet_structs.h"
#include "nu_stylesheet_tokeniser.h"
#include "nu_stylesheet_parser.h"
#include "nu_stylesheet_apply.h"

void Stylesheet_Init(Stylesheet* ss)
{
    Array_Init(&ss->items, sizeof(StyleItem), 512);
    Array_Init(&ss->variables, sizeof(StylesheetVariable), 128);
    Array_Init(&ss->screenQueries, sizeof(StylesheetScreenQuery), 8);
    Array_Init(&ss->fonts, sizeof(NU_Font), 8);
    LinearStringmap_Init(&ss->classIdMap, sizeof(int), 256, 2048);
    LinearStringmap_Init(&ss->idIdMap, sizeof(int), 256, 2048);
    LinearStringmap_Init(&ss->fontNameIndexMap, sizeof(int), 16, 256);
    Hashmap_Init(&ss->itemIndexMap, sizeof(StyleItemKey), sizeof(int), 512);

    // Create default style item (at index 0)
    StyleItem* item = Array_PushEmpty(&ss->items);
    memset(item, 0, sizeof(StyleItem)); // zero base
    item->nextVariationIndex = -1;
    item->screenQueryIndex = -1;
    item->propertyFlags = ~(uint64_t)0; // apply all properties
    item->propertyFlags &= ~PROPERTY_FLAG_IMAGE; // do not apply certain properties
    item->propertyFlags &= ~PROPERTY_FLAG_HIDDEN;
    item->propertyFlags &= ~PROPERTY_FLAG_TOP;
    item->propertyFlags &= ~PROPERTY_FLAG_BOTTOM;
    item->propertyFlags &= ~PROPERTY_FLAG_LEFT;
    item->propertyFlags &= ~PROPERTY_FLAG_RIGHT;
    item->propertyFlags &= ~PROPERTY_FLAG_MIN_WIDTH;
    item->propertyFlags &= ~PROPERTY_FLAG_MIN_HEIGHT;
    item->propertyFlags &= ~PROPERTY_FLAG_MAX_WIDTH;
    item->propertyFlags &= ~PROPERTY_FLAG_MAX_HEIGHT;
    item->propertyFlags &= ~PROPERTY_FLAG_PREFERRED_WIDTH;
    item->propertyFlags &= ~PROPERTY_FLAG_PREFERRED_HEIGHT;
    item->backgroundR = 50; // colors
    item->backgroundG = 50;
    item->backgroundB = 50;
    item->borderR = 100;
    item->borderG = 100;
    item->borderB = 100;
    item->textR = 255;
    item->textG = 255;
    item->textB = 255;
    item->horizontalTextAlignment = 1; // alignment
    item->verticalTextAlignment = 1;

    // Set default scrollbar styles
    memset(&ss->scrollbarStyle, 0, sizeof(Stylesheet_Scrollbar_Style)); // zero base
    ss->scrollbarStyle.width = 8;
    ss->scrollbarStyle.height = 8;
    ss->scrollbarStyle.overlay = false;
    ss->scrollbarStyle.thumbMinSize = 4;
    ss->scrollbarStyle.thumbBackgroundR = 210;
    ss->scrollbarStyle.thumbBackgroundG = 210;
    ss->scrollbarStyle.thumbBackgroundB = 210;
    ss->scrollbarStyle.thumbBorderR = 240;
    ss->scrollbarStyle.thumbBorderG = 240;
    ss->scrollbarStyle.thumbBorderB = 240;
    ss->scrollbarStyle.trackBackgroundR = 40;
    ss->scrollbarStyle.trackBackgroundG = 40;
    ss->scrollbarStyle.trackBackgroundB = 40;
    ss->scrollbarStyle.trackBorderR = 40;
    ss->scrollbarStyle.trackBorderG = 40;
    ss->scrollbarStyle.trackBorderB = 40;
}

void Stylesheet_Free(Stylesheet* ss)
{
    Array_Free(&ss->items);
    Array_Free(&ss->variables);
    Array_Free(&ss->screenQueries);
    LinearStringmap_Free(&ss->classIdMap);
    LinearStringmap_Free(&ss->idIdMap);
    Hashmap_Free(&ss->itemIndexMap);
    LinearStringmap_Free(&ss->fontNameIndexMap);
    for (int i=0; i<ss->fonts.size; i++) {
        NU_Font* font = Array_Get(&ss->fonts, i);
        NU_Font_Free(font);
    }
    Array_Free(&ss->fonts);
}

int Stylesheet_Create(Stylesheet* stylesheet, ThreadPool* threadpool, const char* filepath, ImageResourceLoader* imageResourceLoader)
{
    Stylesheet_Init(stylesheet);

    // Load CSS file into memory
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;

    // Init temp data structures
    TokenArray tokens = TokenArray_Create(8000);
    Array textRefs; Array_Init(&textRefs, sizeof(StyleTextRef), 2000);
    LinearStringmap variableMap; LinearStringmap_Init(&variableMap, sizeof(int), 128, 4096);

    // Tokenise
    Stylesheet_Tokenise(src, &tokens, &textRefs);

    // Generate stylesheet
    if (!Stylesheet_Parse(
        StringCstr(src),
        &tokens,
        &textRefs,
        &variableMap,
        stylesheet,
        threadpool,
        imageResourceLoader)
    ) {
        TokenArray_Free(&tokens);
        Array_Free(&textRefs);
        LinearStringmap_Free(&variableMap);
        StringFree(src);
        return 0;
    }

    // Free memory
    TokenArray_Free(&tokens);
    Array_Free(&textRefs);
    LinearStringmap_Free(&variableMap);
    StringFree(src);
    return 1; // Success
}

static inline NU_Font* Stylesheet_Get_Font(Stylesheet* ss, u8 fontIdx) {
    return Array_Get(&ss->fonts, fontIdx);
}
