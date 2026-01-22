#pragma once 

#define NU_XML_PROPERTY_COUNT 44
const char* nu_xml_property_keywords[] = {
    "id", "class",
    "dir", "grow", "overflow-v", "overflow-h", "position", "hide", "gap",
    "width", "min-width", "max-width", "height", "min-height", "max-height",
    "align-h", "align-v", "text-align-h", "text-align-v",
    "left", "right", "top", "bottom",
    "background", "border-colour", "text-colour",
    "border", "border-top", "border-bottom", "border-left", "border-right",
    "border-radius", "border-radius-top-left", "border-radius-top-right", "border-radius-bottom-left", "border-radius-bottom-right",
    "padding", "padding-top", "padding-bottom", "padding-left", "padding-right",
    "image-src",
    "input-type", "decimals",
};

#define NU_XML_TYPE_COUNT 9
const char* nu_xml_type_keywords[] = {
    "window", "box", "button",
    "input", "canvas", "image",
    "table", "thead", "row",
};

typedef enum NU_XML_TOKEN
{
    // properties
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
    IMAGE_SOURCE_PROPERTY, INPUT_TYPE_PROPERTY, INPUT_DECIMALS_PROPERTY,

    // types
    WINDOW_TAG,
    BOX_TAG,
    BUTTON_TAG,
    TEXT_INPUT_TAG,
    CANVAS_TAG,
    IMAGE_TAG,
    TABLE_TAG,
    TABLE_HEAD_TAG,
    ROW_TAG,

    // xml misc
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
} NU_XML_TOKEN;

typedef struct Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count;
} Text_Ref;

NU_XML_TOKEN NU_Word_To_Token(char* word, uint8_t wordLen)
{
    word[wordLen] = '\0';

    // check property keywords
    for (int i=0; i<NU_XML_PROPERTY_COUNT; i++) {
        if (strcmp(word, nu_xml_property_keywords[i]) == 0) {
            return i;
        }
    }

    // check type keywords
    for (int i=0; i<NU_XML_TYPE_COUNT; i++) {
        if (strcmp(word, nu_xml_type_keywords[i]) == 0) {
            return i + NU_XML_PROPERTY_COUNT;
        }
    }
    return UNDEFINED;
}

NU_XML_TOKEN NU_Word_To_Tag_Token(char* word, uint8_t wordLen)
{
    word[wordLen] = '\0';
    for (int i=0; i<NU_XML_TYPE_COUNT; i++) {
        if (strcmp(word, nu_xml_type_keywords[i]) == 0) {
            return i + NU_XML_PROPERTY_COUNT;
        }
    }
    return UNDEFINED_TAG;
}

NU_XML_TOKEN NU_Word_To_Property_Token(char* word, uint8_t wordLen)
{
    word[wordLen] = '\0';
    for (int i=0; i<NU_XML_PROPERTY_COUNT; i++) {
        if (strcmp(word, nu_xml_property_keywords[i]) == 0) {
            return i;
        }
    }
    return UNDEFINED_PROPERTY;
}

NodeType NU_TokenToNodeType(NU_XML_TOKEN token)
{
    if (token < WINDOW_TAG || token > ROW_TAG) return NU_NAT;
    return token - NU_XML_PROPERTY_COUNT;
}

int NU_Is_Token_Property(NU_XML_TOKEN t)
{
    return (t >= ID_PROPERTY && t <= INPUT_DECIMALS_PROPERTY) || t == UNDEFINED_PROPERTY; 
}