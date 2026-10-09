#pragma once

#define STYLE_PROPERTY_FIRST STYLE_LAYOUT_DIRECTION_PROPERTY
#define STYLE_PROPERTY_LAST  STYLE_SCROLL_THUMB_MIN_SIZE

#define STYLE_TAG_SELECTOR_FIRST STYLE_WINDOW_SELECTOR
#define STYLE_TAG_SELECTOR_LAST  STYLE_FRAME_SELECTOR

#define STYLE_SPECIAL_SELECTOR_FIRST STYLE_VAR_SELECTOR
#define STYLE_SPECIAL_SELECTOR_LAST  STYLE_SCROLL_TRACK_SELECTOR

#define STYLE_PSEUDO_FIRST STYLE_HOVER_PSEUDO
#define STYLE_PSEUDO_LAST  STYLE_FOCUS_PSEUDO

static const char* style_keywords[] = {
    "dir", "grow",
    "overflow-v", "overflow-h",
    "position", "hide",
    "ignore-mouse", "gap",
    "width", "min-width", "max-width",
    "height", "min-height", "max-height",
    "align-h", "align-v", "text-align-h", "text-align-v",
    "left", "right", "top", "bottom",
    "background", "border-colour", "text-colour",
    "border", "border-top", "border-bottom", "border-left", "border-right",
    "border-radius",
    "border-radius-top-left", "border-radius-top-right", "border-radius-bottom-left", "border-radius-bottom-right",
    "padding", "padding-top", "padding-bottom", "padding-left", "padding-right",
    "image-src", "image-repeat", "image-filter",
    "input-type",
    "font", "src", "size", "weight",
    "scrollbar-overlay", "thumb-min-size",
    "window", "box", "button",
    "input", "canvas", "image",
    "table", "thead", "row",
    "frame",
    "@var", "@font", "@default", "@screen",
    "@scrollbar", "@scrollbar-thumb", "@scrollbar-track",
    "hover", "press", "focus",
};

typedef enum StylesheetToken
{
    // --- Properties ---
    STYLE_LAYOUT_DIRECTION_PROPERTY,
    STYLE_GROW_PROPERTY,
    STYLE_OVERFLOW_V_PROPERTY,
    STYLE_OVERFLOW_H_PROPERTY,
    STYLE_POSITION_PROPERTY,
    STYLE_HIDE_PROPERTY,
    STYLE_IGNORE_MOUSE_PROPERTY,
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
    STYLE_IMAGE_REPEAT_PROPERTY,
    STYLE_IMAGE_FILTER_PROPERTY,
    STYLE_INPUT_TYPE_PROPERTY,
    STYLE_FONT_PROPERTY,
    STYLE_FONT_SRC,
    STYLE_FONT_SIZE,
    STYLE_FONT_WEIGHT,
    STYLE_SCROLLBAR_OVERLAY,
    STYLE_SCROLL_THUMB_MIN_SIZE,

    // --- Tag selectors ---
    STYLE_WINDOW_SELECTOR,
    STYLE_BOX_SELECTOR,
    STYLE_BUTTON_SELECTOR,
    STYLE_INPUT_SELECTOR,
    STYLE_CANVAS_SELECTOR,
    STYLE_IMAGE_SELECTOR,
    STYLE_TABLE_SELECTOR,
    STYLE_THEAD_SELECTOR,
    STYLE_ROW_SELECTOR,
    STYLE_FRAME_SELECTOR,

    // --- Special selectors ---
    STYLE_VAR_SELECTOR,
    STYLE_FONT_CREATION_SELECTOR,
    STYLE_DEFAULT_SELECTOR,
    STYLE_SCREEN_SELECTOR,
    STYLE_SCROLLBAR_SELECTOR,
    STYLE_SCROLL_THUMB_SELECTOR,
    STYLE_SCROLL_TRACK_SELECTOR,

    // --- Pseudos ---
    STYLE_HOVER_PSEUDO,
    STYLE_PRESS_PSEUDO,
    STYLE_FOCUS_PSEUDO,

    // --- Other tokens ---
    STYLE_ID_SELECTOR,
    STYLE_CLASS_SELECTOR,

    STYLE_PSEUDO_COLON,
    STYLE_SELECTOR_COMMA,
    STYLE_SELECTOR_OPEN_BRACE,
    STYLE_SELECTOR_CLOSE_BRACE,
    STYLE_GREATER,
    STYLE_LESS,
    STYLE_GREATER_EQUAL,
    STYLE_LESS_EQUAL,
    STYLE_PROPERTY_ASSIGNMENT,
    STYLE_PROPERTY_VALUE,
    STYLE_VARIABLE_NAME,
    STYLE_VARIABLE_PROPERTY_VALUE,
    STYLE_SCREEN_QUERY_WIDTH,
    STYLE_FONT_CREATION_PROPERTY_VALUE,
    STYLE_FONT_NAME,
    STYLE_UNDEFINED
} StylesheetToken;

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
    STYLESHEET_VARIABLE_DTYPE_LINEAR,
    STYLESHEET_VARIABLE_DTYPE_NEAREST,
    STYLESHEET_VARIABLE_DTYPE_IMAGE,
    STYLESHEET_VARIABLE_DTYPE_ASPECT,
    STYLESHEET_VARIABLE_DTYPE_NONE,
    STYLESHEET_VARIABLE_DTYPE_INPUT_NUMBER,
    STYLESHEET_VARIABLE_DTYPE_UNKNOWN,
} StylesheetVariableDtype;

enum StylesheetTokeniseCtx
{
    CSS_TOKENISE_CTX_GLOBAL,
    CSS_TOKENISE_CTX_GLOBAL_COMMENT,
    CSS_TOKENISE_CTX_SELECTOR,
    CSS_TOKENISE_CTX_SELECTOR_COMMENT,
    CSS_TOKENISE_CTX_PROPERTY_VALUE,
    CSS_TOKENISE_CTX_PROPERTY_STRING_VALUE,
    CSS_TOKENISE_CTX_PROPERTY_VALUE_STRING_ESCAPE_CHAR,
    CSS_TOKENISE_CTX_VARIABLE_NAME_DEFINE,
    CSS_TOKENISE_CTX_CLASS_SELECTOR_NAME,
    CSS_TOKENISE_CTX_ID_SELECTOR_NAME,
    CSS_TOKENISE_CTX_PSEUDO_NAME,
    CSS_TOKENISE_CTX_FONT_CREATION_NAME,
    CSS_TOKENISE_CTX_SCREEN_SELECTOR_NAME, // Eg. @screen > 2000 (after @screen and before the {)
};

enum StylesheetParseCtx
{
    STYLE_PARSE_CTX_SELECTOR,
    STYLE_PARSE_CTX_VAR_SELECTOR,
    STYLE_PARSE_CTX_SCREEN_SELECTOR,
    STYLE_PARSE_CTX_FONT_SELECTOR,
    STYLE_PARSE_CTX_DEFAULT_SELECTOR,
    STYLE_PARSE_CTX_SCROLLBAR_SELECTOR,
    STYLE_PARSE_CTX_SCROLL_THUMB_SELECTOR,
    STYLE_PARSE_CTX_SCROLL_TRACK_SELECTOR,
};

void PrintStyleTokenLine(StylesheetToken token)
{
    const char* name = NULL;
    switch (token)
    {
        // --- Properties ---
        case STYLE_LAYOUT_DIRECTION_PROPERTY:      name = "STYLE_LAYOUT_DIRECTION_PROPERTY"; break;
        case STYLE_GROW_PROPERTY:                  name = "STYLE_GROW_PROPERTY"; break;
        case STYLE_OVERFLOW_V_PROPERTY:            name = "STYLE_OVERFLOW_V_PROPERTY"; break;
        case STYLE_OVERFLOW_H_PROPERTY:            name = "STYLE_OVERFLOW_H_PROPERTY"; break;
        case STYLE_POSITION_PROPERTY:              name = "STYLE_POSITION_PROPERTY"; break;
        case STYLE_HIDE_PROPERTY:                  name = "STYLE_HIDE_PROPERTY"; break;
        case STYLE_IGNORE_MOUSE_PROPERTY:          name = "STYLE_IGNORE_MOUSE_PROPERTY"; break;
        case STYLE_GAP_PROPERTY:                   name = "STYLE_GAP_PROPERTY"; break;
        case STYLE_WIDTH_PROPERTY:                 name = "STYLE_WIDTH_PROPERTY"; break;
        case STYLE_MIN_WIDTH_PROPERTY:             name = "STYLE_MIN_WIDTH_PROPERTY"; break;
        case STYLE_MAX_WIDTH_PROPERTY:             name = "STYLE_MAX_WIDTH_PROPERTY"; break;
        case STYLE_HEIGHT_PROPERTY:                name = "STYLE_HEIGHT_PROPERTY"; break;
        case STYLE_MIN_HEIGHT_PROPERTY:            name = "STYLE_MIN_HEIGHT_PROPERTY"; break;
        case STYLE_MAX_HEIGHT_PROPERTY:            name = "STYLE_MAX_HEIGHT_PROPERTY"; break;
        case STYLE_ALIGN_H_PROPERTY:               name = "STYLE_ALIGN_H_PROPERTY"; break;
        case STYLE_ALIGN_V_PROPERTY:               name = "STYLE_ALIGN_V_PROPERTY"; break;
        case STYLE_TEXT_ALIGN_H_PROPERTY:          name = "STYLE_TEXT_ALIGN_H_PROPERTY"; break;
        case STYLE_TEXT_ALIGN_V_PROPERTY:          name = "STYLE_TEXT_ALIGN_V_PROPERTY"; break;
        case STYLE_LEFT_PROPERTY:                  name = "STYLE_LEFT_PROPERTY"; break;
        case STYLE_RIGHT_PROPERTY:                 name = "STYLE_RIGHT_PROPERTY"; break;
        case STYLE_TOP_PROPERTY:                   name = "STYLE_TOP_PROPERTY"; break;
        case STYLE_BOTTOM_PROPERTY:                name = "STYLE_BOTTOM_PROPERTY"; break;
        case STYLE_BACKGROUND_COLOUR_PROPERTY:     name = "STYLE_BACKGROUND_COLOUR_PROPERTY"; break;
        case STYLE_BORDER_COLOUR_PROPERTY:         name = "STYLE_BORDER_COLOUR_PROPERTY"; break;
        case STYLE_TEXT_COLOUR_PROPERTY:           name = "STYLE_TEXT_COLOUR_PROPERTY"; break;
        case STYLE_BORDER_WIDTH_PROPERTY:          name = "STYLE_BORDER_WIDTH_PROPERTY"; break;
        case STYLE_BORDER_TOP_WIDTH_PROPERTY:      name = "STYLE_BORDER_TOP_WIDTH_PROPERTY"; break;
        case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:   name = "STYLE_BORDER_BOTTOM_WIDTH_PROPERTY"; break;
        case STYLE_BORDER_LEFT_WIDTH_PROPERTY:     name = "STYLE_BORDER_LEFT_WIDTH_PROPERTY"; break;
        case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:    name = "STYLE_BORDER_RIGHT_WIDTH_PROPERTY"; break;
        case STYLE_BORDER_RADIUS_PROPERTY:         name = "STYLE_BORDER_RADIUS_PROPERTY"; break;
        case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:    name = "STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY"; break;
        case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:   name = "STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY"; break;
        case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY: name = "STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY"; break;
        case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:name = "STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY"; break;
        case STYLE_PADDING_PROPERTY:               name = "STYLE_PADDING_PROPERTY"; break;
        case STYLE_PADDING_TOP_PROPERTY:           name = "STYLE_PADDING_TOP_PROPERTY"; break;
        case STYLE_PADDING_BOTTOM_PROPERTY:        name = "STYLE_PADDING_BOTTOM_PROPERTY"; break;
        case STYLE_PADDING_LEFT_PROPERTY:          name = "STYLE_PADDING_LEFT_PROPERTY"; break;
        case STYLE_PADDING_RIGHT_PROPERTY:         name = "STYLE_PADDING_RIGHT_PROPERTY"; break;
        case STYLE_IMAGE_SOURCE_PROPERTY:          name = "STYLE_IMAGE_SOURCE_PROPERTY"; break;
        case STYLE_INPUT_TYPE_PROPERTY:            name = "STYLE_INPUT_TYPE_PROPERTY"; break;
        case STYLE_FONT_PROPERTY:                  name = "STYLE_FONT_PROPERTY"; break;
        case STYLE_FONT_SRC:                       name = "STYLE_FONT_SRC"; break;
        case STYLE_FONT_SIZE:                      name = "STYLE_FONT_SIZE"; break;
        case STYLE_FONT_WEIGHT:                    name = "STYLE_FONT_WEIGHT"; break;

        // --- Tag selectors ---
        case STYLE_WINDOW_SELECTOR: name = "STYLE_WINDOW_SELECTOR"; break;
        case STYLE_BOX_SELECTOR:    name = "STYLE_BOX_SELECTOR"; break;
        case STYLE_BUTTON_SELECTOR: name = "STYLE_BUTTON_SELECTOR"; break;
        case STYLE_INPUT_SELECTOR:  name = "STYLE_INPUT_SELECTOR"; break;
        case STYLE_CANVAS_SELECTOR: name = "STYLE_CANVAS_SELECTOR"; break;
        case STYLE_IMAGE_SELECTOR:  name = "STYLE_IMAGE_SELECTOR"; break;
        case STYLE_TABLE_SELECTOR:  name = "STYLE_TABLE_SELECTOR"; break;
        case STYLE_THEAD_SELECTOR:  name = "STYLE_THEAD_SELECTOR"; break;
        case STYLE_ROW_SELECTOR:    name = "STYLE_ROW_SELECTOR"; break;

        // --- Special selectors ---
        case STYLE_VAR_SELECTOR:           name = "STYLE_VAR_SELECTOR"; break;
        case STYLE_SCREEN_SELECTOR:        name = "STYLE_SCREEN_SELECTOR"; break;
        case STYLE_FONT_CREATION_SELECTOR: name = "STYLE_FONT_CREATION_SELECTOR"; break;
        case STYLE_DEFAULT_SELECTOR:       name = "STYLE_DEFAULT_SELECTOR"; break;

        // --- Pseudos ---
        case STYLE_HOVER_PSEUDO:  name = "STYLE_HOVER_PSEUDO"; break;
        case STYLE_PRESS_PSEUDO:  name = "STYLE_PRESS_PSEUDO"; break;
        case STYLE_FOCUS_PSEUDO:  name = "STYLE_FOCUS_PSEUDO"; break;

        // --- Other tokens ---
        case STYLE_ID_SELECTOR:               name = "STYLE_ID_SELECTOR"; break;
        case STYLE_CLASS_SELECTOR:            name = "STYLE_CLASS_SELECTOR"; break;
        case STYLE_PSEUDO_COLON:              name = "STYLE_PSEUDO_COLON"; break;
        case STYLE_SELECTOR_COMMA:            name = "STYLE_SELECTOR_COMMA"; break;
        case STYLE_SELECTOR_OPEN_BRACE:       name = "STYLE_SELECTOR_OPEN_BRACE"; break;
        case STYLE_SELECTOR_CLOSE_BRACE:      name = "STYLE_SELECTOR_CLOSE_BRACE"; break;
        case STYLE_PROPERTY_ASSIGNMENT:       name = "STYLE_PROPERTY_ASSIGNMENT"; break;
        case STYLE_PROPERTY_VALUE:            name = "STYLE_PROPERTY_VALUE"; break;
        case STYLE_VARIABLE_NAME:             name = "STYLE_VARIABLE_NAME"; break;
        case STYLE_FONT_CREATION_PROPERTY_VALUE:name = "STYLE_FONT_CREATION_PROPERTY_VALUE"; break;
        case STYLE_FONT_NAME:                  name = "STYLE_FONT_NAME"; break;

        default: name = "STYLE_UNDEFINED"; break;
    }

    printf("%s\n", name);
}

static inline u32 Property_Token_To_Flag(StylesheetToken token)
{
    return (1u << token);
}

static StylesheetToken NU_Word_To_Style_Token(char* word, u8 len)
{
    word[len] = '\0';
    for (int i = STYLE_PROPERTY_FIRST; i <= STYLE_PSEUDO_LAST; i++) {
        if (strcmp(word, style_keywords[i]) == 0) return (StylesheetToken)i;
    }
    return STYLE_UNDEFINED;
}

static StylesheetToken NU_Word_To_Style_Property_Token(char* word, u8 len)
{
    word[len] = '\0';
    for (int i = STYLE_PROPERTY_FIRST; i <= STYLE_PROPERTY_LAST; i++) {
        if (strcmp(word, style_keywords[i]) == 0) return (StylesheetToken)i;
    }
    return STYLE_UNDEFINED;
}

static StylesheetToken NU_Word_To_Tag_Selector_Token(char* word, u8 len)
{
    word[len] = '\0';
    for (int i = STYLE_TAG_SELECTOR_FIRST; i <= STYLE_TAG_SELECTOR_LAST; i++) {
        if (strcmp(word, style_keywords[i]) == 0) return (StylesheetToken)i;
    }
    return STYLE_UNDEFINED;
}

static StylesheetToken NU_Word_To_Any_Selector_Token(char* word, u8 len)
{
    word[len] = '\0';
    for (int i = STYLE_TAG_SELECTOR_FIRST; i <= STYLE_SPECIAL_SELECTOR_LAST; i++) {
        if (strcmp(word, style_keywords[i]) == 0) return (StylesheetToken)i;
    }
    return STYLE_UNDEFINED;
}

static StylesheetToken NU_Word_To_Pseudo_Token(char* word, u8 len)
{
    word[len] = '\0';
    for (int i = STYLE_PSEUDO_FIRST; i <= STYLE_PSEUDO_LAST; i++) {
        if (strcmp(word, style_keywords[i]) == 0) return (StylesheetToken)i;
    }
    return STYLE_UNDEFINED;
}

static StylePseudoClass Token_To_Pseudo_Class(StylesheetToken token)
{
    return token - STYLE_PSEUDO_FIRST;
}

static int NU_Token_To_Tag(StylesheetToken token)
{
    return token - STYLE_TAG_SELECTOR_FIRST;
}

static inline int Stylesheet_IsPropertyIdentifierToken(StylesheetToken token)
{
    return token >= STYLE_PROPERTY_FIRST && token <= STYLE_PROPERTY_LAST;
}

static inline int Stylesheet_IsTagSelectorToken(StylesheetToken token)
{
    return token >= STYLE_TAG_SELECTOR_FIRST && token <= STYLE_TAG_SELECTOR_LAST;
}

static inline int NU_Is_Pseudo_Token(StylesheetToken token)
{
    return token >= STYLE_PSEUDO_FIRST && token <= STYLE_PSEUDO_LAST;
}
