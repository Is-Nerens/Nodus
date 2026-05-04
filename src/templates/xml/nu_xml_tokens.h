#pragma once 

#define XML_PROPERTY_FIRST ID_PROPERTY
#define XML_PROPERTY_LAST IMPORT_SRC_PROPERTY

#define XML_TAG_FIRST WINDOW_TAG
#define XML_TAG_LAST IMPORT_TAG

const char* nu_xml_keywords[] = {
    "id", "class",
    "dir", "grow", "overflow-v", "overflow-h", "position", "hide", "ignore-mouse", "gap",
    "width", "min-width", "max-width", "height", "min-height", "max-height",
    "align-h", "align-v", "text-align-h", "text-align-v",
    "left", "right", "top", "bottom",
    "background", "border-colour", "text-colour",
    "border", "border-top", "border-bottom", "border-left", "border-right",
    "border-radius", "border-radius-top-left", "border-radius-top-right", "border-radius-bottom-left", "border-radius-bottom-right",
    "padding", "padding-top", "padding-bottom", "padding-left", "padding-right",
    "image-src",
    "input-type",
    "src",

    "window", "box", "button",
    "input", "canvas", "image",
    "table", "thead", "row",
    "frame", "import"
};
typedef enum NU_XML_TOKEN
{   
    // -------------------------------------------------------------
    // --- These enums positionally correspond to keywords above ---
    // -------------------------------------------------------------

    // Properties
    ID_PROPERTY, CLASS_PROPERTY, 
    LAYOUT_DIRECTION_PROPERTY, GROW_PROPERTY,
    OVERFLOW_V_PROPERTY, OVERFLOW_H_PROPERTY,
    POSITION_PROPERTY,
    HIDE_PROPERTY,
    IGNORE_MOUSE_PROPERTY,
    GAP_PROPERTY,
    WIDTH_PROPERTY, MIN_WIDTH_PROPERTY, MAX_WIDTH_PROPERTY,
    HEIGHT_PROPERTY, MIN_HEIGHT_PROPERTY, MAX_HEIGHT_PROPERTY,
    ALIGN_H_PROPERTY, ALIGN_V_PROPERTY, TEXT_ALIGN_H_PROPERTY, TEXT_ALIGN_V_PROPERTY,
    LEFT_PROPERTY, RIGHT_PROPERTY, TOP_PROPERTY, BOTTOM_PROPERTY,
    BACKGROUND_COLOUR_PROPERTY, BORDER_COLOUR_PROPERTY, TEXT_COLOUR_PROPERTY,
    BORDER_WIDTH_PROPERTY, BORDER_TOP_WIDTH_PROPERTY, BORDER_BOTTOM_WIDTH_PROPERTY, BORDER_LEFT_WIDTH_PROPERTY, BORDER_RIGHT_WIDTH_PROPERTY,
    BORDER_RADIUS_PROPERTY, BORDER_TOP_LEFT_RADIUS_PROPERTY, BORDER_TOP_RIGHT_RADIUS_PROPERTY, BORDER_BOTTOM_LEFT_RADIUS_PROPERTY, BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY,
    PADDING_PROPERTY, PADDING_TOP_PROPERTY, PADDING_BOTTOM_PROPERTY, PADDING_LEFT_PROPERTY, PADDING_RIGHT_PROPERTY,
    IMAGE_SOURCE_PROPERTY, INPUT_TYPE_PROPERTY, IMPORT_SRC_PROPERTY,

    // Tags
    WINDOW_TAG,
    BOX_TAG,
    BUTTON_TAG,
    TEXT_INPUT_TAG,
    CANVAS_TAG,
    IMAGE_TAG,
    TABLE_TAG,
    TABLE_HEAD_TAG,
    ROW_TAG,
    FRAME_TAG,
    IMPORT_TAG,

    // -----------------------------------------------------
    // --- These enums do not correspond to any keywords ---
    // -----------------------------------------------------
    // Misc
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
    u32 NU_Token_index;
    u32 src_index;
    u8 char_count;
} Text_Ref;

NU_XML_TOKEN NU_Word_To_Token(char* word, u8 wordLen)
{
    word[wordLen] = '\0';
    for (int i = XML_PROPERTY_FIRST; i<= XML_TAG_LAST; i++) {
        if (stringEquals(word, nu_xml_keywords[i])) return (enum NU_XML_TOKEN)i;
    }
    return UNDEFINED;
}

NU_XML_TOKEN NU_Word_To_Tag_Token(char* word, u8 wordLen)
{
    word[wordLen] = '\0';
    for (int i = XML_TAG_FIRST; i<= XML_TAG_LAST; i++) {
        if (stringEquals(word, nu_xml_keywords[i])) return (enum NU_XML_TOKEN)i;
    }
    return UNDEFINED_TAG;
}

NU_XML_TOKEN NU_Word_To_Property_Token(char* word, u8 wordLen)
{
    word[wordLen] = '\0';
    for (int i = XML_PROPERTY_FIRST; i<= XML_PROPERTY_LAST; i++) {
        if (stringEquals(word, nu_xml_keywords[i])) return (enum NU_XML_TOKEN)i;
    }
    return UNDEFINED_PROPERTY;
}

NodeType NU_TokenToNodeType(NU_XML_TOKEN token)
{
    if (token < XML_TAG_FIRST || token > XML_TAG_LAST) return NU_NAT;
    return token - XML_TAG_FIRST;
}

int NU_Is_Token_Property(NU_XML_TOKEN t)
{
    return (t >= XML_PROPERTY_FIRST && t <= XML_PROPERTY_LAST) || t == UNDEFINED_PROPERTY; 
}