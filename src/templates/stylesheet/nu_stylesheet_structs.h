#pragma once

typedef enum StylePseudoClass
{
    PSEUDO_HOVER,
    PSEUDO_PRESS,
    PSEUDO_FOCUS,
    PSEUDO_UNDEFINED
} StylePseudoClass;

#include "nu_stylesheet_tokens.h"

typedef struct StyleItemKey {
    int classID;
    int idID;
    int tag;
    int pseudoClass;
} StyleItemKey;

typedef struct StyleItem
{
    uint64 propertyFlags;
    int nextVariationIndex;
    int screenQueryIndex;
    int imageHandle;
    uint16 preferredWidth, preferredHeight;
    uint16 minWidth, maxWidth, minHeight, maxHeight;
    int16 left, right, top, bottom;
    uint16 layoutFlags;
    uint8 gap, padTop, padBottom, padLeft, padRight;
    uint8 borderTop, borderBottom, borderLeft, borderRight;
    uint8 borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    uint8 backgroundR, backgroundG, backgroundB;
    uint8 borderR, borderG, borderB;
    uint8 textR, textG, textB;
    uint8 fontIdx;
    uint8 imageWidthDimType;
    uint8 imageHeightDimType;
    uint8 imageRepeat;
    uint8 imageFilter;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    uint8 inputType;
} StyleItem;

typedef struct StylesheetVariable
{
    enum StylesheetVariableDtype type;
    int value;
    int nextVariationIndex;
    int screenQueryIndex;
} StylesheetVariable;

typedef struct StylesheetScreenQuery
{
    StylesheetToken comparator;
    int screenWidth;
} StylesheetScreenQuery;

typedef struct Stylesheet_Scrollbar_Style
{
    bool overlay;
    uint8 width;
    uint8 height;
    uint8 thumbMinSize;
    uint8 thumbBorderTop, thumbBorderBottom, thumbBorderLeft, thumbBorderRight;
    uint8 thumbBorderRadiusTl, thumbBorderRadiusTr, thumbBorderRadiusBl, thumbBorderRadiusBr;
    uint8 thumbBackgroundR, thumbBackgroundG, thumbBackgroundB;
    uint8 thumbBorderR, thumbBorderG, thumbBorderB;
    uint8 trackPadTop, trackPadBottom, trackPadLeft, trackPadRight;
    uint8 trackBorderTop, trackBorderBottom, trackBorderLeft, trackBorderRight;
    uint8 trackBorderRadiusTl, trackBorderRadiusTr, trackBorderRadiusBl, trackBorderRadiusBr;
    uint8 trackBackgroundR, trackBackgroundG, trackBackgroundB;
    uint8 trackBorderR, trackBorderG, trackBorderB;
} Stylesheet_Scrollbar_Style;

typedef struct Stylesheet
{
    Array items;
    Array pseudoItems;
    Array variables;
    Array screenQueries;
    Array fonts;
    LinearStringmap classIdMap;
    LinearStringmap idIdMap;
    Hashmap itemIndexMap;
    LinearStringmap fontNameIndexMap;
    Stylesheet_Scrollbar_Style scrollbarStyle;
} Stylesheet;

typedef struct StyleTextRef
{
    uint32 tokenIndex;
    uint32 srcIndex;
    uint32 len;
} StyleTextRef;
