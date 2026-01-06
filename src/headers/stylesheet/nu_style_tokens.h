#pragma once

#define STYLE_PROPERTY_COUNT 44
#define STYLE_KEYWORD_COUNT 57
#define STYLE_TAG_SELECTOR_COUNT 9
#define STYLE_SPECIAL_SELECTOR_COUNT 1
#define STYLE_PSEUDO_COUNT 3

static const char* style_keywords[] = {
    "dir", "grow", "overflow-v", "overflow-h", "position", "hide", "gap",
    "width", "min-width", "max-width", "height",  "min-height", "max-height",
    "align-h", "align-v", "text-align-h", "text-align-v",
    "left", "right", "top", "bottom",
    "background", "border-colour", "text-colour",
    "border", "border-top", "border-bottom", "border-left", "border-right",
    "border-radius", "border-radius-top-left", "border-radius-top-right", "border-radius-bottom-left", "border-radius-bottom-right",
    "padding", "padding-top", "padding-bottom", "padding-left", "padding-right", 
    "imags-src", "font", "src", "size", "weight",
    "window", "rect", "button", "grid", "canvas", "image", "table", "thead", "row", "@font",
    "hover", "press", "focus",
};

static const uint8_t style_keyword_lengths[] = { 
    3, 4, 10, 10, 8, 4, 3, 
    5, 9, 9, 6, 10, 10,           // width height
    7, 7, 12, 12,                 // alignment
    4, 5, 3, 6,                   // absolute positioning
    10, 13, 11,                   // background, border, text colour
    6, 10, 13, 11, 12,            // border width
    13, 22, 23, 25, 26,           // border radius
    7, 11, 14, 12, 13,            // padding
    9,                            // image src
    4,                            // font property (css only, no xml inline equivalent)
    3, 4, 6,                      // font creation
    6, 4, 6, 4, 6, 5, 5, 5, 3,    // tag selectors
    5,                            // other selectors
    5, 5, 5,                      // pseudo classes
};
enum NU_Style_Token 
{
    // --- Inline node poperties ---
    STYLE_LAYOUT_DIRECTION_PROPERTY,
    STYLE_GROW_PROPERTY,
    STYLE_OVERFLOW_V_PROPERTY,
    STYLE_OVERFLOW_H_PROPERTY,
    STYLE_POSITION_PROPERTY,
    STYLE_HIDE_PROPERTY,
    STYLE_GAP_PROPERTY,
    STYLE_WIDTH_PROPERTY,
    STYLE_MIN_WIDTH_PROPERTY,
    STYLE_MAX_WIDTH_PROPERTY,
    STYLE_HEIGHT_PROPERTY,
    STYLE_MIN_HEIGHT_PROPERTY,
    STYLE_MAX_HEIGHT_PROPERTY,
    STYLE_ALIGN_H_PROPERTY,
    STYLE_ALIGN_V_PROPERTY,
    STYLE_TEXT_ALIGN_H_PROPERTY,
    STYLE_TEXT_ALIGN_V_PROPERTY,
    STYLE_LEFT_PROPERTY,
    STYLE_RIGHT_PROPERTY,
    STYLE_TOP_PROPERTY,
    STYLE_BOTTOM_PROPERTY,
    STYLE_BACKGROUND_COLOUR_PROPERTY,
    STYLE_BORDER_COLOUR_PROPERTY,
    STYLE_TEXT_COLOUR_PROPERTY,
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
    STYLE_IMAGE_SOURCE_PROPERTY,
    

    // --- CSS only node properties ---
    STYLE_FONT_PROPERTY,
    STYLE_FONT_SRC,
    STYLE_FONT_SIZE,
    STYLE_FONT_WEIGHT,

    // --- Tag selectors ---
    STYLE_WINDOW_SELECTOR,
    STYLE_RECT_SELECTOR,
    STYLE_BUTTON_SELECTOR,
    STYLE_GRID_SELECTOR,
    STYLE_CANVAS_SELECTOR,
    STYLE_IMAGE_SELECTOR,
    STYLE_TABLE_SELECTOR,
    STYLE_THEAD_SELECTOR,
    STYLE_ROW_SELECTOR,

    // --- Special selectors --- 
    STYLE_FONT_CREATION_SELECTOR,

    // --- Pseudos ---
    STYLE_HOVER_PSEUDO,
    STYLE_PRESS_PSEUDO,
    STYLE_FOCUS_PSEUDO,
    // --- End of keyword direct mapping ---


    // --- Class/ID selector prefixes ---
    STYLE_ID_SELECTOR,
    STYLE_CLASS_SELECTOR,

    // --- Syntax tokens ---
    STYLE_PSEUDO_COLON,
    STYLE_SELECTOR_COMMA,
    STYLE_SELECTOR_OPEN_BRACE,
    STYLE_SELECTOR_CLOSE_BRACE,
    STYLE_PROPERTY_ASSIGNMENT,
    STYLE_PROPERTY_VALUE,
    STYLE_FONT_CREATION_PROPERTY_VALUE,
    STYLE_FONT_NAME,

    // --- Unknown token (error occured) --- 
    STYLE_UNDEFINED
};

enum NU_Pseudo_Class
{
    PSEUDO_HOVER,
    PSEUDO_PRESS,
    PSEUDO_FOCUS,
    PSEUDO_UNDEFINED
};

static uint32_t Property_Token_To_Flag(enum NU_Style_Token token)
{
    return (1u << token);
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

static enum NU_Style_Token NU_Word_To_Style_Property_Token(char* word, uint8_t word_char_count)
{
    for (int i=0; i<STYLE_PROPERTY_COUNT; i++)
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
    int start = STYLE_PROPERTY_COUNT;
    int end = STYLE_PROPERTY_COUNT + STYLE_TAG_SELECTOR_COUNT;
    for (int i=start; i<end; i++)
    {
        size_t len = style_keyword_lengths[i];
        if (len == word_char_count && memcmp(word, style_keywords[i], len) == 0) {
            return i;
        }
    }
    return STYLE_UNDEFINED;
}

static enum NU_Style_Token NU_Word_To_Any_Selector_Token(char* word, uint8_t word_char_count)
{
    int start = STYLE_PROPERTY_COUNT;
    int end = STYLE_PROPERTY_COUNT + STYLE_TAG_SELECTOR_COUNT + STYLE_SPECIAL_SELECTOR_COUNT;
    for (int i=start; i<end; i++)
    {
        size_t len = style_keyword_lengths[i];
        if (len == word_char_count && memcmp(word, style_keywords[i], len) == 0) {
            return i;
        }
    }
    return STYLE_UNDEFINED;
}

static enum NU_Style_Token NU_Word_To_Pseudo_Token(char* word, uint8_t word_char_count)
{
    int start = STYLE_PROPERTY_COUNT + STYLE_TAG_SELECTOR_COUNT + STYLE_SPECIAL_SELECTOR_COUNT;
    int end = start + STYLE_PSEUDO_COUNT;
    for (int i=start; i<end; i++)
    {
        size_t len = style_keyword_lengths[i];
        if (len == word_char_count && memcmp(word, style_keywords[i], len) == 0) {
            return i;
        }
    }
    return STYLE_UNDEFINED;
}

static enum NU_Pseudo_Class Token_To_Pseudo_Class(enum NU_Style_Token token)
{
    return token - (STYLE_PROPERTY_COUNT + STYLE_TAG_SELECTOR_COUNT + STYLE_SPECIAL_SELECTOR_COUNT);
}

static int NU_Is_Property_Identifier_Token(enum NU_Style_Token token)
{
    if (token < STYLE_PROPERTY_COUNT) return 1;
    return 0;
}

static int NU_Is_Tag_Selector_Token(enum NU_Style_Token token)
{
    return token > (STYLE_PROPERTY_COUNT - 1) && token < STYLE_PROPERTY_COUNT + STYLE_TAG_SELECTOR_COUNT;
}

static int NU_Is_Pseudo_Token(enum NU_Style_Token token)
{
    int start = STYLE_PROPERTY_COUNT + STYLE_TAG_SELECTOR_COUNT + STYLE_SPECIAL_SELECTOR_COUNT - 1;
    int end = start + STYLE_PSEUDO_COUNT + 1;
    return token > start && token < end;
}

