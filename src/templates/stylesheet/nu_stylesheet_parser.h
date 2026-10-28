#pragma once
#include "nu_stylesheet_grammar_assertions.h"
#include "../nu_token_array.h"
#include "nu_stylesheet_structs.h"
#include "nu_stylesheet_tokens.h"

typedef struct Stylesheet_FontLoadJob
{
    Stylesheet* stylesheet;
    int fontIdx;
    int fontSize;
    int fontWeight;
    String filepath;
} Stylesheet_FontLoadJob;

typedef struct Stylesheet_FontLoaderJobBatch
{
    Stylesheet_FontLoadJob* jobs;
    int start;
    int end;
} Stylesheet_FontLoaderJobBatch;

int Stylesheet_FontLoaderThread(void* data)
{
    Stylesheet_FontLoadJob* job = (Stylesheet_FontLoadJob*)data;

    bool subpixelRendering = true;
    #ifdef PLATFORM_MACOS
        subpixelRendering = false;
    #endif

    Font* font = Array_Get(&job->stylesheet->fonts, job->fontIdx);
    Font_Create(font, StringCstr(job->filepath), job->fontSize, job->fontWeight, subpixelRendering);
    return 0;
}

void Stylesheet_OverwriteStyleItem(StyleItem* item, StyleItem* overwriter)
{
    item->propertyFlags |= overwriter->propertyFlags;

    // Overwrite layout flags
    if (overwriter->propertyFlags & PROPERTY_FLAG_LAYOUT_DIR)        item->layoutFlags = (item->layoutFlags & ~LAYOUT_VERTICAL)                 | (overwriter->layoutFlags & LAYOUT_VERTICAL);
    if (overwriter->propertyFlags & PROPERTY_FLAG_GROW)              item->layoutFlags = (item->layoutFlags & ~(GROW_HORIZONTAL|GROW_VERTICAL)) | (overwriter->layoutFlags & (GROW_HORIZONTAL|GROW_VERTICAL));
    if (overwriter->propertyFlags & PROPERTY_FLAG_VERTICAL_SCROLL)   item->layoutFlags = (item->layoutFlags & ~OVERFLOW_VERTICAL_SCROLL)        | (overwriter->layoutFlags & OVERFLOW_VERTICAL_SCROLL);
    if (overwriter->propertyFlags & PROPERTY_FLAG_HORIZONTAL_SCROLL) item->layoutFlags = (item->layoutFlags & ~OVERFLOW_HORIZONTAL_SCROLL)      | (overwriter->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL);
    if (overwriter->propertyFlags & PROPERTY_FLAG_POSITION_ABSOLUTE) item->layoutFlags = (item->layoutFlags & ~POSITION_ABSOLUTE)               | (overwriter->layoutFlags & POSITION_ABSOLUTE);
    if (overwriter->propertyFlags & PROPERTY_FLAG_HIDDEN)            item->layoutFlags = (item->layoutFlags & ~HIDDEN)                          | (overwriter->layoutFlags & HIDDEN);
    if (overwriter->propertyFlags & PROPERTY_FLAG_IGNORE_MOUSE)      item->layoutFlags = (item->layoutFlags & ~IGNORE_MOUSE)                    | (overwriter->layoutFlags & IGNORE_MOUSE);
    if (overwriter->propertyFlags & PROPERTY_FLAG_HIDE_BACKGROUND)   item->layoutFlags = (item->layoutFlags & ~HIDE_BACKGROUND)                 | (overwriter->layoutFlags & HIDE_BACKGROUND);

    // Overwrite gap and size fields (branchless)
    item->gap             = item->gap             * !(overwriter->propertyFlags & PROPERTY_FLAG_GAP)              + overwriter->gap             * !!(overwriter->propertyFlags & PROPERTY_FLAG_GAP);
    item->preferredWidth  = item->preferredWidth  * !(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_WIDTH)  + overwriter->preferredWidth  * !!(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_WIDTH);
    item->minWidth        = item->minWidth        * !(overwriter->propertyFlags & PROPERTY_FLAG_MIN_WIDTH)        + overwriter->minWidth        * !!(overwriter->propertyFlags & PROPERTY_FLAG_MIN_WIDTH);
    item->maxWidth        = item->maxWidth        * !(overwriter->propertyFlags & PROPERTY_FLAG_MAX_WIDTH)        + overwriter->maxWidth        * !!(overwriter->propertyFlags & PROPERTY_FLAG_MAX_WIDTH);
    item->preferredHeight = item->preferredHeight * !(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_HEIGHT) + overwriter->preferredHeight * !!(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_HEIGHT);
    item->minHeight       = item->minHeight       * !(overwriter->propertyFlags & PROPERTY_FLAG_MIN_HEIGHT)       + overwriter->minHeight       * !!(overwriter->propertyFlags & PROPERTY_FLAG_MIN_HEIGHT);
    item->maxHeight       = item->maxHeight       * !(overwriter->propertyFlags & PROPERTY_FLAG_MAX_HEIGHT)       + overwriter->maxHeight       * !!(overwriter->propertyFlags & PROPERTY_FLAG_MAX_HEIGHT);

    // Overwrite alignments (branchless)
    item->horizontalAlignment     = item->horizontalAlignment     * !(overwriter->propertyFlags & PROPERTY_FLAG_ALIGN_H)      + overwriter->horizontalAlignment     * !!(overwriter->propertyFlags & PROPERTY_FLAG_ALIGN_H);
    item->verticalAlignment       = item->verticalAlignment       * !(overwriter->propertyFlags & PROPERTY_FLAG_ALIGN_V)      + overwriter->verticalAlignment       * !!(overwriter->propertyFlags & PROPERTY_FLAG_ALIGN_V);
    item->horizontalTextAlignment = item->horizontalTextAlignment * !(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_ALIGN_H) + overwriter->horizontalTextAlignment * !!(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_ALIGN_H);
    item->verticalTextAlignment   = item->verticalTextAlignment   * !(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_ALIGN_V) + overwriter->verticalTextAlignment   * !!(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_ALIGN_V);

    // Overwrite absolute distances (branchless)
    item->left   = item->left   * !(overwriter->propertyFlags & PROPERTY_FLAG_LEFT)   + overwriter->left   * !!(overwriter->propertyFlags & PROPERTY_FLAG_LEFT);
    item->right  = item->right  * !(overwriter->propertyFlags & PROPERTY_FLAG_RIGHT)  + overwriter->right  * !!(overwriter->propertyFlags & PROPERTY_FLAG_RIGHT);
    item->top    = item->top    * !(overwriter->propertyFlags & PROPERTY_FLAG_TOP)    + overwriter->top    * !!(overwriter->propertyFlags & PROPERTY_FLAG_TOP);
    item->bottom = item->bottom * !(overwriter->propertyFlags & PROPERTY_FLAG_BOTTOM) + overwriter->bottom * !!(overwriter->propertyFlags & PROPERTY_FLAG_BOTTOM);

    // Branchless RGB overwrite
    item->backgroundR = item->backgroundR * !(overwriter->propertyFlags & PROPERTY_FLAG_BACKGROUND)    + overwriter->backgroundR * !!(overwriter->propertyFlags & PROPERTY_FLAG_BACKGROUND);
    item->backgroundG = item->backgroundG * !(overwriter->propertyFlags & PROPERTY_FLAG_BACKGROUND)    + overwriter->backgroundG * !!(overwriter->propertyFlags & PROPERTY_FLAG_BACKGROUND);
    item->backgroundB = item->backgroundB * !(overwriter->propertyFlags & PROPERTY_FLAG_BACKGROUND)    + overwriter->backgroundB * !!(overwriter->propertyFlags & PROPERTY_FLAG_BACKGROUND);
    item->borderR     = item->borderR     * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_COLOUR) + overwriter->borderR     * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_COLOUR);
    item->borderG     = item->borderG     * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_COLOUR) + overwriter->borderG     * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_COLOUR);
    item->borderB     = item->borderB     * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_COLOUR) + overwriter->borderB     * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_COLOUR);
    item->textR       = item->textR       * !(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_COLOUR)   + overwriter->textR       * !!(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_COLOUR);
    item->textG       = item->textG       * !(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_COLOUR)   + overwriter->textG       * !!(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_COLOUR);
    item->textB       = item->textB       * !(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_COLOUR)   + overwriter->textB       * !!(overwriter->propertyFlags & PROPERTY_FLAG_TEXT_COLOUR);

    // Overwrite border widths (branchless)
    item->borderTop    = item->borderTop    * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_TOP)    + overwriter->borderTop    * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_TOP);
    item->borderBottom = item->borderBottom * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_BOTTOM) + overwriter->borderBottom * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_BOTTOM);
    item->borderLeft   = item->borderLeft   * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_LEFT)   + overwriter->borderLeft   * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_LEFT);
    item->borderRight  = item->borderRight  * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RIGHT)  + overwriter->borderRight  * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RIGHT);

    // Overwrite border radii (branchless)
    item->borderRadiusTl = item->borderRadiusTl * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_TL) + overwriter->borderRadiusTl * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_TL);
    item->borderRadiusTr = item->borderRadiusTr * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_TR) + overwriter->borderRadiusTr * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_TR);
    item->borderRadiusBl = item->borderRadiusBl * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_BL) + overwriter->borderRadiusBl * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_BL);
    item->borderRadiusBr = item->borderRadiusBr * !(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_BR) + overwriter->borderRadiusBr * !!(overwriter->propertyFlags & PROPERTY_FLAG_BORDER_RADIUS_BR);

    // Overwrite padding (branchless)
    item->padTop    = item->padTop    * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_TOP)    + overwriter->padTop    * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_TOP);
    item->padBottom = item->padBottom * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_BOTTOM) + overwriter->padBottom * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_BOTTOM);
    item->padLeft   = item->padLeft   * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_LEFT)   + overwriter->padLeft   * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_LEFT);
    item->padRight  = item->padRight  * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_RIGHT)  + overwriter->padRight  * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_RIGHT);

    // Overwrite image and input type (branchless)
    item->imageHandle = item->imageHandle * !(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE)        + overwriter->imageHandle * !!(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE);
    item->imageWidthDimType = item->imageWidthDimType * !(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_WIDTH_DIM) + overwriter->imageWidthDimType * !!(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_WIDTH_DIM);
    item->imageHeightDimType = item->imageHeightDimType * !(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_HEIGHT_DIM) + overwriter->imageHeightDimType * !!(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_HEIGHT_DIM);
    item->imageFilter = item->imageFilter * !(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_FILTER) + overwriter->imageFilter * !!(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_FILTER);
    item->imageRepeat = item->imageRepeat * !(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_REPEAT) + overwriter->imageRepeat * !!(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE_REPEAT);
    item->inputType   = item->inputType   * !(overwriter->propertyFlags & PROPERTY_FLAG_INPUT_TYPE)   + overwriter->inputType   * !!(overwriter->propertyFlags & PROPERTY_FLAG_INPUT_TYPE);

    // Overwrite font Id
    item->fontIdx = item->fontIdx * !(overwriter->propertyFlags & PROPERTY_FLAG_FONT) + overwriter->fontIdx  * !!(overwriter->propertyFlags & PROPERTY_FLAG_FONT);
}

u64 Stylesheet_ParseProperty(
    Stylesheet* ss,
    StylesheetToken token,
    StyleItem* item,
    const char* text, int textLen,
    ImageResourceManager* imageResouceManager,
    DisplayManager* displayManager)
{
    u64 propertyFlagsSet = 0;

    switch (token)
    {
        // Set layout direction
        case STYLE_LAYOUT_DIRECTION_PROPERTY:
            switch(text[0])
            {
                case 'v':
                    item->layoutFlags |= LAYOUT_VERTICAL;
                    propertyFlagsSet |= PROPERTY_FLAG_LAYOUT_DIR; break;
                    break;
                case 'h':
                    propertyFlagsSet |= PROPERTY_FLAG_LAYOUT_DIR; break;
            }
            break;

        // Set growth
        case STYLE_GROW_PROPERTY:
            switch(text[0])
            {
                case 'v': item->layoutFlags |= GROW_VERTICAL; break;
                case 'h': item->layoutFlags |= GROW_HORIZONTAL; break;
                case 'b': item->layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL); break;
                default: {
                    if (stringEquals(text, "none")) item->layoutFlags &= ~(GROW_HORIZONTAL | GROW_VERTICAL);
                    break;
                }
            }
            propertyFlagsSet |= PROPERTY_FLAG_GROW;
            break;

        // Set overflow behaviour
        case STYLE_OVERFLOW_V_PROPERTY:
            switch(text[0])
            {
                case 's':
                    item->layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                    propertyFlagsSet |= PROPERTY_FLAG_VERTICAL_SCROLL; break;
                case 'h':
                    propertyFlagsSet |= PROPERTY_FLAG_VERTICAL_SCROLL; break;
            }
            break;

        case STYLE_OVERFLOW_H_PROPERTY:
            switch(text[0])
            {
                case 's':
                    item->layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                    propertyFlagsSet |= PROPERTY_FLAG_HORIZONTAL_SCROLL; break;
                case 'h':
                    propertyFlagsSet |= PROPERTY_FLAG_HORIZONTAL_SCROLL; break;
            }
            break;

        // Relative/Absolute positioning
        case STYLE_POSITION_PROPERTY:
            if (strcmp(text, "absolute") == 0) {
                item->layoutFlags |= POSITION_ABSOLUTE;
                propertyFlagsSet |= PROPERTY_FLAG_POSITION_ABSOLUTE;
            } else if (strcmp(text, "relative") == 0) {
                propertyFlagsSet |= PROPERTY_FLAG_POSITION_ABSOLUTE;
            }
            break;

        // Hide/show
        case STYLE_HIDE_PROPERTY:
            if (strcmp(text, "true") == 0) {
                item->layoutFlags |= HIDDEN;
                propertyFlagsSet |= PROPERTY_FLAG_HIDDEN;
            }
            else if (strcmp(text, "false") == 0) {
                propertyFlagsSet |= PROPERTY_FLAG_HIDDEN;
            }
            break;

        // Ignore mouse
        case STYLE_IGNORE_MOUSE_PROPERTY:
            if (strcmp(text, "true") == 0) {
                item->layoutFlags |= IGNORE_MOUSE;
                propertyFlagsSet |= PROPERTY_FLAG_IGNORE_MOUSE;
            }
            else if (strcmp(text, "false") == 0) {
                propertyFlagsSet |= PROPERTY_FLAG_IGNORE_MOUSE;
            }
            break;

        // Set gap
        case STYLE_GAP_PROPERTY:
            propertyFlagsSet |= (PROPERTY_FLAG_GAP * String_To_u8(&item->gap, text));
            break;

        // Set preferred width
        case STYLE_WIDTH_PROPERTY:
            if (String_To_Uint16(&item->preferredWidth, text)) {
                item->imageWidthDimType = IMAGE_DIM_SPECIFIC;
                propertyFlagsSet |= PROPERTY_FLAG_PREFERRED_WIDTH;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
            }
            else if (strcmp(text, "image") == 0) {
                item->imageWidthDimType = IMAGE_DIM_INTRINSIC;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
            }
            else if (strcmp(text, "aspect") == 0) {
                item->imageWidthDimType = IMAGE_DIM_ASPECT;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
            }
            break;

        // Set min width
        case STYLE_MIN_WIDTH_PROPERTY:
            propertyFlagsSet |= (PROPERTY_FLAG_MIN_WIDTH * String_To_Uint16(&item->minWidth, text));
            break;

        // Set max width
        case STYLE_MAX_WIDTH_PROPERTY:
            propertyFlagsSet |= (PROPERTY_FLAG_MAX_WIDTH * String_To_Uint16(&item->maxWidth, text));
            break;

        // Set preferred height
        case STYLE_HEIGHT_PROPERTY:
            if (String_To_Uint16(&item->preferredHeight, text)) {
                item->imageHeightDimType = IMAGE_DIM_SPECIFIC;
                propertyFlagsSet |= PROPERTY_FLAG_PREFERRED_HEIGHT;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
            }
            else if (strcmp(text, "image") == 0) {
                item->imageHeightDimType = IMAGE_DIM_INTRINSIC;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
            }
            else if (strcmp(text, "aspect") == 0) {
                item->imageHeightDimType = IMAGE_DIM_ASPECT;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
            }
            break;

        // Set min height
        case STYLE_MIN_HEIGHT_PROPERTY:
            propertyFlagsSet |= (PROPERTY_FLAG_MIN_HEIGHT * String_To_Uint16(&item->minHeight, text));
            break;

        // Set max height
        case STYLE_MAX_HEIGHT_PROPERTY:
            propertyFlagsSet |= (PROPERTY_FLAG_MAX_HEIGHT * String_To_Uint16(&item->maxHeight, text));
            break;

        // Set horizontal alignment
        case STYLE_ALIGN_H_PROPERTY:
            if (strcmp(text, "left") == 0) {
                item->horizontalAlignment = 0;
                propertyFlagsSet |= PROPERTY_FLAG_ALIGN_H;
            }
            else if (strcmp(text, "center") == 0) {
                item->horizontalAlignment = 1;
                propertyFlagsSet |= PROPERTY_FLAG_ALIGN_H;
            }
            else if (strcmp(text, "right") == 0) {
                item->horizontalAlignment = 2;
                propertyFlagsSet |= PROPERTY_FLAG_ALIGN_H;
            }
            break;

        // Set vertical alignment
        case STYLE_ALIGN_V_PROPERTY:
            if (strcmp(text, "top") == 0) {
                item->verticalAlignment = 0;
                propertyFlagsSet |= PROPERTY_FLAG_ALIGN_V;
            }
            else if (strcmp(text, "center") == 0) {
                item->verticalAlignment = 1;
                propertyFlagsSet |= PROPERTY_FLAG_ALIGN_V;
            }
            else if (strcmp(text, "bottom") == 0) {
                item->verticalAlignment = 2;
                propertyFlagsSet |= PROPERTY_FLAG_ALIGN_V;
            }
            break;

        // Set horizontal text alignment
        case STYLE_TEXT_ALIGN_H_PROPERTY:
            if (strcmp(text, "left") == 0) {
                item->horizontalTextAlignment = 0;
                propertyFlagsSet |= PROPERTY_FLAG_TEXT_ALIGN_H;
            }
            else if (strcmp(text, "center") == 0) {
                item->horizontalTextAlignment = 1;
                propertyFlagsSet |= PROPERTY_FLAG_TEXT_ALIGN_H;
            }
            else if (strcmp(text, "right") == 0) {
                item->horizontalTextAlignment = 2;
                propertyFlagsSet |= PROPERTY_FLAG_TEXT_ALIGN_H;
            }
            break;

        // Set vertical text alignment
        case STYLE_TEXT_ALIGN_V_PROPERTY:
            if (strcmp(text, "top") == 0) {
                item->verticalTextAlignment = 0;
                propertyFlagsSet |= PROPERTY_FLAG_TEXT_ALIGN_V;
            }
            else if (strcmp(text, "center") == 0) {
                item->verticalTextAlignment = 1;
                propertyFlagsSet |= PROPERTY_FLAG_TEXT_ALIGN_V;
            }
            else if (strcmp(text, "bottom") == 0) {
                item->verticalTextAlignment = 2;
                propertyFlagsSet |= PROPERTY_FLAG_TEXT_ALIGN_V;
            }
            break;

        // Set absolute position properties
        case STYLE_LEFT_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_LEFT * String_To_Int16(&item->left, text);
            break;

        case STYLE_RIGHT_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_RIGHT * String_To_Int16(&item->right, text);
            break;

        case STYLE_TOP_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_TOP * String_To_Int16(&item->top, text);
            break;

        case STYLE_BOTTOM_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BOTTOM * String_To_Int16(&item->bottom, text);
            break;

        // Set background colour
        case STYLE_BACKGROUND_COLOUR_PROPERTY: {
            struct RGB rgb;
            if (Parse_Hexcode(text, textLen, &rgb)) {
                item->backgroundR = rgb.r;
                item->backgroundG = rgb.g;
                item->backgroundB = rgb.b;
                propertyFlagsSet |= PROPERTY_FLAG_BACKGROUND;
                propertyFlagsSet &= ~PROPERTY_FLAG_HIDE_BACKGROUND;
                item->layoutFlags &= ~HIDE_BACKGROUND;
            } else if (strcmp(text, "none") == 0) {
                propertyFlagsSet |= PROPERTY_FLAG_HIDE_BACKGROUND;
                item->layoutFlags |= HIDE_BACKGROUND;
            }
            break;
        }

        // Set border colour
        case STYLE_BORDER_COLOUR_PROPERTY: {
            struct RGB rgb;
            if (Parse_Hexcode(text, textLen, &rgb)) {
                item->borderR = rgb.r;
                item->borderG = rgb.g;
                item->borderB = rgb.b;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_COLOUR;
            }
            break;
        }

        // Set text colour
        case STYLE_TEXT_COLOUR_PROPERTY: {
            struct RGB rgb;
            if (Parse_Hexcode(text, textLen, &rgb)) {
                item->textR = rgb.r;
                item->textG = rgb.g;
                item->textB = rgb.b;
                propertyFlagsSet |= PROPERTY_FLAG_TEXT_COLOUR;
            }
            break;
        }

        // Set border width
        case STYLE_BORDER_WIDTH_PROPERTY: {
            u8 border_width;
            if (String_To_u8(&border_width, text)) {
                item->borderTop = border_width;
                item->borderBottom = border_width;
                item->borderLeft = border_width;
                item->borderRight = border_width;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_TOP;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_BOTTOM;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_LEFT;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_RIGHT;
            }
            break;
        }

        case STYLE_BORDER_TOP_WIDTH_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_TOP * String_To_u8(&item->borderTop, text);
            break;

        case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_BOTTOM * String_To_u8(&item->borderBottom, text);
            break;

        case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_LEFT * String_To_u8(&item->borderLeft, text);
            break;

        case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_RIGHT * String_To_u8(&item->borderRight, text);
            break;

        // Set border radii
        case STYLE_BORDER_RADIUS_PROPERTY: {
            u8 border_radius;
            if (String_To_u8(&border_radius, text)) {
                item->borderRadiusTl = border_radius;
                item->borderRadiusTr = border_radius;
                item->borderRadiusBl = border_radius;
                item->borderRadiusBr = border_radius;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_TL;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_TR;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_BL;
                propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_BR;
            }
            break;
        }

        case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_TL *
                String_To_u8(&item->borderRadiusTl, text);
            break;

        case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_TR *
                String_To_u8(&item->borderRadiusTr, text);
            break;

        case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_BL *
                String_To_u8(&item->borderRadiusBl, text);
            break;

        case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_BORDER_RADIUS_BR *
                String_To_u8(&item->borderRadiusBr, text);
            break;

        // Set padding
        case STYLE_PADDING_PROPERTY: {
            u8 pad;
            if (String_To_u8(&pad, text)) {
                item->padTop = pad;
                item->padBottom = pad;
                item->padLeft = pad;
                item->padRight = pad;
                propertyFlagsSet |= PROPERTY_FLAG_PAD_TOP;
                propertyFlagsSet |= PROPERTY_FLAG_PAD_BOTTOM;
                propertyFlagsSet |= PROPERTY_FLAG_PAD_LEFT;
                propertyFlagsSet |= PROPERTY_FLAG_PAD_RIGHT;
            }
            break;
        }

        case STYLE_PADDING_TOP_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_PAD_TOP * String_To_u8(&item->padTop, text);
            break;

        case STYLE_PADDING_BOTTOM_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_PAD_BOTTOM * String_To_u8(&item->padBottom, text);
            break;

        case STYLE_PADDING_LEFT_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_PAD_LEFT * String_To_u8(&item->padLeft, text);
            break;

        case STYLE_PADDING_RIGHT_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_PAD_RIGHT * String_To_u8(&item->padRight, text);
            break;

        case STYLE_IMAGE_SOURCE_PROPERTY: {
            int imageHandle = ImageResourceManager_ImageHandleFromFilepath(imageResouceManager, text);

            // Image not loaded yet
            if (imageHandle == 0) {
                imageHandle = ImageResourceManager_LoadImage(imageResouceManager, displayManager, text);
                item->imageHandle = imageHandle;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE;
            }
            else {
                item->imageHandle = imageHandle;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE;
            }
            break;
        }

        case STYLE_IMAGE_REPEAT_PROPERTY: {
            switch(text[0]) {
                case 'h': {
                    item->imageRepeat = IMAGE_REPEAT_HORIZONTAL;
                    propertyFlagsSet |= PROPERTY_FLAG_IMAGE_REPEAT;
                    break;
                }
                case 'v': {
                    item->imageRepeat = IMAGE_REPEAT_VERTICAL;
                    propertyFlagsSet |= PROPERTY_FLAG_IMAGE_REPEAT;
                    break;
                }
                case 'b': {
                    item->imageRepeat = IMAGE_REPEAT_BOTH;
                    propertyFlagsSet |= PROPERTY_FLAG_IMAGE_REPEAT;
                    break;
                }
            }
            break;
        }

        case STYLE_IMAGE_FILTER_PROPERTY: {
            if (strcmp(text, "linear") == 0) {
                item->imageFilter = IMAGE_FILTER_LINEAR;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE_FILTER;
            }
            else if (strcmp(text, "nearest") == 0) {
                item->imageFilter = IMAGE_FILTER_NEAREST;
                propertyFlagsSet |= PROPERTY_FLAG_IMAGE_FILTER;
            }
            break;
        }

        case STYLE_FONT_PROPERTY: {
            void* foundFont = LinearStringmap_Get(&ss->fontNameIndexMap, text);
            if (foundFont) {
                item->fontIdx = *(u8*)foundFont;
                propertyFlagsSet |= PROPERTY_FLAG_FONT;
            }
            break;
        }

        case STYLE_INPUT_TYPE_PROPERTY:
            propertyFlagsSet |= PROPERTY_FLAG_INPUT_TYPE;
            if (strcmp(text, "number") == 0) item->inputType = 1;
            else item->inputType = 0;
            break;

        default: break;
    }

    item->propertyFlags |= propertyFlagsSet;
    return propertyFlagsSet;
}

void Stylesheet_ParseVariableProperty(Stylesheet* ss, const enum StylesheetToken token, StyleItem* item, StylesheetVariable variable)
{
    switch (token)
    {
        // Set layout direction
        case STYLE_LAYOUT_DIRECTION_PROPERTY:
            switch(variable.type)
            {
                case STYLESHEET_VARIABLE_DTYPE_VERTICAL:
                    item->layoutFlags |= LAYOUT_VERTICAL;
                    item->propertyFlags |= PROPERTY_FLAG_LAYOUT_DIR; break;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_HORIZONTAL:
                    item->propertyFlags |= PROPERTY_FLAG_LAYOUT_DIR; break;
                default:
                    break;
            }
            break;

        // Set growth
        case STYLE_GROW_PROPERTY:
            switch(variable.type)
            {
                case STYLESHEET_VARIABLE_DTYPE_VERTICAL: item->layoutFlags |= GROW_VERTICAL; break;
                case STYLESHEET_VARIABLE_DTYPE_HORIZONTAL: item->layoutFlags |= GROW_HORIZONTAL; break;
                case STYLESHEET_VARIABLE_DTYPE_BOTH: item->layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL); break;
                case STYLESHEET_VARIABLE_DTYPE_NONE: item->layoutFlags &= ~(GROW_HORIZONTAL | GROW_VERTICAL); break;
                default: break;
            }
            item->propertyFlags |= PROPERTY_FLAG_GROW;
            break;

        // Set overflow behaviour
        case STYLE_OVERFLOW_V_PROPERTY:
            switch(variable.type)
            {
                case STYLESHEET_VARIABLE_DTYPE_SCROLL:
                    item->layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                    item->propertyFlags |= PROPERTY_FLAG_VERTICAL_SCROLL; break;
                case STYLESHEET_VARIABLE_DTYPE_HORIZONTAL:
                    item->propertyFlags |= PROPERTY_FLAG_VERTICAL_SCROLL; break;
                default:
                    break;
            }
            break;

        case STYLE_OVERFLOW_H_PROPERTY:
            switch(variable.type)
            {
                case STYLESHEET_VARIABLE_DTYPE_SCROLL:
                    item->layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                    item->propertyFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL; break;
                case STYLESHEET_VARIABLE_DTYPE_HORIZONTAL:
                    item->propertyFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL; break;
                default:
                    break;
            }
            break;

        // Relative/Absolute positioning
        case STYLE_POSITION_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_ABSOLUTE) {
                item->layoutFlags |= POSITION_ABSOLUTE;
                item->propertyFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
            } else if (variable.type == STYLESHEET_VARIABLE_DTYPE_RELATIVE) {
                item->propertyFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
            }
            break;

        // Hide/show
        case STYLE_HIDE_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_TRUE) {
                item->layoutFlags |= HIDDEN;
                item->propertyFlags |= PROPERTY_FLAG_HIDDEN;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_FALSE) {
                item->propertyFlags |= PROPERTY_FLAG_HIDDEN;
            }
            break;

        // Ignore mouse
        case STYLE_IGNORE_MOUSE_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_TRUE) {
                item->layoutFlags |= IGNORE_MOUSE;
                item->propertyFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_FALSE) {
                item->propertyFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
            }
            break;

        // Set gap
        case STYLE_GAP_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->gap = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_GAP;
            }
            break;

        // Set preferred width
        case STYLE_WIDTH_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->preferredWidth = variable.value;
                item->imageWidthDimType = IMAGE_DIM_SPECIFIC;
                item->propertyFlags |= PROPERTY_FLAG_PREFERRED_WIDTH;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_IMAGE) {
                item->imageWidthDimType = IMAGE_DIM_INTRINSIC;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_ASPECT) {
                item->imageWidthDimType = IMAGE_DIM_ASPECT;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
            }
            break;

        // Set min width
        case STYLE_MIN_WIDTH_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->minWidth = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_MIN_WIDTH;
            }
            break;

        // Set max width
        case STYLE_MAX_WIDTH_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->maxWidth = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_MAX_WIDTH;
            }
            break;

        // Set preferred height
        case STYLE_HEIGHT_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->preferredHeight = variable.value;
                item->imageHeightDimType = IMAGE_DIM_SPECIFIC;
                item->propertyFlags |= PROPERTY_FLAG_PREFERRED_HEIGHT;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_IMAGE) {
                item->imageHeightDimType = IMAGE_DIM_INTRINSIC;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_ASPECT) {
                item->imageHeightDimType = IMAGE_DIM_ASPECT;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
            }
            break;

        // Set min height
        case STYLE_MIN_HEIGHT_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->minHeight = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_MIN_HEIGHT;
            }
            break;

        // Set max height
        case STYLE_MAX_HEIGHT_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->maxHeight = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_MAX_HEIGHT;
            }
            break;

        // Set horizontal alignment
        case STYLE_ALIGN_H_PROPERTY:
            switch(variable.type) {
                case STYLESHEET_VARIABLE_DTYPE_LEFT:
                    item->horizontalAlignment = 0;
                    item->propertyFlags |= PROPERTY_FLAG_ALIGN_H;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_CENTER:
                    item->horizontalAlignment = 1;
                    item->propertyFlags |= PROPERTY_FLAG_ALIGN_H;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_RIGHT:
                    item->horizontalAlignment = 2;
                    item->propertyFlags |= PROPERTY_FLAG_ALIGN_H;
                    break;
                default:
                    break;
            }
            break;

        // Set vertical alignment
        case STYLE_ALIGN_V_PROPERTY:
            switch(variable.type) {
                case STYLESHEET_VARIABLE_DTYPE_TOP:
                    item->verticalAlignment = 0;
                    item->propertyFlags |= PROPERTY_FLAG_ALIGN_V;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_CENTER:
                    item->verticalAlignment = 1;
                    item->propertyFlags |= PROPERTY_FLAG_ALIGN_V;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_BOTTOM:
                    item->verticalAlignment = 2;
                    item->propertyFlags |= PROPERTY_FLAG_ALIGN_V;
                    break;
                default:
                    break;
            }
            break;

        // Set horizontal text alignment
        case STYLE_TEXT_ALIGN_H_PROPERTY:
            switch(variable.type) {
                case STYLESHEET_VARIABLE_DTYPE_LEFT:
                    item->horizontalTextAlignment = 0;
                    item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_CENTER:
                    item->horizontalTextAlignment = 1;
                    item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_RIGHT:
                    item->horizontalTextAlignment = 2;
                    item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                    break;
                default:
                    break;
            }
            break;

        // Set vertical text alignment
        case STYLE_TEXT_ALIGN_V_PROPERTY:
            switch(variable.type) {
                case STYLESHEET_VARIABLE_DTYPE_TOP:
                    item->verticalTextAlignment = 0;
                    item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_CENTER:
                    item->verticalTextAlignment = 1;
                    item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                    break;
                case STYLESHEET_VARIABLE_DTYPE_BOTTOM:
                    item->verticalTextAlignment = 2;
                    item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                    break;
                default:
                    break;
            }
            break;

        // Set absolute position properties
        case STYLE_LEFT_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->left = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_LEFT;
            }
            break;

        case STYLE_RIGHT_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->right = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_RIGHT;
            }
            break;

        case STYLE_TOP_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->top = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_TOP;
            }
            break;

        case STYLE_BOTTOM_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->bottom = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BOTTOM;
            }
            break;

        // Set background colour
        case STYLE_BACKGROUND_COLOUR_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_RGB) {
                struct RGB rgb = UnpackRGB(variable.value);
                item->backgroundR = rgb.r;
                item->backgroundG = rgb.g;
                item->backgroundB = rgb.b;
                item->propertyFlags |= PROPERTY_FLAG_BACKGROUND;
                item->propertyFlags &= ~PROPERTY_FLAG_HIDE_BACKGROUND;
                item->layoutFlags &= ~HIDE_BACKGROUND;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_NONE) {
                item->propertyFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
                item->layoutFlags |= HIDE_BACKGROUND;
            }
            break;
        }

        // Set border colour
        case STYLE_BORDER_COLOUR_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_RGB) {
                struct RGB rgb = UnpackRGB(variable.value);
                item->borderR = rgb.r;
                item->borderG = rgb.g;
                item->borderB = rgb.b;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_COLOUR;
            }
            break;
        }

        // Set text colour
        case STYLE_TEXT_COLOUR_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_RGB) {
                struct RGB rgb = UnpackRGB(variable.value);
                item->textR = rgb.r;
                item->textG = rgb.g;
                item->textB = rgb.b;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_COLOUR;
            }
            break;
        }

        // Set border width
        case STYLE_BORDER_WIDTH_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                u8 border_width = variable.value;
                item->borderTop = border_width;
                item->borderBottom = border_width;
                item->borderLeft = border_width;
                item->borderRight = border_width;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_TOP;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_LEFT;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RIGHT;
            }
            break;
        }

        case STYLE_BORDER_TOP_WIDTH_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderTop = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_TOP;
            }
            break;

        case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderBottom = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
            }
            break;

        case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderLeft = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_LEFT;
            }
            break;

        case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderRight = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RIGHT;
            }
            break;

        // Set border radii
        case STYLE_BORDER_RADIUS_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                u8 border_radius = variable.value;
                item->borderRadiusTl = border_radius;
                item->borderRadiusTr = border_radius;
                item->borderRadiusBl = border_radius;
                item->borderRadiusBr = border_radius;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
            }
            break;
        }

        case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderRadiusTl = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
            }
            break;

        case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderRadiusTr = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
            }
            break;

        case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderRadiusBl = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
            }
            break;

        case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->borderRadiusBr = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
            }
            break;

        // Set padding
        case STYLE_PADDING_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                u8 pad = variable.value;
                item->padTop = pad;
                item->padBottom = pad;
                item->padLeft = pad;
                item->padRight = pad;
                item->propertyFlags |= PROPERTY_FLAG_PAD_TOP;
                item->propertyFlags |= PROPERTY_FLAG_PAD_BOTTOM;
                item->propertyFlags |= PROPERTY_FLAG_PAD_LEFT;
                item->propertyFlags |= PROPERTY_FLAG_PAD_RIGHT;
            }
            break;
        }

        case STYLE_PADDING_TOP_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->padTop = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_PAD_TOP;
            }
            break;

        case STYLE_PADDING_BOTTOM_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->padBottom = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_PAD_BOTTOM;
            }
            break;

        case STYLE_PADDING_LEFT_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->padLeft = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_PAD_LEFT;
            }
            break;

        case STYLE_PADDING_RIGHT_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->padRight = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_PAD_RIGHT;
            }
            break;

        case STYLE_IMAGE_REPEAT_PROPERTY: {
            switch(variable.type) {
                case STYLESHEET_VARIABLE_DTYPE_HORIZONTAL: {
                    item->imageRepeat = IMAGE_REPEAT_HORIZONTAL;
                    item->propertyFlags |= PROPERTY_FLAG_IMAGE_REPEAT;
                    break;
                }
                case STYLESHEET_VARIABLE_DTYPE_VERTICAL: {
                    item->imageRepeat = IMAGE_REPEAT_VERTICAL;
                    item->propertyFlags |= PROPERTY_FLAG_IMAGE_REPEAT;
                    break;
                }
                case STYLESHEET_VARIABLE_DTYPE_BOTH: {
                    item->imageRepeat = IMAGE_REPEAT_BOTH;
                    item->propertyFlags |= PROPERTY_FLAG_IMAGE_REPEAT;
                    break;
                }
                default:
                    break;
            }
            break;
        }

        case STYLE_IMAGE_FILTER_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_LINEAR) {
                item->imageFilter = IMAGE_FILTER_LINEAR;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_FILTER;
            }
            else if (variable.type == STYLESHEET_VARIABLE_DTYPE_NEAREST) {
                item->imageFilter = IMAGE_FILTER_NEAREST;
                item->propertyFlags |= PROPERTY_FLAG_IMAGE_FILTER;
            }
            break;
        }

        case STYLE_FONT_PROPERTY: {
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_NUMBER) {
                item->fontIdx = variable.value;
                item->propertyFlags |= PROPERTY_FLAG_FONT;
            }
            break;
        }

        case STYLE_INPUT_TYPE_PROPERTY:
            if (variable.type == STYLESHEET_VARIABLE_DTYPE_INPUT_NUMBER) {
                item->propertyFlags |= PROPERTY_FLAG_INPUT_TYPE;
                item->inputType = 1;
            }
            else {
                item->inputType = 0;
            }
            break;

        default:
            break;
    }
}

StyleTextRef* Stylesheet_BinarySearchTextRef(Array* textRefs, int targetTokenIndex)
{
    int left = 0;
    int right = textRefs->size - 1;
    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        StyleTextRef* textRef = (StyleTextRef*)Array_Get(textRefs, mid);
        if (textRef->tokenIndex == targetTokenIndex) {
            return textRef;  // found
        } else if (textRef->tokenIndex < targetTokenIndex) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return NULL;  // not found (does not happen)
}

void Stylesheet_ParseVariable(Stylesheet* ss, const char* text, int textLen, int* valueOut, enum StylesheetVariableDtype* typeOut)
{
    struct RGB rgb;
    int _int;
    if (strcmp(text, "v") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_VERTICAL;
    }
    else if (strcmp(text, "h") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_HORIZONTAL;
    }
    else if (strcmp(text, "b") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_BOTH;
    }
    else if (strcmp(text, "s") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_SCROLL;
    }
    else if (strcmp(text, "none") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_NONE;
    }
    else if (strcmp(text, "left") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_LEFT;
    }
    else if (strcmp(text, "right") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_RIGHT;
    }
    else if (strcmp(text, "top") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_TOP;
    }
    else if (strcmp(text, "bottom") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_BOTTOM;
    }
    else if (strcmp(text, "center") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_CENTER;
    }
    else if (strcmp(text, "true") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_TRUE;
    }
    else if (strcmp(text, "false") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_FALSE;
    }
    else if (strcmp(text, "absolute") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_ABSOLUTE;
    }
    else if (strcmp(text, "relative") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_RELATIVE;
    }
    else if (strcmp(text, "linear") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_LINEAR;
    }
    else if (strcmp(text, "nearest") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_NEAREST;
    }
    else if (strcmp(text, "number") == 0) {
        *typeOut = STYLESHEET_VARIABLE_DTYPE_INPUT_NUMBER;
    }
    else if (Parse_Hexcode(text, textLen, &rgb)) {
        *valueOut = PackRGB(rgb);
        *typeOut = STYLESHEET_VARIABLE_DTYPE_RGB;
    }
    else if (String_To_Int(&_int, text)) {
        *valueOut = _int;
        *typeOut = STYLESHEET_VARIABLE_DTYPE_NUMBER;
    }
    else {
        void* foundFont = LinearStringmap_Get(&ss->fontNameIndexMap, text);
        if (foundFont) {
            *valueOut = *(u8*)foundFont;
            *typeOut = STYLESHEET_VARIABLE_DTYPE_NUMBER;
        }
        else {
            *valueOut = 0;
            *typeOut = STYLESHEET_VARIABLE_DTYPE_UNKNOWN;
        }
    }
}

int Stylesheet_ParseVariables(char* src, TokenArray* tokens, Stylesheet* ss, Array* textRefs, LinearStringmap* variableMap)
{
    int inVariableSelector = 0;
    int i = 0;

    while(i < tokens->size)
    {
        const enum StylesheetToken token = TokenArray_Get(tokens, i);

        if (token == STYLE_VAR_SELECTOR)
        {
            if (!AssertVariableSelectorGrammar(tokens, i)) return 0;
            inVariableSelector = 1; // Enter @var selector
        }
        else if (token == STYLE_VARIABLE_NAME && inVariableSelector)
        {
            if (!AssertVariableAssignmentGrammar(tokens, i)) return 0;

            // Use binary search to find the desired text refs
            StyleTextRef* varNameTextRef = Stylesheet_BinarySearchTextRef(textRefs, i);
            StyleTextRef* valTextRef = Stylesheet_BinarySearchTextRef(textRefs, i+2);

            // If text ref -> parse variable
            if (varNameTextRef && valTextRef)
            {
                // Get variable name and value strings
                char* varName = &src[varNameTextRef->srcIndex];
                char* valText = &src[valTextRef->srcIndex];

                // Try to parse variable
                StylesheetVariable newVar;
                newVar.type = STYLESHEET_VARIABLE_DTYPE_UNKNOWN;
                newVar.value = 0;
                newVar.nextVariationIndex = -1;
                Stylesheet_ParseVariable(ss, valText, valTextRef->len, &newVar.value, &newVar.type);

                // Variable exists -> overwrite value
                int* existingVarIndex = LinearStringmap_Get(variableMap, varName);
                if (existingVarIndex) {
                    StylesheetVariable* variable = Array_Get(&ss->variables, *existingVarIndex);
                    *variable = newVar;
                }
                // Variable does not exist -> create it
                else {
                    // Add variable
                    Array_Push(&ss->variables, &newVar);
                    int index = ss->variables.size - 1;

                    // Add index to variableMap
                    LinearStringmap_Set(variableMap, varName, &index);
                }
            }
            i += 3; continue; // ^
        }
        else if (token == STYLE_SELECTOR_CLOSE_BRACE) inVariableSelector = 0; // Exit @var selector
        i += 1; // ^
    }

    return 1;
}

int Stylesheet_ParseScreenQueries(char* src, TokenArray* tokens, Stylesheet* ss, Array* textRefs, LinearStringmap* variableMap)
{
    StylesheetScreenQuery* screenQuery = NULL;
    int inScreenQuerySelector = 0;
    int i = 0;

    while(i < tokens->size)
    {
        const enum StylesheetToken token = TokenArray_Get(tokens, i);

        if (token == STYLE_SCREEN_SELECTOR) {

            if (!AssertScreenSelectorGrammar(tokens, i)) return 0;

            enum StylesheetToken comparatorToken = TokenArray_Get(tokens, i+1);
            enum StylesheetToken screenWidthToken = TokenArray_Get(tokens, i+2);

            // Get screen width text ref
            StyleTextRef* textRef = Stylesheet_BinarySearchTextRef(textRefs, i+2);
            const char* text = &src[textRef->srcIndex];
            int screenWidth;
            if (String_To_Int(&screenWidth, text)) {

                // Add screen query
                screenQuery = Array_PushEmpty(&ss->screenQueries);
                screenQuery->screenWidth = screenWidth;
                screenQuery->comparator = comparatorToken;
            }
            else {
                char errMessage[256];
                snprintf(errMessage, sizeof(errMessage), "<CSS Error> Expected screen width (int), got \"%s\" ','", text);
                ErrorSystem_AddError(&GUI.errorSystem, errMessage);
                return 0;
            }

            inScreenQuerySelector = 1;
            i += 4; continue;
        }
        else if (token == STYLE_VARIABLE_NAME && inScreenQuerySelector) {

            if (!AssertVariableAssignmentGrammar(tokens, i)) return 0;

            // Use binary search to find the desired text refs
            StyleTextRef* varNameTextRef = Stylesheet_BinarySearchTextRef(textRefs, i);
            StyleTextRef* valTextRef = Stylesheet_BinarySearchTextRef(textRefs, i+2);

            // If text ref -> parse variable
            if (varNameTextRef && valTextRef) {

                // Get variable name
                char* varName = &src[varNameTextRef->srcIndex];
                char* valText = &src[valTextRef->srcIndex];

                // Get existing variable index
                int* existingVarIndex = LinearStringmap_Get(variableMap, varName);
                if (existingVarIndex)
                {
                    int screenQueryIndex = ss->screenQueries.size - 1;
                    StylesheetVariable* variable = Array_Get(&ss->variables, *existingVarIndex);

                    // Walk to the end of the variable chain
                    while (variable->nextVariationIndex != -1) {
                        variable = Array_Get(&ss->variables, variable->nextVariationIndex);
                    }

                    // Variable variation appeared previously in this @screen -> overwrite it
                    if (variable->screenQueryIndex == screenQueryIndex) {
                        Stylesheet_ParseVariable(ss, valText, valTextRef->len, &variable->value, &variable->type);
                    }
                    // Create new variable variation
                    else {
                        // Parse property into variable
                        StylesheetVariable newVar;
                        newVar.type = STYLESHEET_VARIABLE_DTYPE_UNKNOWN;
                        newVar.value = 0;
                        newVar.nextVariationIndex = -1;
                        newVar.screenQueryIndex = screenQueryIndex;
                        Stylesheet_ParseVariable(ss, valText, valTextRef->len, &newVar.value, &newVar.type);

                        // Add new variable to the variation chain
                        Array_Push(&ss->variables, &newVar);
                        int newVarIndex = ss->variables.size - 1;
                        variable->nextVariationIndex = newVarIndex;
                    }
                }
                // Variable not initialised (not found in @var) -> error
                else {
                    char errMessage[256];
                    snprintf(errMessage, sizeof(errMessage), "<CSS Error> Variable \"%s\" uninitialised! Add this variable to @var','", varName);
                    ErrorSystem_AddError(&GUI.errorSystem, errMessage);
                    return 0;
                }
            }
            i += 3; continue; // ^
        }
        else if (token == STYLE_SELECTOR_CLOSE_BRACE)  {
            inScreenQuerySelector = 0; // Exit @var selector
        }
        i += 1;
    }
    return 1;
}

void Stylesheet_ParseScrollbarProperty(Stylesheet* ss, const enum StylesheetToken token, const char* text)
{
    u8 property;
    switch (token)
    {
        case STYLE_WIDTH_PROPERTY:
            if (String_To_u8(&property, text)) ss->scrollbarStyle.width = property;
            break;
        case STYLE_HEIGHT_PROPERTY:
            if (String_To_u8(&property, text)) ss->scrollbarStyle.height = property;
            break;
        case STYLE_SCROLLBAR_OVERLAY:
            if (stringEquals(text, "true")) ss->scrollbarStyle.overlay = true;
            break;
        default: break;
    }
}

void Stylesheet_ParseScrollThumbProperty(Stylesheet* ss, const enum StylesheetToken token, const char* text, int textLen)
{
    u8 property;
    struct RGB rgb;
    switch (token)
    {
        case STYLE_BACKGROUND_COLOUR_PROPERTY:
            if (Parse_Hexcode(text, textLen, &rgb)) {
                ss->scrollbarStyle.thumbBackgroundR = rgb.r;
                ss->scrollbarStyle.thumbBackgroundG = rgb.g;
                ss->scrollbarStyle.thumbBackgroundB = rgb.b;
            }
            break;
        case STYLE_BORDER_COLOUR_PROPERTY:
            if (Parse_Hexcode(text, textLen, &rgb)) {
                ss->scrollbarStyle.thumbBorderR = rgb.r;
                ss->scrollbarStyle.thumbBorderG = rgb.g;
                ss->scrollbarStyle.thumbBorderB = rgb.b;
            }
            break;
        case STYLE_BORDER_WIDTH_PROPERTY:
            if (String_To_u8(&property, text)) {
                ss->scrollbarStyle.thumbBorderTop    = property;
                ss->scrollbarStyle.thumbBorderBottom = property;
                ss->scrollbarStyle.thumbBorderLeft   = property;
                ss->scrollbarStyle.thumbBorderRight  = property;
            }
            break;
        case STYLE_BORDER_TOP_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderTop, text); // 0 if parse fails
            break;
        case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderBottom, text); // 0 if parse fails
            break;
        case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderLeft, text); // 0 if parse fails
            break;
        case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderRight, text); // 0 if parse fails
            break;
        case STYLE_BORDER_RADIUS_PROPERTY:
            if (String_To_u8(&property, text)) {
                ss->scrollbarStyle.thumbBorderRadiusTl = property;
                ss->scrollbarStyle.thumbBorderRadiusTr = property;
                ss->scrollbarStyle.thumbBorderRadiusBl = property;
                ss->scrollbarStyle.thumbBorderRadiusBr = property;
            }
            break;
        case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderRadiusTl, text); // 0 if parse fails
            break;
        case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderRadiusTr, text); // 0 if parse fails
            break;
        case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderRadiusBl, text); // 0 if parse fails
            break;
        case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.thumbBorderRadiusBr, text); // 0 if parse fails
            break;
        case STYLE_SCROLL_THUMB_MIN_SIZE:
            if (String_To_u8(&property, text)) {
                ss->scrollbarStyle.thumbMinSize = property;
            }
            break;
        default: // Invalid property (ignore)
            break;
    }
}

void Stylesheet_ParseScrollTrackProperty(Stylesheet* ss, const enum StylesheetToken token, const char* text, int textLen)
{
    u8 property;
    struct RGB rgb;
    switch (token)
    {
        case STYLE_BACKGROUND_COLOUR_PROPERTY:
            if (Parse_Hexcode(text, textLen, &rgb)) {
                ss->scrollbarStyle.trackBackgroundR = rgb.r;
                ss->scrollbarStyle.trackBackgroundG = rgb.g;
                ss->scrollbarStyle.trackBackgroundB = rgb.b;
            }
            break;
        case STYLE_BORDER_COLOUR_PROPERTY:
            if (Parse_Hexcode(text, textLen, &rgb)) {
                ss->scrollbarStyle.trackBorderR = rgb.r;
                ss->scrollbarStyle.trackBorderG = rgb.g;
                ss->scrollbarStyle.trackBorderB = rgb.b;
            }
            break;
        case STYLE_BORDER_WIDTH_PROPERTY:
            if (String_To_u8(&property, text)) {
                ss->scrollbarStyle.trackBorderTop    = property;
                ss->scrollbarStyle.trackBorderBottom = property;
                ss->scrollbarStyle.trackBorderLeft   = property;
                ss->scrollbarStyle.trackBorderRight  = property;
            }
            break;
        case STYLE_BORDER_TOP_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderTop, text);
            break;
        case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderBottom, text);
            break;
        case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderLeft, text);
            break;
        case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderRight, text);
            break;
        case STYLE_BORDER_RADIUS_PROPERTY:
            if (String_To_u8(&property, text)) {
                ss->scrollbarStyle.trackBorderRadiusTl = property;
                ss->scrollbarStyle.trackBorderRadiusTr = property;
                ss->scrollbarStyle.trackBorderRadiusBl = property;
                ss->scrollbarStyle.trackBorderRadiusBr = property;
            }
            break;
        case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderRadiusTl, text);
            break;
        case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderRadiusTr, text);
            break;
        case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderRadiusBl, text);
            break;
        case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackBorderRadiusBr, text);
            break;
        case STYLE_PADDING_PROPERTY:
            if (String_To_u8(&property, text)) {
                ss->scrollbarStyle.trackPadTop    = property;
                ss->scrollbarStyle.trackPadBottom = property;
                ss->scrollbarStyle.trackPadLeft   = property;
                ss->scrollbarStyle.trackPadRight  = property;
            }
            break;
        case STYLE_PADDING_TOP_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackPadTop, text);
            break;
        case STYLE_PADDING_BOTTOM_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackPadBottom, text);
            break;
        case STYLE_PADDING_LEFT_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackPadLeft, text);
            break;
        case STYLE_PADDING_RIGHT_PROPERTY:
            String_To_u8(&ss->scrollbarStyle.trackPadRight, text);
            break;
        default: // Invalid property (ignore)
            break;
    }
}

int Stylesheet_ParseFonts(Stylesheet* ss, ThreadPool* threadpool, char* src, TokenArray* tokens, Array* textRefs)
{
    int inFontSelector = 0;
    int fontSize = 18;
    int fontWeight = 400;
    char* fontName = NULL;
    char* fontSrc = NULL;
    Stylesheet_FontLoadJob fontJobs[64]; int fontJobCount = 0;
    int i = 0;

    while(i < tokens->size)
    {
        const enum StylesheetToken token = TokenArray_Get(tokens, i);

        if (Stylesheet_IsPropertyIdentifierToken(token) && inFontSelector) {
            if (!AssertPropertyIdentifierGrammar(tokens, i)) return 0;

            // Use binary search to find the corresponding property text
            StyleTextRef* textRef = Stylesheet_BinarySearchTextRef(textRefs, i + 2);

            if (textRef)
            {
                char* propertyText = &src[textRef->srcIndex];

                switch (token)
                {
                    case STYLE_FONT_SRC: {
                        fontSrc = propertyText;
                        break;
                    }
                    case STYLE_FONT_SIZE: {
                        int size = 0;
                        if (String_To_Int(&size, propertyText)) fontSize = size;
                        break;
                    }
                    case STYLE_FONT_WEIGHT: {
                        int weight = 0;
                        if (String_To_Int(&weight, propertyText)) fontWeight = weight;
                        break;
                    }
                    default:
                        break;
                }
            }
            i += 3; continue;
        }
        else if (token == STYLE_FONT_CREATION_SELECTOR) {
            if (!AssertFontCreationSelectorGrammar(tokens, i)) return 0;

            StyleTextRef* textRef = Stylesheet_BinarySearchTextRef(textRefs, i+1);
            if (textRef) fontName = &src[textRef->srcIndex];
            inFontSelector = 1; // Enter @font selector
            i += 3; continue; // ^
        }
        else if (token == STYLE_SELECTOR_CLOSE_BRACE && inFontSelector) {
            if (fontSrc != NULL) {

                // Create a new font
                void* found_font = LinearStringmap_Get(&ss->fontNameIndexMap, fontName);
                if (found_font == NULL && fontJobCount < 64)
                {
                    // Create uninitialised font
                    Font* createFont = Array_PushEmpty(&ss->fonts);
                    u8 createFontIdx = ss->fonts.size - 1;
                    LinearStringmap_Set(&ss->fontNameIndexMap, fontName, &createFontIdx);

                    // Create a font job
                    fontJobs[fontJobCount].stylesheet = ss;
                    fontJobs[fontJobCount].filepath = StringCreate(fontSrc);
                    fontJobs[fontJobCount].fontSize = fontSize;
                    fontJobs[fontJobCount].fontWeight = fontWeight;
                    fontJobs[fontJobCount].fontIdx = createFontIdx;
                    fontJobCount++;
                }
                else if (fontJobCount == 64) {
                    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Exceeded max font count of 64! ','");
                }
            }
            else {
                // Free filepath strings
                for (int i=0; i<fontJobCount; i++) {
                    StringFree(fontJobs[i].filepath);
                }
                return 0;
            }

            inFontSelector = 0; // Exit @font selector
        }
        i += 1;
    }

    timer_start();

    // Load fonts in parallel
    for (int i=0; i<fontJobCount; i++) {
        if (!ThreadPool_Submit(threadpool, Stylesheet_FontLoaderThread, &fontJobs[i])) return 0;
    }
    ThreadPool_Wait(threadpool);

    timer_stop();

    // Free filepath strings
    for (int i=0; i<fontJobCount; i++) {
        StringFree(fontJobs[i].filepath);
    }

    return 1;
}

StyleItem* Stylesheet_FindOrCreateItemVariant(Stylesheet* ss, StyleItem* baseItem, int screenQueryIndex)
{
    StyleItem* current = baseItem;
    StyleItem* prev = NULL;

    while (current != NULL) {
        if (current->screenQueryIndex == screenQueryIndex) {
            return current;  // Found it!
        }
        else if (current->screenQueryIndex > screenQueryIndex) {
            // Insert before current
            StyleItem* newVariant = Array_PushEmpty(&ss->items);
            newVariant->propertyFlags = 0;
            newVariant->layoutFlags = 0;
            newVariant->screenQueryIndex = screenQueryIndex;
            newVariant->nextVariationIndex = -1;

            if (prev == NULL) {
                newVariant->nextVariationIndex = current->nextVariationIndex;
            } else {
                newVariant->nextVariationIndex = prev->nextVariationIndex;
                prev->nextVariationIndex = ss->items.size - 1;
            }
            return newVariant;
        }

        prev = current;
        if (current->nextVariationIndex == -1) break;
        current = Array_Get(&ss->items, current->nextVariationIndex);
    }

    // Not found - append to end
    StyleItem* newVariant = Array_PushEmpty(&ss->items);
    newVariant->propertyFlags = 0;
    newVariant->layoutFlags = 0;
    newVariant->screenQueryIndex = screenQueryIndex;
    newVariant->nextVariationIndex = -1;

    if (prev == NULL) baseItem->nextVariationIndex = ss->items.size - 1;
    else prev->nextVariationIndex = ss->items.size - 1;

    return newVariant;
}

int Stylesheet_LoadDefaultFont(Stylesheet* ss)
{
    bool subpixelRendering = true;
    #ifdef PLATFORM_MACOS
        subpixelRendering = false;
    #endif

    Font* font = Array_PushEmpty(&ss->fonts); // FontID will be 0
    if (Font_CreateDefault(font, 14, 400, subpixelRendering)) {
        return 1;
    }

    // Failed to create default font
    return 0;
}

int Stylesheet_Parse(
    char* src,
    TokenArray* tokens,
    Array* textRefs,
    LinearStringmap* variableMap,
    Stylesheet* ss,
    ThreadPool* threadpool,
    ImageResourceManager* imageResouceManager,
    DisplayManager* displayManager)
{
    // Parse fonts
    if (!Stylesheet_ParseFonts(ss, threadpool, src, tokens, textRefs)) return 0;

    // Parse variables
    if (!Stylesheet_ParseVariables(src, tokens, ss, textRefs, variableMap)) return 0;

    // Parse screen queries
    if (!Stylesheet_ParseScreenQueries(src, tokens, ss, textRefs, variableMap)) return 0;

    // Context
    int ctx = STYLE_PARSE_CTX_SELECTOR;

    // State
    u32 selectorIndexes[256];
    int selectorCount = 0;
    int classIdAutoIncr = 0;
    int idIdAutoIncr = 0;
    u32 textRefIndex = 0;
    StyleTextRef* textRef;

    // Temp style item
    StyleItem tempItem;

    // Parse
    int i = 0;
    while(i < tokens->size)
    {
        const enum StylesheetToken token = TokenArray_Get(tokens, i);

        int iAdvance = 1;

        switch(token) {
            case STYLE_CLASS_SELECTOR: {
                if (i < tokens->size - 1)
                {
                    enum StylesheetToken next_token = TokenArray_Get(tokens, i+1);

                    // Get class string
                    textRef = (StyleTextRef*)Array_Get(textRefs, textRefIndex++);
                    char* src_class = &src[textRef->srcIndex];

                    StyleItemKey key;
                    key.idID = -1;
                    key.tag = -1;
                    key.pseudoClass = -1;

                    // If regular class selector
                    if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)
                    {
                        iAdvance = 1;
                    }
                    // If class pseudo selector
                    else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size - 3)
                    {
                        enum StylesheetToken following_token = TokenArray_Get(tokens, i+2);
                        enum StylesheetToken third_token = TokenArray_Get(tokens, i+3);
                        if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) {
                            key.pseudoClass = Token_To_Pseudo_Class(following_token);
                        }
                        else return 0;
                        iAdvance = 3;
                    }
                    else return 0;

                    // Get or create class ID
                    int* classID = LinearStringmap_Get(&ss->classIdMap, src_class);
                    if (classID) {
                        key.classID = *classID;
                    }
                    else {
                        // Generate and add class ID
                        int classID = classIdAutoIncr++;
                        LinearStringmap_Set(&ss->classIdMap, src_class, &classID);
                        key.classID = classID;
                    }

                    // Get or create Item
                    int* itemIndex = Hashmap_Get(&ss->itemIndexMap, &key);
                    if (itemIndex) {
                        selectorIndexes[selectorCount] = *itemIndex;
                    }
                    else {
                        // Create and add item
                        StyleItem newItem;
                        newItem.propertyFlags = 0;
                        newItem.nextVariationIndex = -1;
                        newItem.screenQueryIndex = -1;
                        Array_Push(&ss->items, &newItem);
                        selectorIndexes[selectorCount] = ss->items.size - 1;
                        Hashmap_Set(&ss->itemIndexMap , &key, &selectorIndexes[selectorCount]);
                    }
                    selectorCount++;
                }
                else return 0;
                break;
            }
            case STYLE_ID_SELECTOR: {
                if (i < tokens->size - 1)
                {
                    enum StylesheetToken next_token = TokenArray_Get(tokens, i+1);

                    // Get id string
                    textRef = (StyleTextRef*)Array_Get(textRefs, textRefIndex++);
                    char* src_id = &src[textRef->srcIndex];

                    StyleItemKey key;
                    key.classID = -1;
                    key.tag = -1;
                    key.pseudoClass = -1;

                    // If regular id selector
                    if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)
                    {
                        iAdvance = 1;
                    }
                    // If id pseudo selector
                    else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size - 3)
                    {
                        enum StylesheetToken following_token = TokenArray_Get(tokens, i+2);
                        enum StylesheetToken third_token = TokenArray_Get(tokens, i+3);
                        if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) {
                            key.pseudoClass = Token_To_Pseudo_Class(following_token);
                        }
                        else return 0;
                        iAdvance = 3;
                    }
                    else return 0;

                    // Get or create id ID
                    int* idID = LinearStringmap_Get(&ss->idIdMap, src_id);
                    if (idID) {
                        key.idID = *idID;
                    }
                    else {
                        // Generate and add class ID
                        int idID = idIdAutoIncr++;
                        LinearStringmap_Set(&ss->idIdMap, src_id, &idID);
                        key.idID = idID;
                    }

                    // Get or create Item
                    int* itemIndex = Hashmap_Get(&ss->itemIndexMap, &key);
                    if (itemIndex) {
                        selectorIndexes[selectorCount] = *itemIndex;
                    }
                    else {
                        // Create and add item
                        StyleItem newItem;
                        newItem.propertyFlags = 0;
                        newItem.nextVariationIndex = -1;
                        newItem.screenQueryIndex = -1;
                        Array_Push(&ss->items, &newItem);
                        selectorIndexes[selectorCount] = ss->items.size - 1;
                        Hashmap_Set(&ss->itemIndexMap , &key, &selectorIndexes[selectorCount]);
                    }
                    selectorCount++;
                }
                else return 0;
                break;
            }
            case STYLE_SELECTOR_COMMA: {
                if (AssertSelectorCommaGrammar(tokens, i)) {
                    if (selectorCount == 256) return 0;
                    iAdvance = 1;
                }
                else return 0;
                break;
            }
            case STYLE_FONT_CREATION_SELECTOR: {
                textRefIndex += 1;
                ctx = STYLE_PARSE_CTX_FONT_SELECTOR;
                iAdvance = 2;
                break;
            }
            case STYLE_SCROLLBAR_SELECTOR: {
                if (!AssertScrollSelectorGrammar(tokens, i)) return 0;
                ctx = STYLE_PARSE_CTX_SCROLLBAR_SELECTOR;
                iAdvance = 2;
                break;
            }
            case STYLE_SCROLL_THUMB_SELECTOR: {
                if (!AssertScrollSelectorGrammar(tokens, i)) return 0;
                ctx = STYLE_PARSE_CTX_SCROLL_THUMB_SELECTOR;
                iAdvance = 2;
                break;
            }
            case STYLE_SCROLL_TRACK_SELECTOR: {
                if (!AssertScrollSelectorGrammar(tokens, i)) return 0;
                ctx = STYLE_PARSE_CTX_SCROLL_TRACK_SELECTOR;
                iAdvance = 2;
                break;
            }
            case STYLE_VAR_SELECTOR: {
                ctx = STYLE_PARSE_CTX_VAR_SELECTOR;
                iAdvance = 2;
                break;
            }
            case STYLE_SCREEN_SELECTOR: {
                textRefIndex += 1; // skip screen width value text e.g. "1080"
                ctx = STYLE_PARSE_CTX_SCREEN_SELECTOR;
                iAdvance = 4;
                break;
            }
            case STYLE_VARIABLE_NAME: {
                textRefIndex += 2; // skip name and value text e.g. "--var" "200"
                iAdvance = 3;
                break;
            }
            case STYLE_SELECTOR_OPEN_BRACE: {
                if (AssertSelectionOpeningBraceGrammar(tokens, i)) {
                    tempItem.propertyFlags = 0;
                    tempItem.layoutFlags = 0;
                    iAdvance = 1;
                }
                else return 0;
                break;
            }
            case STYLE_SELECTOR_CLOSE_BRACE: {
                if (!AssertSelectionClosingBraceGrammar(tokens, i)) return 0;
                // Apply temp item to all listed items
                for (int s=0; s<selectorCount; s++) {
                    StyleItem* baseItem = Array_Get(&ss->items, selectorIndexes[s]);
                    Stylesheet_OverwriteStyleItem(baseItem, &tempItem);
                }

                selectorCount = 0;
                ctx = 0;
                iAdvance = 1;
                break;
            }
            case STYLE_DEFAULT_SELECTOR: {
                if (!AssertDefaultSelectorGrammar(tokens, i)) return 0;
                selectorIndexes[selectorCount] = 0;
                selectorCount++;
                iAdvance = 1;
                break;
            }
            default: {
                if (Stylesheet_IsPropertyIdentifierToken(token)) {

                    // Property Identifier Assertion
                    if (!AssertPropertyIdentifierGrammar(tokens, i)) return 0;

                    // Get property / variable text ref
                    textRef = (StyleTextRef*)Array_Get(textRefs, textRefIndex++);
                    char* text = &src[textRef->srcIndex];

                    // Variable property
                    enum StylesheetToken secondNextToken = TokenArray_Get(tokens, i+2);
                    if (secondNextToken == STYLE_VARIABLE_PROPERTY_VALUE && ctx == 0)
                    {
                        const char* variableName = text;
                        int* varIndex = LinearStringmap_Get(variableMap, variableName);

                        // Error! property value not assigned
                        if (!varIndex) {
                            char errMessage[256];
                            snprintf(errMessage, sizeof(errMessage), "<CSS Error> Variable \"%s\" is undefined", variableName);
                            ErrorSystem_AddError(&GUI.errorSystem, errMessage);
                            return 0;
                        }

                        int largestVariantIndex = 0;

                        // Apply base variable to tempItem
                        StylesheetVariable* variable = Array_Get(&ss->variables, *varIndex);
                        Stylesheet_ParseVariableProperty(ss, token, &tempItem, *variable);

                        // Apply variable variations to variant items
                        while (variable->nextVariationIndex != -1) {

                            // Get next variable variant
                            variable = Array_Get(&ss->variables, variable->nextVariationIndex);

                            // Apply directly to variant style items (variant variables don't have to go through temp intermediary)
                            for (int s=0; s<selectorCount; s++) {
                                StyleItem* baseItem = Array_Get(&ss->items, selectorIndexes[s]);
                                StyleItem* variantItem = Stylesheet_FindOrCreateItemVariant(ss, baseItem, variable->screenQueryIndex);
                                Stylesheet_ParseVariableProperty(ss, token, variantItem, *variable);
                            }
                        }
                    }
                    // Inline property
                    else {
                        switch (ctx) {
                            case STYLE_PARSE_CTX_SELECTOR: {
                                u64 propertyFlagsSet = Stylesheet_ParseProperty(ss, token, &tempItem, text, textRef->len, imageResouceManager, displayManager);

                                // Clear these flags from all selected variations
                                for (int s=0; s<selectorCount; s++) {
                                    StyleItem* item = Array_Get(&ss->items, selectorIndexes[s]);
                                    while (item->nextVariationIndex != -1) {
                                        item = Array_Get(&ss->items, item->nextVariationIndex);
                                        item->propertyFlags &= ~propertyFlagsSet;
                                    }
                                }
                                break;
                            }
                            case STYLE_PARSE_CTX_SCROLLBAR_SELECTOR:
                                Stylesheet_ParseScrollbarProperty(ss, token, text);
                                break;
                            case STYLE_PARSE_CTX_SCROLL_THUMB_SELECTOR:
                                Stylesheet_ParseScrollThumbProperty(ss, token, text, textRef->len);
                                break;
                            case STYLE_PARSE_CTX_SCROLL_TRACK_SELECTOR:
                                Stylesheet_ParseScrollTrackProperty(ss, token, text, textRef->len);
                                break;
                            default:
                                break;
                        }
                    }

                    iAdvance = 3;
                }
                else if (Stylesheet_IsTagSelectorToken(token)) {
                    if (i < tokens->size - 1)
                    {
                        enum StylesheetToken next_token = TokenArray_Get(tokens, i+1);

                        StyleItemKey key;
                        key.classID = -1;
                        key.idID = -1;
                        key.tag = NU_Token_To_Tag(token);
                        key.pseudoClass = -1;

                        // Regular tag item key
                        if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)
                        {
                            iAdvance = 1;
                        }
                        // Pseudo tag item key
                        else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size - 3)
                        {
                            enum StylesheetToken following_token = TokenArray_Get(tokens, i+2);
                            enum StylesheetToken third_token = TokenArray_Get(tokens, i+3);
                            if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE))
                            {
                                key.pseudoClass = Token_To_Pseudo_Class(following_token);
                            }
                            else return 0;
                            iAdvance = 3;
                        }
                        else return 0;

                        // If style item exists
                        int* itemIndex = Hashmap_Get(&ss->itemIndexMap, &key);
                        if (itemIndex) {
                            selectorIndexes[selectorCount] = *itemIndex;
                        }
                        // Create style item
                        else {
                            // Create and add item
                            StyleItem newItem;
                            newItem.propertyFlags = 0;
                            newItem.nextVariationIndex = -1;
                            newItem.screenQueryIndex = -1;
                            Array_Push(&ss->items, &newItem);
                            selectorIndexes[selectorCount] = ss->items.size - 1;
                            Hashmap_Set(&ss->itemIndexMap , &key, &selectorIndexes[selectorCount]);
                        }
                        selectorCount++;
                    }
                    else return 0;
                }
                break;
            }
        }
        i += iAdvance;
    }

    // No fonts loaded -> default load embedded font
    if (ss->fonts.size == 0) Stylesheet_LoadDefaultFont(ss);

    // Success
    return 1;
}
