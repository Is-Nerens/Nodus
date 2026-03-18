#pragma once
#include "nu_stylesheet_grammar_assertions.h"
#include <datastructures/linear_stringmap.h>
#include <datastructures/linear_stringset.h>
#include "../nu_token_array.h"

// You might not like it, but this is what *peak performance looks like
void NU_Stylesheet_Overwrite_Style_Item(NU_Stylesheet_Item* item, NU_Stylesheet_Item* overwriter)
{
    item->propertyFlags |= overwriter->propertyFlags;

    // Branchless layoutFlags update
    item->layoutFlags = (item->layoutFlags & ~LAYOUT_VERTICAL)                 | ((overwriter->layoutFlags & LAYOUT_VERTICAL)                 * !!(overwriter->propertyFlags & PROPERTY_FLAG_LAYOUT_VERTICAL));
    item->layoutFlags = (item->layoutFlags & ~(GROW_HORIZONTAL|GROW_VERTICAL)) | ((overwriter->layoutFlags & (GROW_HORIZONTAL|GROW_VERTICAL)) * !!(overwriter->propertyFlags & PROPERTY_FLAG_GROW));
    item->layoutFlags = (item->layoutFlags & ~OVERFLOW_VERTICAL_SCROLL)        | ((overwriter->layoutFlags & OVERFLOW_VERTICAL_SCROLL)        * !!(overwriter->propertyFlags & PROPERTY_FLAG_VERTICAL_SCROLL));
    item->layoutFlags = (item->layoutFlags & ~OVERFLOW_HORIZONTAL_SCROLL)      | ((overwriter->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)      * !!(overwriter->propertyFlags & PROPERTY_FLAG_HORIZONTAL_SCROLL));
    item->layoutFlags = (item->layoutFlags & ~POSITION_ABSOLUTE)               | ((overwriter->layoutFlags & POSITION_ABSOLUTE)               * !!(overwriter->propertyFlags & PROPERTY_FLAG_POSITION_ABSOLUTE));
    item->layoutFlags = (item->layoutFlags & ~HIDDEN)                          | ((overwriter->layoutFlags & HIDDEN)                          * !!(overwriter->propertyFlags & PROPERTY_FLAG_HIDDEN));
    item->layoutFlags = (item->layoutFlags & ~IGNORE_MOUSE)                    | ((overwriter->layoutFlags & IGNORE_MOUSE)                    * !!(overwriter->propertyFlags & PROPERTY_FLAG_IGNORE_MOUSE));
    item->layoutFlags = (item->layoutFlags & ~HIDE_BACKGROUND)                 | ((overwriter->layoutFlags & HIDE_BACKGROUND) * !!(overwriter->propertyFlags & PROPERTY_FLAG_HIDE_BACKGROUND));

    // Overwrite gap and size fields (branchless)
    item->gap              = item->gap              * !(overwriter->propertyFlags & PROPERTY_FLAG_GAP)              + overwriter->gap              * !!(overwriter->propertyFlags & PROPERTY_FLAG_GAP);
    item->prefWidth  = item->prefWidth  * !(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_WIDTH)  + overwriter->prefWidth  * !!(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_WIDTH);
    item->minWidth         = item->minWidth         * !(overwriter->propertyFlags & PROPERTY_FLAG_MIN_WIDTH)        + overwriter->minWidth         * !!(overwriter->propertyFlags & PROPERTY_FLAG_MIN_WIDTH);
    item->maxWidth         = item->maxWidth         * !(overwriter->propertyFlags & PROPERTY_FLAG_MAX_WIDTH)        + overwriter->maxWidth         * !!(overwriter->propertyFlags & PROPERTY_FLAG_MAX_WIDTH);
    item->prefHeight = item->prefHeight * !(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_HEIGHT) + overwriter->prefHeight * !!(overwriter->propertyFlags & PROPERTY_FLAG_PREFERRED_HEIGHT);
    item->minHeight        = item->minHeight        * !(overwriter->propertyFlags & PROPERTY_FLAG_MIN_HEIGHT)       + overwriter->minHeight        * !!(overwriter->propertyFlags & PROPERTY_FLAG_MIN_HEIGHT);
    item->maxHeight        = item->maxHeight        * !(overwriter->propertyFlags & PROPERTY_FLAG_MAX_HEIGHT)       + overwriter->maxHeight        * !!(overwriter->propertyFlags & PROPERTY_FLAG_MAX_HEIGHT);

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
    item->padTop = item->padTop       * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_TOP)    + overwriter->padTop    * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_TOP);
    item->padBottom = item->padBottom * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_BOTTOM) + overwriter->padBottom * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_BOTTOM);
    item->padLeft = item->padLeft     * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_LEFT)   + overwriter->padLeft   * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_LEFT);
    item->padRight = item->padRight   * !(overwriter->propertyFlags & PROPERTY_FLAG_PAD_RIGHT)  + overwriter->padRight  * !!(overwriter->propertyFlags & PROPERTY_FLAG_PAD_RIGHT);

    // Overwrite image and input type (branchless)
    item->glImageHandle = item->glImageHandle * !(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE)      + overwriter->glImageHandle * !!(overwriter->propertyFlags & PROPERTY_FLAG_IMAGE);
    item->inputType     = item->inputType     * !(overwriter->propertyFlags & PROPERTY_FLAG_INPUT_TYPE) + overwriter->inputType     * !!(overwriter->propertyFlags & PROPERTY_FLAG_INPUT_TYPE);
    
    // Overwrite font Id
    item->fontId = overwriter->fontId;
}

static void NU_Stylesheet_Parse_Property(NU_Stylesheet* ss, const enum NU_Style_Token token, NU_Stylesheet_Item* item, const char* text, int textLen, LinearStringmap* imageFilepathToHandleMap)
{
    char c = text[0];

    switch (token)
    {
        // Set layout direction
        case STYLE_LAYOUT_DIRECTION_PROPERTY:
            if (c == 'v') {
                item->layoutFlags |= LAYOUT_VERTICAL;
                item->propertyFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
            }
            else if (c == 'h') { 
                item->propertyFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
            }
            break;

        // Set growth 
        case STYLE_GROW_PROPERTY:
            switch(c)
            {
                case 'v':
                    item->layoutFlags |= GROW_VERTICAL;
                    break;
                case 'h':
                    item->layoutFlags |= GROW_HORIZONTAL;
                    break;
                case 'b':
                    item->layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                    break;
            }
            item->propertyFlags |= PROPERTY_FLAG_GROW;
            break;
        
        // Set overflow behaviour
        case STYLE_OVERFLOW_V_PROPERTY:
            if (c == 's') {
                item->layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                item->propertyFlags |= PROPERTY_FLAG_VERTICAL_SCROLL;
            }
            else if (c == 'h') {
                item->propertyFlags |= PROPERTY_FLAG_VERTICAL_SCROLL;
            }
            break;
        
        case STYLE_OVERFLOW_H_PROPERTY:
            if (c == 's') {
                item->layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                item->propertyFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL;
            } 
            else if (c == 'h') {
                item->propertyFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL;
            }
            break;

        // Relative/Absolute positioning
        case STYLE_POSITION_PROPERTY:
            if (strcmp(text, "absolute") == 0) {
                item->layoutFlags |= POSITION_ABSOLUTE;
                item->propertyFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
            } else if (strcmp(text, "relative") == 0) {
                item->propertyFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
            }
            break;

        // Hide/show
        case STYLE_HIDE_PROPERTY:
            if (strcmp(text, "true") == 0) {
                item->layoutFlags |= HIDDEN;
                item->propertyFlags |= PROPERTY_FLAG_HIDDEN;
            }
            else if (strcmp(text, "false") == 0) {
                item->propertyFlags |= PROPERTY_FLAG_HIDDEN;
            }
            break;

        // Ignore mouse
        case STYLE_IGNORE_MOUSE_PROPERTY:
            if (strcmp(text, "true") == 0) {
                item->layoutFlags |= IGNORE_MOUSE;
                item->propertyFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
            }
            else if (strcmp(text, "false") == 0) {
                item->propertyFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
            }
            break;
        
        // Set gap
        case STYLE_GAP_PROPERTY:
            if (String_To_u8(&item->gap, text)) 
                item->propertyFlags |= PROPERTY_FLAG_GAP;
            break;

        // Set preferred width
        case STYLE_WIDTH_PROPERTY:
            if (String_To_Uint16(&item->prefWidth, text))
                item->propertyFlags |= PROPERTY_FLAG_PREFERRED_WIDTH;
            break;

        // Set min width
        case STYLE_MIN_WIDTH_PROPERTY:
            if (String_To_Uint16(&item->minWidth, text))
                item->propertyFlags |= PROPERTY_FLAG_MIN_WIDTH;
            break;
        
        // Set max width
        case STYLE_MAX_WIDTH_PROPERTY:
            if (String_To_Uint16(&item->maxWidth, text))
                item->propertyFlags |= PROPERTY_FLAG_MAX_WIDTH;
            break;

        // Set preferred height
        case STYLE_HEIGHT_PROPERTY:
            if (String_To_Uint16(&item->prefHeight, text)) 
                item->propertyFlags |= PROPERTY_FLAG_PREFERRED_HEIGHT;
            break;

        // Set min height
        case STYLE_MIN_HEIGHT_PROPERTY:
            if (String_To_Uint16(&item->minHeight, text)) 
                item->propertyFlags |= PROPERTY_FLAG_MIN_HEIGHT;
            break;

        // Set max height
        case STYLE_MAX_HEIGHT_PROPERTY:
            if (String_To_Uint16(&item->maxHeight, text)) 
                item->propertyFlags |= PROPERTY_FLAG_MAX_HEIGHT;
            break;

        // Set horizontal alignment
        case STYLE_ALIGN_H_PROPERTY:
            if (strcmp(text, "left") == 0) {
                item->horizontalAlignment = 0;
                item->propertyFlags |= PROPERTY_FLAG_ALIGN_H;
            }
            if (strcmp(text, "center") == 0) {
                item->horizontalAlignment = 1;
                item->propertyFlags |= PROPERTY_FLAG_ALIGN_H;
            }
            if (strcmp(text, "right") == 0) {
                item->horizontalAlignment = 2;
                item->propertyFlags |= PROPERTY_FLAG_ALIGN_H;
            }
            break;

        // Set vertical alignment
        case STYLE_ALIGN_V_PROPERTY:
            if (strcmp(text, "top") == 0) {
                item->verticalAlignment = 0;
                item->propertyFlags |= PROPERTY_FLAG_ALIGN_V;
            }
            if (strcmp(text, "center") == 0) {
                item->verticalAlignment = 1;
                item->propertyFlags |= PROPERTY_FLAG_ALIGN_V;
            }
            if (strcmp(text, "bottom") == 0) {
                item->verticalAlignment = 2;
                item->propertyFlags |= PROPERTY_FLAG_ALIGN_V;
            }
            break;

        // Set horizontal text alignment
        case STYLE_TEXT_ALIGN_H_PROPERTY:
            if (strcmp(text, "left") == 0) {
                item->horizontalTextAlignment = 0;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
            }
            if (strcmp(text, "center") == 0) {
                item->horizontalTextAlignment = 1;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
            }
            if (strcmp(text, "right") == 0) {
                item->horizontalTextAlignment = 2;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
            }
            break;

        // Set vertical text alignment
        case STYLE_TEXT_ALIGN_V_PROPERTY:
            if (strcmp(text, "top") == 0) {
                item->verticalTextAlignment = 0;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
            }
            if (strcmp(text, "center") == 0) {
                item->verticalTextAlignment = 1;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
            }
            if (strcmp(text, "bottom") == 0) {
                item->verticalTextAlignment = 2;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
            }
            break;

        // Set absolute position properties
        case STYLE_LEFT_PROPERTY:
            if (String_To_Int16(&item->left, text)) {
                item->propertyFlags |= PROPERTY_FLAG_LEFT;
            }
            break;

        case STYLE_RIGHT_PROPERTY:
            if (String_To_Int16(&item->right, text)) {
                item->propertyFlags |= PROPERTY_FLAG_RIGHT;
            }
            break;

        case STYLE_TOP_PROPERTY:
            if (String_To_Int16(&item->top, text)) {
                item->propertyFlags |= PROPERTY_FLAG_TOP;
            }
            break;

        case STYLE_BOTTOM_PROPERTY:
            if (String_To_Int16(&item->bottom, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BOTTOM;
            }
            break;

        // Set background colour
        case STYLE_BACKGROUND_COLOUR_PROPERTY:
            struct RGB rgb;
            if (Parse_Hexcode(text, textLen, &rgb)) {
                item->backgroundR = rgb.r;
                item->backgroundG = rgb.g;
                item->backgroundB = rgb.b;
                item->propertyFlags |= PROPERTY_FLAG_BACKGROUND;
            } else if (strcmp(text, "none") == 0) {
                item->propertyFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
                item->layoutFlags |= HIDE_BACKGROUND;
            }
            break;

        // Set border colour
        case STYLE_BORDER_COLOUR_PROPERTY:
            if (Parse_Hexcode(text, textLen, &rgb)) {
                item->borderR = rgb.r;
                item->borderG = rgb.g;
                item->borderB = rgb.b;
                item->propertyFlags |= PROPERTY_FLAG_BORDER_COLOUR;
            }
            break;

        // Set text colour
        case STYLE_TEXT_COLOUR_PROPERTY:
            if (Parse_Hexcode(text, textLen, &rgb)) {
                item->textR = rgb.r;
                item->textG = rgb.g;
                item->textB = rgb.b;
                item->propertyFlags |= PROPERTY_FLAG_TEXT_COLOUR;
            }
            break;
        
        // Set border width
        case STYLE_BORDER_WIDTH_PROPERTY:
            u8 border_width;
            if (String_To_u8(&border_width, text)) {
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
        case STYLE_BORDER_TOP_WIDTH_PROPERTY:
            if (String_To_u8(&item->borderTop, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_TOP;
            }
            break;
        case STYLE_BORDER_BOTTOM_WIDTH_PROPERTY:
            if (String_To_u8(&item->borderBottom, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
            }
            break;
        case STYLE_BORDER_LEFT_WIDTH_PROPERTY:
            if (String_To_u8(&item->borderLeft, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_LEFT;
            }
            break;
        case STYLE_BORDER_RIGHT_WIDTH_PROPERTY:
            if (String_To_u8(&item->borderRight, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RIGHT;
            }
            break;

        // Set border radii
        case STYLE_BORDER_RADIUS_PROPERTY:
            u8 border_radius;
            if (String_To_u8(&border_radius, text)) {
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
        case STYLE_BORDER_TOP_LEFT_RADIUS_PROPERTY:
            if (String_To_u8(&item->borderRadiusTl, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
            }
            break;
        case STYLE_BORDER_TOP_RIGHT_RADIUS_PROPERTY:
            if (String_To_u8(&item->borderRadiusTr, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
            }
            break;
        case STYLE_BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
            if (String_To_u8(&item->borderRadiusBl, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
            }
            break;
        case STYLE_BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
            if (String_To_u8(&item->borderRadiusBr, text)) {
                item->propertyFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
            }
            break;

        // Set padding
        case STYLE_PADDING_PROPERTY:
            u8 pad;
            if (String_To_u8(&pad, text)) {
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
        case STYLE_PADDING_TOP_PROPERTY:
            if (String_To_u8(&item->padTop, text)) {
                item->propertyFlags |= PROPERTY_FLAG_PAD_TOP;
            }
            break;
        case STYLE_PADDING_BOTTOM_PROPERTY:
            if (String_To_u8(&item->padBottom, text)) {
                item->propertyFlags |= PROPERTY_FLAG_PAD_BOTTOM;
            }
            break;
        case STYLE_PADDING_LEFT_PROPERTY:
            if (String_To_u8(&item->padLeft, text)) {
                item->propertyFlags |= PROPERTY_FLAG_PAD_LEFT;
            }
            break;
        case STYLE_PADDING_RIGHT_PROPERTY:
            if (String_To_u8(&item->padRight, text)) {
                item->propertyFlags |= PROPERTY_FLAG_PAD_RIGHT;
            }
            break;
        
        case STYLE_IMAGE_SOURCE_PROPERTY:
            void* found = LinearStringmapGet(imageFilepathToHandleMap, text);
            if (found == NULL) {
                GLuint image_handle = Image_Load(text);
                if (image_handle) {
                    item->glImageHandle = image_handle;
                    LinearStringmapSet(imageFilepathToHandleMap, text, &image_handle);
                    item->propertyFlags |= PROPERTY_FLAG_IMAGE;
                }
            } 
            else {
                item->propertyFlags |= PROPERTY_FLAG_IMAGE;
                item->glImageHandle = *(GLuint*)found;
            }
            break;

        case STYLE_FONT_PROPERTY:
            void* found_font = LinearStringmapGet(&ss->fontNameIndexMap, text);
            if (found_font != NULL) {
                item->fontId = *(u8*)found_font;
            }
            break;

        case STYLE_INPUT_TYPE_PROPERTY:
            item->propertyFlags |= PROPERTY_FLAG_INPUT_TYPE;
            if (strcmp(text, "number") == 0) {
                item->inputType = 1;
            } else {
                item->inputType = 0;
            }
            break;

        default:
            break;
    }
}

struct Style_Text_Ref* BinarySearchTextRef(Vector* textRefs, int targetTokenIndex) {
    int left = 0;
    int right = textRefs->size - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        struct Style_Text_Ref* textRef = (struct Style_Text_Ref*)Vector_Get(textRefs, mid);

        if (textRef->NU_Token_index == targetTokenIndex) {
            return textRef;  // found
        } else if (textRef->NU_Token_index < targetTokenIndex) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return NULL;  // not found
}

static int NU_Stylesheet_Parse_Fonts(NU_Stylesheet* ss, char* src, TokenArray* tokens, struct Vector* textRefs)
{
    // State
    int inFontSelector = 0;
    int createFontSize = 18;
    int createFontWeight = 400;
    char* createFontName = NULL;
    char* createFontSrc = NULL;

    int i = 0;
    while(i < tokens->size)
    {
        const enum NU_Style_Token token = TokenArray_Get(tokens, i);

        if (token == STYLE_FONT_CREATION_SELECTOR) {

            if (!AssertFontCreationSelectorGrammar(tokens, i)) return 0;

            struct Style_Text_Ref* textRef = BinarySearchTextRef(textRefs, i+1);

            if (textRef) {
                createFontName = &src[textRef->src_index]; src[textRef->src_index + textRef->char_count] = '\0';
            }

            inFontSelector = 1;
            i += 3; continue;
        }

        else if (NU_Is_Property_Identifier_Token(token)) {

            if (!AssertPropertyIdentifierGrammar(tokens, i)) return 0;

            // Use binary search to find the corresponding property text
            struct Style_Text_Ref* textRef = BinarySearchTextRef(textRefs, i + 2);

            if (textRef) {
                // Get null terminated property text
                char* text = &src[textRef->src_index]; src[textRef->src_index + textRef->char_count] = '\0';

                switch (token)
                {
                    case STYLE_FONT_SRC:
                        createFontSrc = text;
                        break;

                    case STYLE_FONT_SIZE:
                        int size = 0;
                        if (String_To_Int(&size, text)) createFontSize = size;
                        break;

                    case STYLE_FONT_WEIGHT:
                        int weight = 0;
                        if (String_To_Int(&weight, text)) createFontWeight = weight;
                        break;

                    default:
                        break;
                }
            } 

            i += 3; continue;
        }
        else if (token == STYLE_SELECTOR_OPEN_BRACE) {
            if (!AssertSelectionOpeningBraceGrammar(tokens, i)) return 0;
        }
        else if (token == STYLE_SELECTOR_CLOSE_BRACE) {

            if (!AssertSelectionClosingBraceGrammar(tokens, i)) return 0;

            if (inFontSelector) {
                if (createFontSrc != NULL) {
                    
                    // Create a new font
                    void* found_font = LinearStringmapGet(&ss->fontNameIndexMap, createFontName);
                    if (found_font == NULL) {
                        NU_Font font;
                        u8 createFontID = Container_Add(&ss->fonts, &font);
                        NU_Font* createFont = Container_Get(&ss->fonts, createFontID);
                        LinearStringmapSet(&ss->fontNameIndexMap, createFontName, &createFontID);
                        NU_Font_Create(createFont, createFontSrc, createFontSize, true);
                    } 
                }
                else {
                    return 0;
                }
            }

            inFontSelector = 0;
        }
        i += 1;
    }

    return 1;
}

static int NU_Stylesheet_Parse_Default(char* src, TokenArray* tokens, NU_Stylesheet* ss, struct Vector* textRefs, LinearStringmap* imageFilepathToHandleMap)
{
    int inDefaultSelector = 0;

    int i = 0;
    while(i < tokens->size)
    {
        const enum NU_Style_Token token = TokenArray_Get(tokens, i);

        if (token == STYLE_DEFAULT_SELECTOR) {
            if (!AssertDefaultSelectionGrammar(tokens, i)) return 0;

            inDefaultSelector = 1;
        }
        else if (NU_Is_Property_Identifier_Token(token)) {
            if (!AssertPropertyIdentifierGrammar(tokens, i)) return 0;

            if (inDefaultSelector) {
                // Use binary search to find the corresponding property text
                struct Style_Text_Ref* textRef = BinarySearchTextRef(textRefs, i+2);

                if (textRef) {
                    // Get null terminated property text
                    char* text = &src[textRef->src_index]; src[textRef->src_index + textRef->char_count] = '\0';

                    NU_Stylesheet_Parse_Property(ss, token, &ss->defaultStyleItem, text, textRef->char_count, imageFilepathToHandleMap);
                }   
            }
            i += 3; continue;
        }
        else if (token == STYLE_SELECTOR_OPEN_BRACE) {
            if (!AssertSelectionOpeningBraceGrammar(tokens, i)) return 0;
        }
        else if (STYLE_SELECTOR_CLOSE_BRACE) {
            inDefaultSelector = 0;
        }
        i += 1;
    }

    return 1;
}

static int NU_Stylesheet_Parse(char* src, TokenArray* tokens, NU_Stylesheet* ss, struct Vector* textRefs)
{
    // -----------------------
    
    // -----------------------
    // --- (string -> int) ---
    // -----------------------
    LinearStringmap imageFilepathToHandleMap;
    LinearStringmapInit(&imageFilepathToHandleMap, sizeof(GLuint), 20, 512);

    // -------------------
    // --- Parse Fonts ---
    // -------------------
    int succeeded = NU_Stylesheet_Parse_Fonts(ss, src, tokens, textRefs);
    if (!succeeded) {
        LinearStringmapFree(&imageFilepathToHandleMap);
        LinearStringmapFree(&ss->fontNameIndexMap);
        return 0;
    }

    // -------------------------------
    // --- Parse Special Selectors ---
    // -------------------------------
    succeeded = NU_Stylesheet_Parse_Default(src, tokens, ss, textRefs, &imageFilepathToHandleMap);
    if (!succeeded) {
        LinearStringmapFree(&imageFilepathToHandleMap);
        LinearStringmapFree(&ss->fontNameIndexMap);
        return 0;
    }

    // ----------------------
    // --- Parser Context ---
    // ----------------------
    int ctx = 0;                    // 0 == standard selector; 1 == font creation selector; 2 == default selector
    u32 selectorIndexes[64];
    int selectorCount = 0;

    // ------------------------------
    // --- Text Reference Context ---
    // ------------------------------
    u32 textRefIndex = 0;
    struct Style_Text_Ref* textRef;

    // --------------------------
    // --- Working Style Item ---
    // --------------------------
    NU_Stylesheet_Item item;
    item.propertyFlags = 0;
    item.fontId = 0;

    // -------------
    // --- Parse ---
    // -------------
    int i = 0;
    while(i < tokens->size)
    {
        const enum NU_Style_Token token = TokenArray_Get(tokens, i);

        if (token == STYLE_FONT_CREATION_SELECTOR) {
            textRefIndex += 1;
            ctx = 1;
            i += 2;
            continue;
        }
        else if (token == STYLE_DEFAULT_SELECTOR) {
            ctx = 2;
            i += 2;
            continue;
        }
        else if (token == STYLE_SELECTOR_OPEN_BRACE) {
            if (AssertSelectionOpeningBraceGrammar(tokens, i)) {
                item.propertyFlags = 0;
                item.layoutFlags = 0;
                item.fontId = ss->defaultStyleItem.fontId;
                i += 1;
                continue;
            } 
            else {
                succeeded = 0; break;
            }
        }
        else if (token == STYLE_SELECTOR_CLOSE_BRACE) {
            if (!AssertSelectionClosingBraceGrammar(tokens, i)) { succeeded = 0; break; }
                
            if (ctx == 0) {   
                for (int j=0; j<selectorCount; j++) {
                    u32 item_index = selectorIndexes[j];
                    NU_Stylesheet_Item* curr_item = Vector_Get(&ss->items, item_index);

                    // Update current item
                    NU_Stylesheet_Overwrite_Style_Item(curr_item, &item);
                }
                selectorCount = 0;
            }
            
            ctx = 0;
            i += 1;
            continue;
        }
        else if (NU_Is_Tag_Selector_Token(token)) {
            if (i < tokens->size - 1) {
                enum NU_Style_Token next_token = TokenArray_Get(tokens, i+1);
                if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)    
                {
                    int tag = NU_Token_To_Tag(token);
                    void* found = HashmapGet(&ss->tag_item_hashmap, &tag);
                    
                    // Style item exists
                    if (found != NULL) {
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                        selectorIndexes[selectorCount] = *(u32*)found;
                    } 
                    // Style item does not exist -> add one
                    else { 
                        NU_Stylesheet_Item new_item;
                        new_item.class = NULL;
                        new_item.id = NULL;
                        new_item.tag = tag;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selectorIndexes[selectorCount] = (u32)(ss->items.size - 1);
                        HashmapSet(&ss->tag_item_hashmap, &tag, &selectorIndexes[selectorCount]); // Store item index
                    }

                    i += 1;
                    selectorCount++;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size - 3) 
                {
                    enum NU_Style_Token following_token = TokenArray_Get(tokens, i+2);
                    enum NU_Style_Token third_token = TokenArray_Get(tokens, i+3);
                    if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) 
                    {
                        int tag = NU_Token_To_Tag(token);
                        enum NU_Pseudo_Class pseudo_class = Token_To_Pseudo_Class(following_token);

                        // Construct key
                        struct NU_Stylesheet_Tag_Pseudo_Pair key = { tag, pseudo_class };

                        // Query hashmap
                        void* found = HashmapGet(&ss->tag_pseudo_item_hashmap, &key);

                        // Style item exists
                        if (found != NULL)
                        {
                            NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                            selectorIndexes[selectorCount] = *(u32*)found;
                        }
                        // Style item does not exist -> add one
                        else
                        {
                            NU_Stylesheet_Item new_item;
                            new_item.class = NULL;
                            new_item.id = NULL;
                            new_item.tag = tag;
                            new_item.propertyFlags = 0;
                            Vector_Push(&ss->items, &new_item);
                            selectorIndexes[selectorCount] = (u32)(ss->items.size - 1);
                            HashmapSet(&ss->tag_pseudo_item_hashmap, &key, &selectorIndexes[selectorCount]); // Store item index
                        }
                    }
                    else {
                        succeeded = 0; break;
                    }

                    i += 3;
                    selectorCount++;
                    continue;
                }
                else {
                    succeeded = 0; break;
                }
            }
            else {
                succeeded = 0; break;
            }
        }

        else if (token == STYLE_CLASS_SELECTOR)
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = TokenArray_Get(tokens, i+1);
                if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)  
                {
                    // Get class string
                    textRef = (struct Style_Text_Ref*)Vector_Get(textRefs, textRefIndex++);
                    char* src_class = &src[textRef->src_index];
                    src[textRef->src_index + textRef->char_count] = '\0';

                    // Get stored class
                    char* stored_class = LinearStringsetGet(&ss->class_string_set, src_class);

                    // If style item exists
                    void* found = HashmapGet(&ss->class_item_hashmap, &stored_class);
                    if (found != NULL) 
                    {
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                        selectorIndexes[selectorCount] = *(u32*)found;
                    } 
                    else // does not exist -> add item
                    { 
                        // Add class to string set
                        LinearStringsetAdd(&ss->class_string_set, src_class);
                        stored_class = LinearStringsetGet(&ss->class_string_set, src_class);

                        // Add style item for class
                        NU_Stylesheet_Item new_item;
                        new_item.class = stored_class;
                        new_item.id = NULL;
                        new_item.tag = -1;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selectorIndexes[selectorCount] = (u32)(ss->items.size - 1);
                        HashmapSet(&ss->class_item_hashmap, &stored_class, &selectorIndexes[selectorCount]); // Store item index
                    }

                    i += 1;
                    selectorCount++;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size-3) 
                {
                    enum NU_Style_Token following_token = TokenArray_Get(tokens, i+2);
                    enum NU_Style_Token third_token = TokenArray_Get(tokens, i+3);
                    if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) 
                    {
                        enum NU_Pseudo_Class pseudo_class = Token_To_Pseudo_Class(following_token);

                        // Get class string
                        textRef = (struct Style_Text_Ref*)Vector_Get(textRefs, textRefIndex++);
                        char* src_class = &src[textRef->src_index];
                        src[textRef->src_index + textRef->char_count] = '\0';

                        // Get stored class
                        char* stored_class = LinearStringsetGet(&ss->class_string_set, src_class);

                        if (stored_class != NULL)
                        {
                            // Get stored class pseudo
                            struct NU_Stylesheet_String_Pseudo_Pair key = { stored_class, pseudo_class };
                            void* found = HashmapGet(&ss->class_pseudo_item_hashmap, &key);

                            // No pseudo item exists for this class
                            if (found == NULL) 
                            {
                                // Add pseudo style item
                                NU_Stylesheet_Item new_item;
                                new_item.class = stored_class;
                                new_item.id = NULL;
                                new_item.tag = -1;
                                new_item.propertyFlags = 0;
                                Vector_Push(&ss->items, &new_item);
                                selectorIndexes[selectorCount] = (u32)(ss->items.size - 1);
                                HashmapSet(&ss->class_pseudo_item_hashmap, &key, &selectorIndexes[selectorCount]); // Store item index
                            }
                            // Item found
                            else
                            {
                                NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                                selectorIndexes[selectorCount] = *(u32*)found;
                            }
                        }
                    }
                    else {
                        succeeded = 0; break;
                    }

                    i += 3;
                    selectorCount++;
                    continue;
                }
                else {
                    succeeded = 0; break;
                }
            }
            else {
                succeeded = 0; break;
            }
        }

        else if (token == STYLE_ID_SELECTOR)
        {
            if (i < tokens->size - 1)
            {
                enum NU_Style_Token next_token = TokenArray_Get(tokens, i+1);
                if (next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE)  
                {
                    // Get id string
                    textRef = (struct Style_Text_Ref*)Vector_Get(textRefs, textRefIndex++);
                    char* src_id = &src[textRef->src_index];
                    src[textRef->src_index + textRef->char_count] = '\0';

                    // Get stored id
                    char* stored_id = LinearStringsetGet(&ss->id_string_set, src_id);

                    // If style item exists
                    void* found = HashmapGet(&ss->id_item_hashmap, &stored_id);
                    if (found != NULL)
                    {
                        NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                        selectorIndexes[selectorCount] = *(u32*)found;
                    }
                    else // does not exist -> add item
                    {
                        // Add class to string set
                        LinearStringsetAdd(&ss->id_string_set, src_id);
                        stored_id = LinearStringsetGet(&ss->id_string_set, src_id);

                        // Add style item for id
                        NU_Stylesheet_Item new_item;
                        new_item.class = NULL;
                        new_item.id = stored_id;
                        new_item.tag = -1;
                        new_item.propertyFlags = 0;
                        Vector_Push(&ss->items, &new_item);
                        selectorIndexes[selectorCount] = (u32)(ss->items.size - 1);
                        HashmapSet(&ss->id_item_hashmap, &stored_id, &selectorIndexes[selectorCount]); // Store item index
                        LinearStringsetAdd(&ss->id_string_set, src_id);
                    }

                    i += 1;
                    selectorCount++;
                    continue;
                }
                else if (next_token == STYLE_PSEUDO_COLON && i < tokens->size-3) 
                {
                    enum NU_Style_Token following_token = TokenArray_Get(tokens, i+2);
                    enum NU_Style_Token third_token = TokenArray_Get(tokens, i+3);
                    if (NU_Is_Pseudo_Token(following_token) && (third_token == STYLE_SELECTOR_COMMA || third_token == STYLE_SELECTOR_OPEN_BRACE)) 
                    {
                        enum NU_Pseudo_Class pseudo_class = Token_To_Pseudo_Class(following_token);

                        // Get id string
                        textRef = (struct Style_Text_Ref*)Vector_Get(textRefs, textRefIndex++);
                        char* src_id = &src[textRef->src_index];
                        src[textRef->src_index + textRef->char_count] = '\0';

                        // Get stored id
                        char* stored_id = LinearStringsetGet(&ss->id_string_set, src_id);

                        if (stored_id != NULL)
                        {
                            // Get stored id pseudo
                            struct NU_Stylesheet_String_Pseudo_Pair key = { stored_id, pseudo_class };
                            void* found = HashmapGet(&ss->id_pseudo_item_hashmap, &key);

                            // No pseudo item exists for this id
                            if (found == NULL) 
                            {
                                // Add pseudo style item
                                NU_Stylesheet_Item new_item;
                                new_item.class = NULL;
                                new_item.id = stored_id;
                                new_item.tag = -1;
                                new_item.propertyFlags = 0;
                                Vector_Push(&ss->items, &new_item);
                                selectorIndexes[selectorCount] = (u32)(ss->items.size - 1);
                                HashmapSet(&ss->id_pseudo_item_hashmap, &key, &selectorIndexes[selectorCount]); // Store item index
                            }
                            // Item found
                            else
                            {
                                NU_Stylesheet_Item* found_item = Vector_Get(&ss->items, *(u32*)found);
                                selectorIndexes[selectorCount] = *(u32*)found;
                            }
                        }
                    }
                    else {
                        succeeded = 0; break;
                    }

                    i += 3;
                    selectorCount++;
                    continue;
                }
                else {
                    succeeded = 0; break;
                }
            }
            else {
                succeeded = 0; break;
            }
        }

        else if (token == STYLE_SELECTOR_COMMA)
        {
            if (AssertSelectorCommaGrammar(tokens, i)) {
                if (selectorCount == 64) {
                    printf("%s", "[Generate Stylesheet] Error! Too many selectors in one list! max = 64");
                    succeeded = 0; break;
                }
                i += 1;
                continue;
            }
            else {
                succeeded = 0; break;
            }
        }

        else if (NU_Is_Property_Identifier_Token(token))
        {    
            // Property Assertion
            if (!AssertPropertyIdentifierGrammar(tokens, i)) { succeeded = 0; break; }

            // Get property value text
            textRef = (struct Style_Text_Ref*)Vector_Get(textRefs, textRefIndex++);
            char c = src[textRef->src_index];
            char* text = &src[textRef->src_index];
            src[textRef->src_index + textRef->char_count] = '\0';
                
            if (ctx == 0)
            {
                NU_Stylesheet_Parse_Property(ss, token, &item, text, textRef->char_count, &imageFilepathToHandleMap);
            }

            i += 3;
            continue;
        }

        else {
            i += 1; continue;
        }
    }

    // No fonts loaded -> default load embedded font
    if (ss->fonts.size == 0) {
        NU_Font font; 
        if (NU_Font_Create_Default(&font, 14, true)) Container_Add(&ss->fonts, &font); // FontID will be 0
        else succeeded = 0;
    }

    LinearStringmapFree(&imageFilepathToHandleMap);
    LinearStringmapFree(&ss->fontNameIndexMap);
    return succeeded;
}
