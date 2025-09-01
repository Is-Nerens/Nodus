#pragma once

#define PROPERTY_COUNT 28
#define KEYWORD_COUNT 34

static const char* keywords[] = {
    "dir",
    "grow",
    "overflowV",
    "overflowH",
    "width",
    "minWidth",
    "maxWidth",
    "height", 
    "minHeight",
    "maxHeight",
    "alignH",
    "alignV",
    "background",
    "borderColour",
    "border", "borderTop", "borderBottom", "borderLeft", "borderRight",
    "borderRadius", "borderTopLeftRadius", "borderTopRightRadius", "borderBottomLeftRadius", "borderBottomRightRadius",
    "pad", "padTop", "padBottom", "padLeft", "padRight" "gap",
    "window", "rect", "button", "grid", "text", "image"
};
static const uint8_t keyword_lengths[] = { 
    3, 4, 9, 9, 5, 8, 8, 6, 9, 9, 6, 6, 10, 12,
    6, 9, 12, 10, 11,      // border width
    12, 19, 20, 22, 23,    // border radius
    3, 6, 9, 7, 11, 3,     // padding
    6, 4, 6, 4, 4, 5       // selectors
};
static enum NU_Style_Token 
{
LAYOUT_DIRECTION_PROPERTY,
    GROW_PROPERTY,
    OVERFLOW_V_PROPERTY,
    OVERFLOW_H_PROPERTY,
    WIDTH_PROPERTY,
    MIN_WIDTH_PROPERTY,
    MAX_WIDTH_PROPERTY,
    HEIGHT_PROPERTY,
    MIN_HEIGHT_PROPERTY,
    MAX_HEIGHT_PROPERTY,
    ALIGN_H_PROPERTY,
    ALIGN_V_PROPERTY,
    BACKGROUND_COLOUR_PROPERTY,
    BORDER_COLOUR_PROPERTY,
    BORDER_WIDTH_PROPERTY,
    BORDER_TOP_WIDTH_PROPERTY,
    BORDER_BOTTOM_WIDTH_PROPERTY,
    BORDER_LEFT_WIDTH_PROPERTY,
    BORDER_RIGHT_WIDTH_PROPERTY,
    BORDER_RADIUS_PROPERTY,
    BORDER_TOP_LEFT_RADIUS_PROPERTY,
    BORDER_TOP_RIGHT_RADIUS_PROPERTY,
    BORDER_BOTTOM_LEFT_RADIUS_PROPERTY,
    BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY,
    PADDING_PROPERTY,
    PADDING_TOP_PROPERTY,
    PADDING_BOTTOM_PROPERTY,
    PADDING_LEFT_PROPERTY,
    PADDING_RIGHT_PROPERTY,
    GAP_PROPERTY,
    WINDOW_SELECTOR,
    RECT_SELECTOR,
    BUTTON_SELECTOR,
    GRID_SELECTOR,
    TEXT_SELECTOR,
    IMAGE_SELECTOR,
    ID_SELECTOR,
    CLASS_SELECTOR
}