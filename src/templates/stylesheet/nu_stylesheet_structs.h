#pragma once
#include "nu_stylesheet_tokens.h"

typedef struct StyleItemKey {
    int classID;
    int idID;
    int tag;
    int pseudoClass;
} StyleItemKey;

typedef struct StyleItem
{
    u64 propertyFlags;
    int nextVariationIndex;
    int screenQueryIndex;
    int imageHandle;
    u16 preferredWidth, preferredHeight;
    u16 minWidth, maxWidth, minHeight, maxHeight;
    i16 left, right, top, bottom;
    u16 layoutFlags;
    u8 gap, padTop, padBottom, padLeft, padRight;
    u8 borderTop, borderBottom, borderLeft, borderRight;
    u8 borderRadiusTl, borderRadiusTr, borderRadiusBl, borderRadiusBr;
    u8 backgroundR, backgroundG, backgroundB;
    u8 borderR, borderG, borderB;
    u8 textR, textG, textB;
    u8 fontId;
    char horizontalAlignment;
    char verticalAlignment;
    char horizontalTextAlignment;
    char verticalTextAlignment;
    u8 inputType;
} StyleItem;

typedef enum StylesheetVariableDtype
{
    STYLESHEET_VARIABLE_DTYPE_RGB,
    STYLESHEET_VARIABLE_DTYPE_NUMBER,
    STYLESHEET_VARIABLE_DTYPE_TOP,
    STYLESHEET_VARIABLE_DTYPE_BOTTOM,
    STYLESHEET_VARIABLE_DTYPE_LEFT,
    STYLESHEET_VARIABLE_DTYPE_RIGHT,
    STYLESHEET_VARIABLE_DTYPE_CENTER,
    STYLESHEET_VARIABLE_DTYPE_VERTICAL,
    STYLESHEET_VARIABLE_DTYPE_HORIZONTAL,
    STYLESHEET_VARIABLE_DTYPE_BOTH,
    STYLESHEET_VARIABLE_DTYPE_SCROLL,
    STYLESHEET_VARIABLE_DTYPE_TRUE,
    STYLESHEET_VARIABLE_DTYPE_FALSE,
    STYLESHEET_VARIABLE_DTYPE_ABSOLUTE,
    STYLESHEET_VARIABLE_DTYPE_RELATIVE,
    STYLESHEET_VARIABLE_DTYPE_NONE,
    STYLESHEET_VARIABLE_DTYPE_INPUT_NUMBER,
    STYLESHEET_VARIABLE_DTYPE_UNKNOWN,
} StylesheetVariableDtype;

typedef struct StylesheetVariable
{
    enum StylesheetVariableDtype type;
    int value;
    int nextVariationIndex;
    int screenQueryIndex;
} StylesheetVariable;

typedef struct StylesheetScreenQuery
{
    enum NU_Style_Token comparator;
    int screenWidth;
} StylesheetScreenQuery;

typedef struct Stylesheet_Scrollbar_Style
{
    bool overlay;
    u8 width;
    u8 height;
    u8 thumbMinSize;
    u8 thumbBorderTop, thumbBorderBottom, thumbBorderLeft, thumbBorderRight;
    u8 thumbBorderRadiusTl, thumbBorderRadiusTr, thumbBorderRadiusBl, thumbBorderRadiusBr;
    u8 thumbBackgroundR, thumbBackgroundG, thumbBackgroundB;
    u8 thumbBorderR, thumbBorderG, thumbBorderB;
    u8 trackPadTop, trackPadBottom, trackPadLeft, trackPadRight;
    u8 trackBorderTop, trackBorderBottom, trackBorderLeft, trackBorderRight;
    u8 trackBorderRadiusTl, trackBorderRadiusTr, trackBorderRadiusBl, trackBorderRadiusBr;
    u8 trackBackgroundR, trackBackgroundG, trackBackgroundB;
    u8 trackBorderR, trackBorderG, trackBorderB;
} Stylesheet_Scrollbar_Style;

typedef struct Stylesheet
{
    Array items;
    Array pseudoItems;
    Array variables;
    Array screenQueries;
    LinearStringmap classIdMap;
    LinearStringmap idIdMap;
    Hashmap itemIndexMap;
    LinearStringmap fontNameIndexMap;
    Container fonts;
    Stylesheet_Scrollbar_Style scrollbarStyle;
} Stylesheet;

typedef struct StyleTextRef
{
    u32 tokenIndex;
    u32 srcIndex;
    u32 len;
} StyleTextRef;
