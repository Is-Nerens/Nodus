#pragma once 
#define NU_XML_PROPERTY_COUNT 42
#define NU_XML_TAG_COUNT 9
#define NU_XML_KEYWORD_COUNT 51

const char* nu_xml_keywords[] = {
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
    "box",
    "button",
    "input",
    "canvas",
    "image",
    "table",
    "thead",
    "row",
};

enum NU_XML_TOKEN
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
    BOX_TAG,
    BUTTON_TAG,
    TEXT_INPUT_TAG,
    CANVAS_TAG,
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

void printToken(enum NU_XML_TOKEN token)
{
    switch (token)
    {
        // Property Tokens
        case ID_PROPERTY: printf("ID_PROPERTY"); break;
        case CLASS_PROPERTY: printf("CLASS_PROPERTY"); break;
        case LAYOUT_DIRECTION_PROPERTY: printf("LAYOUT_DIRECTION_PROPERTY"); break;
        case GROW_PROPERTY: printf("GROW_PROPERTY"); break;
        case OVERFLOW_V_PROPERTY: printf("OVERFLOW_V_PROPERTY"); break;
        case OVERFLOW_H_PROPERTY: printf("OVERFLOW_H_PROPERTY"); break;
        case POSITION_PROPERTY: printf("POSITION_PROPERTY"); break;
        case HIDE_PROPERTY: printf("HIDE_PROPERTY"); break;
        case GAP_PROPERTY: printf("GAP_PROPERTY"); break;
        case WIDTH_PROPERTY: printf("WIDTH_PROPERTY"); break;
        case MIN_WIDTH_PROPERTY: printf("MIN_WIDTH_PROPERTY"); break;
        case MAX_WIDTH_PROPERTY: printf("MAX_WIDTH_PROPERTY"); break;
        case HEIGHT_PROPERTY: printf("HEIGHT_PROPERTY"); break;
        case MIN_HEIGHT_PROPERTY: printf("MIN_HEIGHT_PROPERTY"); break;
        case MAX_HEIGHT_PROPERTY: printf("MAX_HEIGHT_PROPERTY"); break;
        case ALIGN_H_PROPERTY: printf("ALIGN_H_PROPERTY"); break;
        case ALIGN_V_PROPERTY: printf("ALIGN_V_PROPERTY"); break;
        case TEXT_ALIGN_H_PROPERTY: printf("TEXT_ALIGN_H_PROPERTY"); break;
        case TEXT_ALIGN_V_PROPERTY: printf("TEXT_ALIGN_V_PROPERTY"); break;
        case LEFT_PROPERTY: printf("LEFT_PROPERTY"); break;
        case RIGHT_PROPERTY: printf("RIGHT_PROPERTY"); break;
        case TOP_PROPERTY: printf("TOP_PROPERTY"); break;
        case BOTTOM_PROPERTY: printf("BOTTOM_PROPERTY"); break;
        case BACKGROUND_COLOUR_PROPERTY: printf("BACKGROUND_COLOUR_PROPERTY"); break;
        case BORDER_COLOUR_PROPERTY: printf("BORDER_COLOUR_PROPERTY"); break;
        case TEXT_COLOUR_PROPERTY: printf("TEXT_COLOUR_PROPERTY"); break;
        case BORDER_WIDTH_PROPERTY: printf("BORDER_WIDTH_PROPERTY"); break;
        case BORDER_TOP_WIDTH_PROPERTY: printf("BORDER_TOP_WIDTH_PROPERTY"); break;
        case BORDER_BOTTOM_WIDTH_PROPERTY: printf("BORDER_BOTTOM_WIDTH_PROPERTY"); break;
        case BORDER_LEFT_WIDTH_PROPERTY: printf("BORDER_LEFT_WIDTH_PROPERTY"); break;
        case BORDER_RIGHT_WIDTH_PROPERTY: printf("BORDER_RIGHT_WIDTH_PROPERTY"); break;
        case BORDER_RADIUS_PROPERTY: printf("BORDER_RADIUS_PROPERTY"); break;
        case BORDER_TOP_LEFT_RADIUS_PROPERTY: printf("BORDER_TOP_LEFT_RADIUS_PROPERTY"); break;
        case BORDER_TOP_RIGHT_RADIUS_PROPERTY: printf("BORDER_TOP_RIGHT_RADIUS_PROPERTY"); break;
        case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY: printf("BORDER_BOTTOM_LEFT_RADIUS_PROPERTY"); break;
        case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY: printf("BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY"); break;
        case PADDING_PROPERTY: printf("PADDING_PROPERTY"); break;
        case PADDING_TOP_PROPERTY: printf("PADDING_TOP_PROPERTY"); break;
        case PADDING_BOTTOM_PROPERTY: printf("PADDING_BOTTOM_PROPERTY"); break;
        case PADDING_LEFT_PROPERTY: printf("PADDING_LEFT_PROPERTY"); break;
        case PADDING_RIGHT_PROPERTY: printf("PADDING_RIGHT_PROPERTY"); break;
        case IMAGE_SOURCE_PROPERTY: printf("IMAGE_SOURCE_PROPERTY"); break;

        // Tag Tokens
        case WINDOW_TAG: printf("WINDOW_TAG"); break;
        case BOX_TAG: printf("BOX_TAG"); break;
        case BUTTON_TAG: printf("BUTTON_TAG"); break;
        case TEXT_INPUT_TAG: printf("TEXT_INPUT_TAG"); break;
        case CANVAS_TAG: printf("CANVAS_TAG"); break;
        case IMAGE_TAG: printf("IMAGE_TAG"); break;
        case TABLE_TAG: printf("TABLE_TAG"); break;
        case TABLE_HEAD_TAG: printf("TABLE_HEAD_TAG"); break;
        case ROW_TAG: printf("ROW_TAG"); break;

        // XML Control Tokens
        case OPEN_TAG: printf("OPEN_TAG"); break;
        case CLOSE_TAG: printf("CLOSE_TAG"); break;
        case OPEN_END_TAG: printf("OPEN_END_TAG"); break;
        case CLOSE_END_TAG: printf("CLOSE_END_TAG"); break;

        case PROPERTY_ASSIGNMENT: printf("PROPERTY_ASSIGNMENT"); break;
        case PROPERTY_VALUE: printf("PROPERTY_VALUE"); break;
        case TEXT_CONTENT: printf("TEXT_CONTENT"); break;

        // Undefined
        case UNDEFINED_TAG: printf("UNDEFINED_TAG"); break;
        case UNDEFINED_PROPERTY: printf("UNDEFINED_PROPERTY"); break;
        case UNDEFINED: printf("UNDEFINED"); break;

        default: printf("UNKNOWN_TOKEN"); break;
    }
}

typedef struct Text_Ref
{
    uint32_t NU_Token_index;
    uint32_t src_index;
    uint8_t char_count;
} Text_Ref;

enum NU_XML_TOKEN NU_Word_To_Token(char* word, uint8_t wordLen)
{
    word[wordLen] = '\0';
    for (uint8_t i=0; i<NU_XML_KEYWORD_COUNT; i++) {
        if (strcmp(word, nu_xml_keywords[i]) == 0) {
            return i;
        }
    }
    return UNDEFINED;
}

enum NU_XML_TOKEN NU_Word_To_Tag_Token(char* word, uint8_t wordLen)
{
    word[wordLen] = '\0';
    for (uint8_t i=NU_XML_PROPERTY_COUNT; i<NU_XML_PROPERTY_COUNT + NU_XML_TAG_COUNT; i++) {
        if (strcmp(word, nu_xml_keywords[i]) == 0) {
            return i;
        }
    }
    return UNDEFINED_TAG;
}

enum NU_XML_TOKEN NU_Word_To_Property_Token(char* word, uint8_t wordLen)
{
    word[wordLen] = '\0';
    for (uint8_t i=0; i<NU_XML_PROPERTY_COUNT; i++) {
        if (strcmp(word, nu_xml_keywords[i]) == 0) {
            return i;
        }
    }
    return UNDEFINED_PROPERTY;
}

NodeType NU_TokenToNodeType(enum NU_XML_TOKEN NU_XML_TOKEN)
{
    int tag_candidate = NU_XML_TOKEN - NU_XML_PROPERTY_COUNT;
    if (tag_candidate < 0 || tag_candidate > 8) return NU_NAT;
    return tag_candidate;
}

int NU_Is_Token_Property(enum NU_XML_TOKEN NU_XML_TOKEN)
{
    return NU_XML_TOKEN < NU_XML_PROPERTY_COUNT || NU_XML_TOKEN == UNDEFINED_PROPERTY;
}
