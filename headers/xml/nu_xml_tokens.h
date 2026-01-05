#pragma once 

#define NU_XML_PROPERTY_COUNT 42
#define NU_XML_TAG_COUNT 9
#define NU_XML_KEYWORD_COUNT 51

static const char* nu_xml_keywords[] = {
    "id", "class", "dir", "grow", "overflow-v", "overflow-h", "position", "hide", "gap",
    "width", "min-width", "max-width", "height", "min-height", "max-height", 
    "align-h", "align-v", "text-align-h", "text-align-v",
    "left", "right", "top", "bottom",
    "background", "border-colour", "text-colour",
    "border", "border-top", "border-bottom", "border-left", "border-right",
    "border-radius", "border-radius-top-left", "border-radius-top-right", "border-radius-bottom-left", "border-radius-bottom-right",
    "padding", "padding-top", "padding-bottom", "padding-left", "padding-right", 
    "image-src",
    "window",
    "rect",
    "button",
    "grid",
    "canvas",
    "image",
    "table",
    "thead",
    "row",
};
static const uint8_t keyword_lengths[] = { 
    2, 5, 3, 4, 10, 10, 8, 4, 3,
    5, 9, 9, 6, 10, 10,        // width height
    7, 7, 12, 12,              // alignment
    4, 5, 3, 6,                // absolute positioning
    10, 13, 11,                // background, border, text colour
    6, 10, 13, 11, 12,         // border width
    13, 22, 23, 25, 26,        // border radius
    7, 11, 14, 12, 13,         // padding
    9,                         // image src
    6, 4, 6, 4, 6, 5, 5, 5, 3  // tags
};
enum NU_XML_Token
{
    // --- Property Tokens ---
    ID_PROPERTY, CLASS_PROPERTY, 
    LAYOUT_DIRECTION_PROPERTY, GROW_PROPERTY,
    OVERFLOW_V_PROPERTY, OVERFLOW_H_PROPERTY,
    POSITION_PROPERTY,
    HIDE_PROPERTY,
    GAP_PROPERTY,
    WIDTH_PROPERTY, MIN_WIDTH_PROPERTY, MAX_WIDTH_PROPERTY,
    HEIGHT_PROPERTY, MIN_HEIGHT_PROPERTY, MAX_HEIGHT_PROPERTY,
    ALIGN_H_PROPERTY, ALIGN_V_PROPERTY, TEXT_ALIGN_H_PROPERTY, TEXT_ALIGN_V_PROPERTY,
    LEFT_PROPERTY, RIGHT_PROPERTY, TOP_PROPERTY, BOTTOM_PROPERTY,
    BACKGROUND_COLOUR_PROPERTY, BORDER_COLOUR_PROPERTY, TEXT_COLOUR_PROPERTY,
    BORDER_WIDTH_PROPERTY, BORDER_TOP_WIDTH_PROPERTY, BORDER_BOTTOM_WIDTH_PROPERTY, BORDER_LEFT_WIDTH_PROPERTY, BORDER_RIGHT_WIDTH_PROPERTY,
    BORDER_RADIUS_PROPERTY, BORDER_TOP_LEFT_RADIUS_PROPERTY, BORDER_TOP_RIGHT_RADIUS_PROPERTY, BORDER_BOTTOM_LEFT_RADIUS_PROPERTY, BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY,
    PADDING_PROPERTY, PADDING_TOP_PROPERTY, PADDING_BOTTOM_PROPERTY, PADDING_LEFT_PROPERTY, PADDING_RIGHT_PROPERTY,
    IMAGE_SOURCE_PROPERTY,

    // --- Tag Tokens ---
    WINDOW_TAG,
    RECT_TAG,
    BUTTON_TAG,
    GRID_TAG,
    TEXT_TAG,
    IMAGE_TAG,
    TABLE_TAG,
    TABLE_HEAD_TAG,
    ROW_TAG,
    OPEN_TAG,
    CLOSE_TAG,
    OPEN_END_TAG,
    CLOSE_END_TAG,
    PROPERTY_ASSIGNMENT,
    PROPERTY_VALUE,
    TEXT_CONTENT,
    UNDEFINED_TAG,
    UNDEFINED_PROPERTY,
    UNDEFINED
};

struct Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count;
};

static enum NU_XML_Token NU_Word_To_Token(char word[], uint8_t word_char_count)
{
    for (uint8_t i=0; i<NU_XML_KEYWORD_COUNT; i++) {
        size_t len = keyword_lengths[i];
        if (len == word_char_count && memcmp(word, nu_xml_keywords[i], len) == 0) {
            return i;
        }
    }
    return UNDEFINED;
}

static enum NU_XML_Token NU_Word_To_Tag_Token(char word[], uint8_t word_char_count)
{
    for (uint8_t i=NU_XML_PROPERTY_COUNT; i<NU_XML_PROPERTY_COUNT + NU_XML_TAG_COUNT; i++) {
        size_t len = keyword_lengths[i];
        if (len == word_char_count && memcmp(word, nu_xml_keywords[i], len) == 0) {
            return i;
        }
    }
    return UNDEFINED_TAG;
}

static enum NU_XML_Token NU_Word_To_Property_Token(char word[], uint8_t word_char_count)
{
    for (uint8_t i=0; i<NU_XML_PROPERTY_COUNT; i++) {
        size_t len = keyword_lengths[i];
        if (len == word_char_count && memcmp(word, nu_xml_keywords[i], len) == 0) {
            return i;
        }
    }
    return UNDEFINED_PROPERTY;
}

static enum Tag NU_Token_To_Tag(enum NU_XML_Token NU_XML_Token)
{
    int tag_candidate = NU_XML_Token - NU_XML_PROPERTY_COUNT;
    if (tag_candidate < 0) return NAT;
    return NU_XML_Token - NU_XML_PROPERTY_COUNT;
}

static int NU_Is_Token_Property(enum NU_XML_Token NU_XML_Token)
{
    return NU_XML_Token < NU_XML_PROPERTY_COUNT;
}
