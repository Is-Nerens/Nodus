#pragma once
#include <utils/nu_file.h>
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
    Array_Init(&ss->fonts, sizeof(Font), 8);
    LinearStringmap_Init(&ss->classIndexMap, sizeof(int), 256, 2048);
    LinearStringmap_Init(&ss->idIndexMap, sizeof(int), 256, 2048);
    LinearStringmap_Init(&ss->fontNameIndexMap, sizeof(int), 16, 256);

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

    // Create tag styles and pseudos
    // These can trivially be indexed
    // using maths for fast lookup
    // index = 1 + ((node->type | tag) * 4 + pseudo)
    StyleItem* windowBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* windowHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* windowPressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* windowFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* boxBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* boxHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* boxPressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* boxFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* buttonBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* buttonHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* buttonPressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* buttonFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* inputBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* inputHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* inputPressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* inputFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* canvasBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* canvasHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* canvasPressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* canvasFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* imageBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* imageHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* imagePressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* imageFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* tableBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* tableHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* tablePressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* tableFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* theadBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* theadHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* theadPressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* theadFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* rowBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* rowHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* rowPressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* rowFocusStyle = Array_PushEmptyZero(&ss->items);

    StyleItem* frameBaseStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* frameHoverStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* framePressStyle = Array_PushEmptyZero(&ss->items);
    StyleItem* frameFocusStyle = Array_PushEmptyZero(&ss->items);

    // Set variation chain to none
    for (int i=0; i<Array_Length(&ss->items); i++) {
        StyleItem* item = Array_Get(&ss->items, i);
        item->nextVariationIndex = -1;
        item->screenQueryIndex = -1;
        item->hoverPseudoItemIndex = -1;
        item->pressPseudoItemIndex = -1;
        item->focusPseudoItemIndex = -1;
    }

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

void Stylesheet_Free(Stylesheet* ss, VkDevice device, VkDescriptorPool fontPool)
{
    Array_Free(&ss->items);
    Array_Free(&ss->variables);
    Array_Free(&ss->screenQueries);
    LinearStringmap_Free(&ss->classIndexMap);
    LinearStringmap_Free(&ss->idIndexMap);
    LinearStringmap_Free(&ss->fontNameIndexMap);
    for (int i=0; i<ss->fonts.size; i++) {
        Font* font = Array_Get(&ss->fonts, i);
        Font_Free(font, device, fontPool);
    }
    Array_Free(&ss->fonts);
}

int Stylesheet_Create(
    Stylesheet* stylesheet,
    ThreadPool* threadpool,
    const char* filepath,
    ImageResourceManager* imageResourceManager,
    DisplayManager* displayManager)
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
        imageResourceManager,
        displayManager)
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

static inline Font* Stylesheet_Get_Font(Stylesheet* ss, uint8 fontIdx) {
    return Array_Get(&ss->fonts, fontIdx);
}
