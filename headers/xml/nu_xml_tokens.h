#pragma once 

#define NU_XML_PROPERTY_COUNT 42
#define NU_XML_KEYWORD_COUNT 51

static const char* nu_xml_keywords[] = {
    "id", "class", "dir", "grow", "overflowV", "overflowH", "position", "hide", "gap",
    "width", "minWidth", "maxWidth", "height", "minHeight", "maxHeight", 
    "alignH", "alignV", "textAlignH", "textAlignV",
    "left", "right", "top", "bottom",
    "background", "borderColour", "textColour",
    "border", "borderTop", "borderBottom", "borderLeft", "borderRight",
    "borderRadius", "borderRadiusTopLeft", "borderRadiusTopRight", "borderRadiusBottomLeft", "borderRadiusBottomRight",
    "pad", "padTop", "padBottom", "padLeft", "padRight",
    "imageSrc",
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
    2, 5, 3, 4, 9, 9, 8, 4, 3,
    5, 8, 8, 6, 9, 9,          // width height
    6, 6, 10, 10,              // alignment
    4, 5, 3, 6,                // absolute positioning
    10, 12, 10,                // background, border, text colour
    6, 9, 12, 10, 11,          // border width
    12, 19, 20, 22, 23,        // border radius
    3, 6, 9, 7, 8,             // padding
    8,                         // image src
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
