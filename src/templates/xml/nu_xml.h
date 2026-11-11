#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <utils/nu_file.h>
#include "nu_xml_tokens.h"
#include "nu_xml_grammar_assertions.h"
#include "nu_xml_tokeniser.h"


enum XMLGenCtx
{
    GENCTX_GLOBAL,                                    // 0 = default
    GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN,      // 1 = in <table></table> with <thead>
    GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD,            // 2 = in <table></table> with <thead>
    GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD,         // 3 = in <table></table> without <thead>
    GENCTX_IN_CONTENT_OF_THEAD,                       // 4 = in <thead></thead>
    GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD,     // 5 = in <row></row> in <table></table> with <thead></thead>
    GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITHOUT_THEAD,  // 6 = in <row></row> in <table></table> without <thead></thead>
    GENCTX_IN_TAG_OF_IMPORT,                          // 7 = in <import> tag
};

typedef struct
{
    TokenArray tokens;
    Array textRefs;
} TokenTextRefsPair;

static bool XML_ParseHexcode(const char* hexcode, uint32 length, NuColor* result)
{
    if (!hexcode || !result) return false;
    uint32 offset = (hexcode[0] == '#') ? 1 : 0;
    uint32 hexLength = length - offset;
    if (hexLength != 6 && hexLength != 8) return false;
    hexcode += offset;
    int32 r1 = HexValue(hexcode[0]);
    int32 r2 = HexValue(hexcode[1]);
    int32 g1 = HexValue(hexcode[2]);
    int32 g2 = HexValue(hexcode[3]);
    int32 b1 = HexValue(hexcode[4]);
    int32 b2 = HexValue(hexcode[5]);
    if (r1 < 0 || r2 < 0 ||
        g1 < 0 || g2 < 0 ||
        b1 < 0 || b2 < 0) {
        return false;
    }
    uint8 alpha = 255;
    if (hexLength == 8) {
        int32 a1 = HexValue(hexcode[6]);
        int32 a2 = HexValue(hexcode[7]);
        if (a1 < 0 || a2 < 0) return false;
        alpha = (uint8)((a1 << 4) | a2);
    }
    *result = (NuColor){
        (uint8)((r1 << 4) | r2),
        (uint8)((g1 << 4) | g2),
        (uint8)((b1 << 4) | b2),
        alpha
    };
    return true;
}


void NU_Parse_Property(const enum NU_XML_TOKEN token, Node* currentNode, char* ptext, struct Text_Ref* current_text_ref)
{
    char c = ptext[0];

    switch (token)
    {
        // Set id
        case ID_PROPERTY: {
            char* id_get = Stringset_Get(&GUI.id_string_set, ptext);
            if (id_get == NULL) {
                currentNode->id = Stringset_Add(&GUI.id_string_set, ptext);
                Stringmap_Set(&GUI.id_node_map, ptext, &currentNode);
            }
            break;
        }

        // Set class
        case CLASS_PROPERTY:
            currentNode->class = Stringset_Add(&GUI.class_string_set, ptext);
            break;

        // Set layout direction
        case LAYOUT_DIRECTION_PROPERTY:
            if (c == 'v') {
                currentNode->layoutFlags |= LAYOUT_VERTICAL;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_LAYOUT_DIR;
            }
            else if (c == 'h') {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_LAYOUT_DIR;
            }
            break;

        // Set growth
        case GROW_PROPERTY:
            switch(c)
            {
                case 'v':
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_GROW;
                    currentNode->layoutFlags |= GROW_VERTICAL;
                    break;
                case 'h':
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_GROW;
                    currentNode->layoutFlags |= GROW_HORIZONTAL;
                    break;
                case 'b':
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_GROW;
                    currentNode->layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                default:
                    if (stringEquals(ptext, "none")) {
                        currentNode->overrideStyleFlags |= PROPERTY_FLAG_GROW;
                        currentNode->layoutFlags &= ~(GROW_HORIZONTAL | GROW_VERTICAL);
                    }
                    break;
            }
            break;

        // Set overflow behaviour
        case OVERFLOW_V_PROPERTY:
            if (c == 's') {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_VERTICAL_SCROLL;
                currentNode->layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
            }
            break;

        case OVERFLOW_H_PROPERTY:
            if (c == 's') {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL;
                currentNode->layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
            }
            break;

        // Relative/Absolute positiong
        case POSITION_PROPERTY:
            if (strcmp(ptext, "absolute") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
                currentNode->layoutFlags |= POSITION_ABSOLUTE;
            }
            break;

        // Show/hide
        case HIDE_PROPERTY:
            if (strcmp(ptext, "true") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_HIDDEN;
                currentNode->layoutFlags |= HIDDEN;
            }
            break;

        // Ignore mouse
        case IGNORE_MOUSE_PROPERTY:
            if (strcmp(ptext, "true") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
                currentNode->layoutFlags |= IGNORE_MOUSE;
            }
            break;

        // Set gap
        case GAP_PROPERTY: {
            uint8 gap;
            if (CStringToUint8(&gap, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_GAP;
                currentNode->base.gap = gap;
            }
            break;
        }

        // Set preferred width
        case WIDTH_PROPERTY:
        {
            uint16 width;
            if (CStringToUint16(&width, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PREFERRED_WIDTH;
                currentNode->base.preferredWidth = width;

                if (currentNode->type == NU_TYPE_IMAGE) {
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
                    currentNode->special.image.widthDimType = IMAGE_DIM_SPECIFIC;
                }
            }
            else if (currentNode->type == NU_TYPE_IMAGE && strcmp(ptext, "image") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
                currentNode->special.image.widthDimType = IMAGE_DIM_INTRINSIC;
            }
            else if (currentNode->type == NU_TYPE_IMAGE && strcmp(ptext, "aspect") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_WIDTH_DIM;
                currentNode->special.image.widthDimType = IMAGE_DIM_ASPECT;
            }
            break;
        }

        // Set min width
        case MIN_WIDTH_PROPERTY: {
            uint16 minWidth;
            if (CStringToUint16(&minWidth, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MIN_WIDTH;
                currentNode->base.minWidth = minWidth;
            }
            break;
        }

        // Set max width
        case MAX_WIDTH_PROPERTY: {
            uint16 maxWidth;
            if (CStringToUint16(&maxWidth, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MAX_WIDTH;
                currentNode->base.maxWidth = maxWidth;
            }
            break;
        }

        // Set preferred height
        case HEIGHT_PROPERTY: {
            uint16 height;
            if (CStringToUint16(&height, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PREFERRED_HEIGHT;
                currentNode->base.preferredHeight = height;
                if (currentNode->type == NU_TYPE_IMAGE) {
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
                    currentNode->special.image.heightDimType = IMAGE_DIM_SPECIFIC;
                }
            }
            else if (currentNode->type == NU_TYPE_IMAGE && strcmp(ptext, "image") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
                currentNode->special.image.heightDimType = IMAGE_DIM_INTRINSIC;
            }
            else if (currentNode->type == NU_TYPE_IMAGE && strcmp(ptext, "aspect") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_HEIGHT_DIM;
                currentNode->special.image.heightDimType = IMAGE_DIM_ASPECT;
            }
            break;
        }

        // Set min height
        case MIN_HEIGHT_PROPERTY: {
            uint16 minHeight;
            if (CStringToUint16(& minHeight, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MIN_HEIGHT;
                currentNode->base.minHeight = minHeight;
            }
            break;
        }

        // Set max height
        case MAX_HEIGHT_PROPERTY: {
            uint16 maxHeight;
            if (CStringToUint16(&maxHeight, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MAX_HEIGHT;
                currentNode->base.maxHeight = maxHeight;
            }
            break;
        }

        // Set horizontal alignment
        case ALIGN_H_PROPERTY:
            if (strcmp(ptext, "left") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_ALIGN_H;
                currentNode->horizontalAlignment = 0;
            } else if (strcmp(ptext, "center") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_ALIGN_H;
                currentNode->horizontalAlignment = 1;
            } else if (strcmp(ptext, "right") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_ALIGN_H;
                currentNode->horizontalAlignment = 2;
            }
            break;

        // Set vertical alignment
        case ALIGN_V_PROPERTY:
            if (strcmp(ptext, "top") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_ALIGN_V;
                currentNode->verticalAlignment = 0;
            } else if (strcmp(ptext, "center") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_ALIGN_V;
                currentNode->verticalAlignment = 1;
            } else if (strcmp(ptext, "bottom") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_ALIGN_V;
                currentNode->verticalAlignment = 2;
            }
            break;

        // Set horizontal text alignment
        case TEXT_ALIGN_H_PROPERTY:
            if (strcmp(ptext, "left") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                currentNode->horizontalTextAlignment = 0;
            } else if (strcmp(ptext, "center") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                currentNode->horizontalTextAlignment = 1;
            } else if (strcmp(ptext, "right") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                currentNode->horizontalTextAlignment = 2;
            }
            break;

        // Set vertical text alignment
        case TEXT_ALIGN_V_PROPERTY:
            if (strcmp(ptext, "top") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                currentNode->verticalTextAlignment = 0;
            } else if (strcmp(ptext, "center") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                currentNode->verticalTextAlignment = 1;
            } else if (strcmp(ptext, "bottom") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                currentNode->verticalTextAlignment = 2;
            }
            break;

        // Set absolute positioning
        case LEFT_PROPERTY: {
            int16_t abs_position;
            if (CStringToInt16(&abs_position, ptext)) {
                currentNode->base.left = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_LEFT;
            }
            break;
        }

        case RIGHT_PROPERTY: {
            int16_t abs_position;
            if (CStringToInt16(&abs_position, ptext)) {
                currentNode->base.right = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_RIGHT;
            }
            break;
            case TOP_PROPERTY:
            if (CStringToInt16(&abs_position, ptext)) {
                currentNode->base.top = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TOP;
            }
            break;
        }

        case BOTTOM_PROPERTY: {
            int16_t abs_position;
            if (CStringToInt16(&abs_position, ptext)) {
                currentNode->base.bottom = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BOTTOM;
            }
            break;
        }

        // Set background colour
        case BACKGROUND_COLOUR_PROPERTY: {
            NuColor color;
            if (XML_ParseHexcode(ptext, current_text_ref->char_count, &color)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BACKGROUND;
                currentNode->overrideStyleFlags &= ~PROPERTY_FLAG_HIDE_BACKGROUND;
                currentNode->layoutFlags &= ~HIDE_BACKGROUND;
                currentNode->base.backgroundR = color.r;
                currentNode->base.backgroundG = color.g;
                currentNode->base.backgroundB = color.b;
            } else if (strcmp(ptext, "none") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
                currentNode->layoutFlags |= HIDE_BACKGROUND;
            }
            break;
        }

        // Set border colour
        case BORDER_COLOUR_PROPERTY: {
            NuColor color;
            if (XML_ParseHexcode(ptext, current_text_ref->char_count, &color)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_COLOUR;
                currentNode->base.borderR = color.r;
                currentNode->base.borderG = color.g;
                currentNode->base.borderB = color.b;
            }
            break;
        }

        // Set text colour
        case TEXT_COLOUR_PROPERTY: {
            NuColor color;
            if (NODE_CAN_HAVE_TEXT_CONTENT(currentNode) && XML_ParseHexcode(ptext, current_text_ref->char_count, &color)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_COLOUR;
                currentNode->special.text.textR = color.r;
                currentNode->special.text.textG = color.g;
                currentNode->special.text.textB = color.b;
            }
            break;
        }

        // Set border width
        case BORDER_WIDTH_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_TOP | PROPERTY_FLAG_BORDER_BOTTOM | PROPERTY_FLAG_BORDER_LEFT | PROPERTY_FLAG_BORDER_RIGHT;
                currentNode->base.borderTop = property_uint8;
                currentNode->base.borderBottom = property_uint8;
                currentNode->base.borderLeft = property_uint8;
                currentNode->base.borderRight = property_uint8;
            }
            break;
        }
        case BORDER_TOP_WIDTH_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_TOP;
                currentNode->base.borderTop = property_uint8;
            }
            break;
        }
        case BORDER_BOTTOM_WIDTH_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
                currentNode->base.borderBottom = property_uint8;
            }
            break;
        }
        case BORDER_LEFT_WIDTH_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_LEFT;
                currentNode->base.borderLeft = property_uint8;
            }
            break;
        }
        case BORDER_RIGHT_WIDTH_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RIGHT;
                currentNode->base.borderRight = property_uint8;
            }
            break;
        }

        // Set border radii
        case BORDER_RADIUS_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL | PROPERTY_FLAG_BORDER_RADIUS_TR | PROPERTY_FLAG_BORDER_RADIUS_BL | PROPERTY_FLAG_BORDER_RADIUS_BR;
                currentNode->base.borderRadiusTl = property_uint8;
                currentNode->base.borderRadiusTr = property_uint8;
                currentNode->base.borderRadiusBl = property_uint8;
                currentNode->base.borderRadiusBr = property_uint8;
            }
            break;
        }
        case BORDER_TOP_LEFT_RADIUS_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
                currentNode->base.borderRadiusTl = property_uint8;
            }
            break;
        }
        case BORDER_TOP_RIGHT_RADIUS_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
                currentNode->base.borderRadiusTr = property_uint8;
            }
            break;
        }
        case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
                currentNode->base.borderRadiusBl = property_uint8;
            }
            break;
        }
        case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
                currentNode->base.borderRadiusBr = property_uint8;
            }
            break;
        }

        // Set padding
        case PADDING_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_TOP | PROPERTY_FLAG_PAD_BOTTOM | PROPERTY_FLAG_PAD_LEFT | PROPERTY_FLAG_PAD_RIGHT;
                currentNode->base.padTop    = property_uint8;
                currentNode->base.padBottom = property_uint8;
                currentNode->base.padLeft   = property_uint8;
                currentNode->base.padRight  = property_uint8;
            }
            break;
        }
        case PADDING_TOP_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_TOP;
                currentNode->base.padTop = property_uint8;
            }
            break;
        }
        case PADDING_BOTTOM_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_BOTTOM;
                currentNode->base.padBottom = property_uint8;
            }
            break;
        }
        case PADDING_LEFT_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_LEFT;
                currentNode->base.padLeft = property_uint8;
            }
            break;
        }
        case PADDING_RIGHT_PROPERTY: {
            uint8 property_uint8;
            if (CStringToUint8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_RIGHT;
                currentNode->base.padRight = property_uint8;
            }
            break;
        }

        // Image source
        case IMAGE_SOURCE_PROPERTY: {
            if (currentNode->type != NU_TYPE_IMAGE) break;

            int imageHandle = ImageResourceManager_ImageHandleFromFilepath(&GUI.imageResourceManager, ptext);

            // Image not loaded yet
            if (imageHandle == 0) {
                imageHandle = ImageResourceManager_LoadImage(&GUI.imageResourceManager, &GUI.displayManager, ptext);
                currentNode->special.image.imageHandle = imageHandle;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE;
            }
            else {
                currentNode->special.image.imageHandle = imageHandle;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE;
            }
            break;
        }

        case IMAGE_REPEAT_PROPERTY: {
            if (currentNode->type != NU_TYPE_IMAGE) break;
            switch (c) {
                case 'h': {
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_REPEAT;
                    currentNode->special.image.repeat = IMAGE_REPEAT_HORIZONTAL;
                    break;
                }
                case 'v': {
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_REPEAT;
                    currentNode->special.image.repeat = IMAGE_REPEAT_VERTICAL;
                    break;
                }
                case 'b': {
                    currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_REPEAT;
                    currentNode->special.image.repeat = IMAGE_REPEAT_BOTH;
                    break;
                }
                default: break;
            }
            break;
        }

        case IMAGE_FILTER_PROPERTY: {
            if (currentNode->type != NU_TYPE_IMAGE) break;
            if (strcmp(ptext, "linear") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_FILTER;
                currentNode->special.image.filter = IMAGE_FILTER_LINEAR;
            }
            else if (strcmp(ptext, "nearest") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE_FILTER;
                currentNode->special.image.filter = IMAGE_FILTER_NEAREST;
            }
            break;
        }

        // Input type property
        case INPUT_TYPE_PROPERTY: {
            if (currentNode->type != NU_TYPE_INPUT) break;
            currentNode->overrideStyleFlags |= PROPERTY_FLAG_INPUT_TYPE;
            InputText* inputText = Container_Get(&GUI.textInputs, currentNode->special.input.textInputHandle);
            if (strcmp(ptext, "number") == 0) {
                inputText->type = 1;
            } else {
                inputText->type = 0;
            }
            break;
        }

        case TITLE_PROPERTY:
            if (currentNode->type != NU_TYPE_WINDOW) break;
            SDL_SetWindowTitle(DisplayManager_GetNodeSDLWindow(&GUI.displayManager, currentNode), ptext);
            break;

        default:
            break;
    }
}

int NU_Parse_Component(
    Node* currentNode,
    char* src,
    TokenArray* tokens,
    Array* textRefs,
    ImageResourceManager* imageResourceManager,
    DisplayManager* displayManager)
{
    enum XMLGenCtx ctx = GENCTX_GLOBAL;
    struct Text_Ref* current_text_ref;
    if (textRefs->size > 0) current_text_ref = Array_Get(textRefs, 0);
    uint32 text_content_ref_index = 0;
    uint32 text_ref_index = 0;

    int i = 0;
    while(i < tokens->size - 3)
    {
        const enum NU_XML_TOKEN token = TokenArray_Get(tokens, i);

        // -------------------------
        // New type -> Add a new node
        // -------------------------
        if (token == OPEN_TAG)
        {
            if (AssertNewTagGrammar(tokens, i))
            {
                // -----------------
                // Enforce type rules
                // -----------------
                uint8 type = NU_TokenToNodeType(TokenArray_Get(tokens, i+1));
                if ((ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) &&
                    type != NU_TYPE_ROW &&
                    type != NU_TYPE_THEAD)
                {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error> children of <table> must be <row> or <thead>");
                    return 0;
                }
                else if (ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD && type == NU_TYPE_THEAD) {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error> <table> cannot have multiple <thead>");
                    return 0;
                }
                else if (ctx != GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN && type == NU_TYPE_THEAD) {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error> <thead> must have parent of type <table> and can only be the first child");
                    return 0;
                }
                else if (!(ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) &&
                    type == NU_TYPE_ROW)
                {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error> <row> must have parent of type <table>");
                    return 0;
                }

                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                currentNode = TreeCreateNode(&GUI.tree, currentNode, type);

                // ----------------------------------------
                // --- Handle scenarios for different tags
                // ----------------------------------------
                if (currentNode->type == NU_TYPE_WINDOW) { // If node is a window -> create SDL window
                    DisplayManager_CreateWindow(&GUI.displayManager, currentNode, "", 100, 100);
                }
                else if (currentNode->type == NU_TYPE_INPUT) {
                    InputText inputText; InputText_Init(&inputText);
                    currentNode->special.input.textInputHandle = Container_Add(&GUI.textInputs, &inputText);
                }
                else if (currentNode->type == NU_TYPE_TABLE) {
                    ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN;
                }
                else if (currentNode->type == NU_TYPE_THEAD) {
                    ctx = GENCTX_IN_CONTENT_OF_THEAD;
                }
                else if (currentNode->type == NU_TYPE_ROW) {
                    if (ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) ctx = GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITHOUT_THEAD;
                    else ctx = GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD;
                }
                else if(currentNode->type == NU_TYPE_IMPORT) {
                    ctx = GENCTX_IN_TAG_OF_IMPORT;
                }

                // Continue ^
                i+=2; continue;
            }
            else return 0;
        }

        // -----------------------------------------------
        // Open end type -> type closes -> move up one layer
        // -----------------------------------------------
        if (token == OPEN_END_TAG)
        {
            // ---------------------------
            // Enforce close grammar rules
            //----------------------------
            if (!AssertTagCloseStartGrammar(tokens, i, currentNode->type)) {
                return 0;
            }

            // Context switch
            if (currentNode->type == NU_TYPE_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes
            else if (currentNode->type == NU_TYPE_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_TYPE_ROW) {          // <row> closes
                if (ctx == GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD;
                else ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD;
            }

            currentNode = currentNode->parent;
            i+=3;            // Increment token index
            continue;
        }

        // -----------------------------------------------------
        // Close end type -> type self closes -> move up one layer
        // -----------------------------------------------------
        if (token == CLOSE_END_TAG)
        {
            // Context switch
            if (currentNode->type == NU_TYPE_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes
            else if (currentNode->type == NU_TYPE_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_TYPE_ROW) {          // <row> closes
                if (ctx == GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD;
                else ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD;
            }

            currentNode = currentNode->parent;

            // Continue ^
            i+=1; continue;
        }

        // ------------
        // Text content
        // ------------
        if (token == TEXT_CONTENT)
        {
            if (NODE_CAN_HAVE_TEXT_CONTENT(currentNode))
            {
                current_text_ref = Array_Get(textRefs, text_ref_index);
                char c = src[current_text_ref->src_index];
                char* text = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';
                currentNode->special.text.textContent = StringArena_Add(&GUI.nodeTextArena, text);
            }

            // Continue ^
            text_ref_index+=1; i+=1; continue;
        }

        // ---------------------------------------
        // Property type -> assign property to node
        // ---------------------------------------
        if (NU_Is_Token_Property(token))
        {
            // ---------------------------------------------
            // Enforce property grammar [property = "value"]
            // ---------------------------------------------
            if (AssertPropertyGrammar(tokens, i))
            {
                // -----------------------
                // Get property value text
                // -----------------------
                current_text_ref = Array_Get(textRefs, text_ref_index++);
                char* ptext = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';

                // Parse property
                NU_Parse_Property(token, currentNode, ptext, current_text_ref);

                // Continue ^
                i+=3; continue;
            }
            else return 0;
        }

        // Continue ^
        i+=1;
    }

    return 1;
}

int NU_Generate_Tree(
    char* src,
    TokenArray* tokens,
    Array* textRefs,
    ImageResourceManager* imageResourceManager,
    DisplayManager* displayManager)
{
    // --------------------
    // Enforce root grammar
    // --------------------
    if (!AssertRootGrammar(tokens)) {
        return 0; // Failure
    }

    // -----------------------
    // Create root window node
    // -----------------------
    Node* rootNode = TreeCreate(&GUI.tree);
    DisplayManager_CreateWindow(&GUI.displayManager, rootNode, "", 100, 100);

    // ---------------------------------
    // Get first property text reference
    // ---------------------------------
    struct Text_Ref* current_text_ref;
    if (textRefs->size > 0) current_text_ref = Array_Get(textRefs, 0);
    uint32 text_content_ref_index = 0;
    uint32 text_ref_index = 0;





    // ---------------
    // (string -> int)
    // ---------------
    LinearStringmap componentFilepathToTokensMap;
    LinearStringmap_Init(&componentFilepathToTokensMap, sizeof(TokenTextRefsPair), 16, 512);

    // -----------------------
    // Iterate over all tokens
    // -----------------------
    enum XMLGenCtx ctx = GENCTX_GLOBAL;

    Node* currentNode = rootNode;
    int i = 2;
    while (i < tokens->size - 3)
    {
        const enum NU_XML_TOKEN token = TokenArray_Get(tokens, i);

        // -------------------------
        // New type -> Add a new node
        // -------------------------
        if (token == OPEN_TAG)
        {
            if (AssertNewTagGrammar(tokens, i))
            {
                // -----------------
                // Enforce type rules
                // -----------------
                uint8 type = NU_TokenToNodeType(TokenArray_Get(tokens, i+1));
                if ((ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) &&
                    type != NU_TYPE_ROW &&
                    type != NU_TYPE_THEAD)
                {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error> children of <table> must be <row> or <thead>");
                    goto error;
                }
                else if (ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD && type == NU_TYPE_THEAD) {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error> <table> cannot have multiple <thead>");
                    goto error;
                }
                else if (ctx != GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN && type == NU_TYPE_THEAD) {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error> <thead> must have parent of type <table> and can only be the first child");
                    goto error;
                }
                else if (!(ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) &&
                    type == NU_TYPE_ROW)
                {
                    ErrorSystem_AddError(&GUI.errorSystem, "<XML Error><row> must have parent of type <table>");
                    goto error;
                }


                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                if (type != NU_TYPE_IMPORT) {
                    currentNode = TreeCreateNode(&GUI.tree, currentNode, type);

                    // ----------------------------------------
                    // --- Handle scenarios for different tags
                    // ----------------------------------------
                    if (currentNode->type == NU_TYPE_WINDOW) { // If node is a window -> create SDL window
                        DisplayManager_CreateWindow(&GUI.displayManager, currentNode, "", 100, 100);
                    } else if (currentNode->type == NU_TYPE_INPUT) {
                        InputText inputText; InputText_Init(&inputText);
                        currentNode->special.input.textInputHandle = Container_Add(&GUI.textInputs, &inputText);
                    }
                    else if (currentNode->type == NU_TYPE_TABLE) {
                        ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN;
                    }
                    else if (currentNode->type == NU_TYPE_THEAD) {
                        ctx = GENCTX_IN_CONTENT_OF_THEAD;
                    }
                    else if (currentNode->type == NU_TYPE_ROW) {
                        if (ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) ctx = GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITHOUT_THEAD;
                        else ctx = GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD;
                    }
                }
                else {
                    ctx = GENCTX_IN_TAG_OF_IMPORT;
                }


                // Continue ^
                i+=2; continue;
            }
            else {
                goto error;
            }
        }

        // -----------------------------------------------
        // Open end type -> type closes -> move up one layer
        // -----------------------------------------------
        if (token == OPEN_END_TAG)
        {
            // Maintain depth if tag was of type <import>
            if (ctx == GENCTX_IN_TAG_OF_IMPORT) {

                // ---------------------------
                // Enforce close grammar rules
                //----------------------------
                if (!AssertTagCloseStartGrammar(tokens, i, NU_TYPE_IMPORT)) goto error;

                ctx = GENCTX_GLOBAL;
                i+=3;            // Increment token index
                continue;
            }

            // ---------------------------
            // Enforce close grammar rules
            //----------------------------
            if (!AssertTagCloseStartGrammar(tokens, i, currentNode->type)) {
                goto error;
            }

            // Context switch
            if (currentNode->type == NU_TYPE_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes
            else if (currentNode->type == NU_TYPE_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_TYPE_ROW) {          // <row> closes
                if (ctx == GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD;
                else ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD;
            }

            currentNode = currentNode->parent;
            i+=3;            // Increment token index
            continue;
        }

        // -----------------------------------------------------
        // Close end type -> type self closes -> move up one layer
        // -----------------------------------------------------
        if (token == CLOSE_END_TAG)
        {
            // Maintain depth if tag was of type <import>
            if (ctx == GENCTX_IN_TAG_OF_IMPORT) {
                ctx = GENCTX_GLOBAL;
                i+=3;            // Increment token index
                continue;
            }

            // Context switch
            if (currentNode->type == NU_TYPE_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes
            else if (currentNode->type == NU_TYPE_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_TYPE_ROW) {          // <row> closes
                if (ctx == GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD;
                else ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD;
            }

            currentNode = currentNode->parent;

            // Continue ^
            i+=1; continue;
        }

        // ------------
        // Text content
        // ------------
        if (token == TEXT_CONTENT)
        {
            if (NODE_CAN_HAVE_TEXT_CONTENT(currentNode))
            {
                current_text_ref = Array_Get(textRefs, text_ref_index);
                char c = src[current_text_ref->src_index];
                char* text = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';
                currentNode->special.text.textContent = StringArena_Add(&GUI.nodeTextArena, text);
            }

            // Continue ^
            text_ref_index+=1; i+=1; continue;
        }

        // ---------------------------------------
        // Property type -> assign property to node
        // ---------------------------------------
        if (NU_Is_Token_Property(token))
        {
            // ---------------------------------------------
            // Enforce property grammar [property = "value"]
            // ---------------------------------------------
            if (AssertPropertyGrammar(tokens, i))
            {
                // -----------------------
                // Get property value text
                // -----------------------
                current_text_ref = Array_Get(textRefs, text_ref_index++);
                char* ptext = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';

                // Import src property
                if (ctx != GENCTX_IN_TAG_OF_IMPORT)
                {
                    // Parse property
                    NU_Parse_Property(token, currentNode, ptext, current_text_ref);
                }
                else
                {
                    if (token == IMPORT_SRC_PROPERTY)
                    {
                        char* filepath = ptext;
                        String componentSrc = FileReadUTF8(filepath);
                        if (componentSrc) {

                            void* found = LinearStringmap_Get(&componentFilepathToTokensMap, filepath);

                            if (!found) {
                                if (!componentSrc) break;

                                // Build (tokens, text refs) pair
                                TokenTextRefsPair tokenTextRefs;
                                tokenTextRefs.tokens = TokenArray_Create(512);
                                Array_Init(&tokenTextRefs.textRefs, sizeof(struct Text_Ref), 128);

                                // Tokenise component
                                NU_Tokenise(componentSrc, &tokenTextRefs.tokens, &tokenTextRefs.textRefs);

                                // Add tokens to <component filepath, token array> map
                                LinearStringmap_Set(&componentFilepathToTokensMap, filepath, &tokenTextRefs);

                                // Parse and build component
                                if (!NU_Parse_Component(currentNode, StringCstr(componentSrc), &tokenTextRefs.tokens, &tokenTextRefs.textRefs, imageResourceManager, displayManager)) {
                                    StringFree(componentSrc);
                                    goto error;
                                }
                            }
                            else {
                                TokenTextRefsPair* tokenTextRefs = (TokenTextRefsPair*)found;

                                // Parse and build component
                                if (!NU_Parse_Component(currentNode, StringCstr(componentSrc), &tokenTextRefs->tokens, &tokenTextRefs->textRefs, imageResourceManager, displayManager)) {
                                    StringFree(componentSrc);
                                    goto error;
                                }
                            }

                            StringFree(componentSrc);
                        }
                        else {
                            ErrorSystem_AddError(&GUI.errorSystem, "<XML Error><row> XML component file not found");
                        }
                    }
                }

                // Continue ^
                i+=3; continue;
            }
            else {
                goto error;
            }
        }

        // Continue ^
        i+=1;
    }

    LinearStringmap_Free(&componentFilepathToTokensMap);
    return 1;


error:
    // Failure
    LinearStringmap_Free(&componentFilepathToTokensMap);
    return 0;
}

int NU_Internal_Load_XML(const char* filepath, ImageResourceManager* imageResourceManager, DisplayManager* displayManager)
{
    // Open XML source file and load into buffer
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;

    // Init token and text ref vectors
    TokenArray tokens = TokenArray_Create(8000);
    struct Array textRefs; Array_Init(&textRefs, sizeof(struct Text_Ref), 2000);

    // Tokenise
    NU_Tokenise(src, &tokens, &textRefs);

    // Generate tree
    if (!NU_Generate_Tree(StringCstr(src), &tokens, &textRefs, imageResourceManager, displayManager)) {
        TokenArray_Free(&tokens);
        Array_Free(&textRefs);
        StringFree(src);
        return 0;
    }

    // Free memory
    TokenArray_Free(&tokens);
    Array_Free(&textRefs);
    StringFree(src);
    return 1;
}
