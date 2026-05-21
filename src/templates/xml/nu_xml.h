#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <ctype.h>
#include <filesystem/nu_file.h>
#include <utils/nu_convert.h>
#include "../nu_token_array.h"
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

void NU_Parse_Property(const enum NU_XML_TOKEN token, NodeP* currentNode, char* ptext, struct Text_Ref* current_text_ref, ImageResourceLoader* imageResourceLoader)
{
    char c = ptext[0];

    switch (token)
    {
        // Set id
        case ID_PROPERTY:
            char* id_get = StringsetGet(&GUI.id_string_set, ptext);
            if (id_get == NULL) {
                currentNode->id = StringsetAdd(&GUI.id_string_set, ptext);
                StringmapSet(&GUI.id_node_map, ptext, &currentNode);
            }
            break;

        // Set class
        case CLASS_PROPERTY:
            currentNode->class = StringsetAdd(&GUI.class_string_set, ptext);
            break;

        // Set layout direction
        case LAYOUT_DIRECTION_PROPERTY:
            if (c == 'v') {
                currentNode->layoutFlags |= LAYOUT_VERTICAL;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
            }
            else if (c == 'h') { 
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
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
        case GAP_PROPERTY:
            u8 gap;
            if (String_To_u8(&gap, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_GAP;
                currentNode->node.gap = gap;
            }
            break;
        
        // Set preferred width
        case WIDTH_PROPERTY:
            uint16_t width;
            if (String_To_Uint16(&width, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PREFERRED_WIDTH;
                currentNode->node.prefWidth = width;
            }
            break;

        // Set min width
        case MIN_WIDTH_PROPERTY:
            uint16_t minWidth;
            if (String_To_Uint16(&minWidth, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MIN_WIDTH;
                currentNode->node.minWidth = minWidth;
            }
            break;

        // Set max width
        case MAX_WIDTH_PROPERTY:
            uint16_t maxWidth;
            if (String_To_Uint16(&maxWidth, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MAX_WIDTH;
                currentNode->node.maxWidth = maxWidth; 
            }
            break;

        // Set preferred height
        case HEIGHT_PROPERTY:
            uint16_t height;
            if (String_To_Uint16(&height, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PREFERRED_HEIGHT;
                currentNode->node.prefHeight = height;
            }
            break;

        // Set min height
        case MIN_HEIGHT_PROPERTY:
            uint16_t minHeight;
            if (String_To_Uint16(& minHeight, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MIN_HEIGHT;
                currentNode->node.minHeight = minHeight;
            }
            break;

        // Set max height
        case MAX_HEIGHT_PROPERTY:
            uint16_t maxHeight;
            if (String_To_Uint16(&maxHeight, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_MAX_HEIGHT;
                currentNode->node.maxHeight = maxHeight;
            }
            break;

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
        case LEFT_PROPERTY:
            int16_t abs_position; 
            if (String_To_Int16(&abs_position, ptext)) {
                currentNode->node.left = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_LEFT;
            }
            break;
        case RIGHT_PROPERTY:
            if (String_To_Int16(&abs_position, ptext)) {
                currentNode->node.right = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_RIGHT;
            }
            break;
            case TOP_PROPERTY:
            if (String_To_Int16(&abs_position, ptext)) {
                currentNode->node.top = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TOP;
            }
            break;
        case BOTTOM_PROPERTY:
            if (String_To_Int16(&abs_position, ptext)) {
                currentNode->node.bottom = abs_position;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BOTTOM;
            }
            break;

        // Set background colour
        case BACKGROUND_COLOUR_PROPERTY:
            struct RGB rgb;
            if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BACKGROUND;
                currentNode->node.backgroundR = rgb.r;
                currentNode->node.backgroundG = rgb.g;
                currentNode->node.backgroundB = rgb.b;
            } else if (strcmp(ptext, "none") == 0) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
                currentNode->layoutFlags |= HIDE_BACKGROUND;
            }
            break;

        // Set border colour
        case BORDER_COLOUR_PROPERTY:
            if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_COLOUR;
                currentNode->node.borderR = rgb.r;
                currentNode->node.borderG = rgb.g;
                currentNode->node.borderB = rgb.b;
            }
            break;

        // Set text colour
        case TEXT_COLOUR_PROPERTY:
            if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_TEXT_COLOUR;
                currentNode->node.textR = rgb.r;
                currentNode->node.textG = rgb.g;
                currentNode->node.textB = rgb.b;
            }
            break;

        // Set border width
        case BORDER_WIDTH_PROPERTY:
            u8 property_uint8;
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_TOP | PROPERTY_FLAG_BORDER_BOTTOM | PROPERTY_FLAG_BORDER_LEFT | PROPERTY_FLAG_BORDER_RIGHT;
                currentNode->node.borderTop = property_uint8;
                currentNode->node.borderBottom = property_uint8;
                currentNode->node.borderLeft = property_uint8;
                currentNode->node.borderRight = property_uint8;
            }
            break;
        case BORDER_TOP_WIDTH_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_TOP;
                currentNode->node.borderTop = property_uint8;
            }
            break;
        case BORDER_BOTTOM_WIDTH_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
                currentNode->node.borderBottom = property_uint8;
            }
            break;
        case BORDER_LEFT_WIDTH_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_LEFT;
                currentNode->node.borderLeft = property_uint8;
            }
            break;
        case BORDER_RIGHT_WIDTH_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RIGHT;
                currentNode->node.borderRight = property_uint8;
            }
            break;

        // Set border radii
        case BORDER_RADIUS_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL | PROPERTY_FLAG_BORDER_RADIUS_TR | PROPERTY_FLAG_BORDER_RADIUS_BL | PROPERTY_FLAG_BORDER_RADIUS_BR;
                currentNode->node.borderRadiusTl = property_uint8;
                currentNode->node.borderRadiusTr = property_uint8;
                currentNode->node.borderRadiusBl = property_uint8;
                currentNode->node.borderRadiusBr = property_uint8;
            }
            break;
        case BORDER_TOP_LEFT_RADIUS_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
                currentNode->node.borderRadiusTl = property_uint8;
            }
            break;
        case BORDER_TOP_RIGHT_RADIUS_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
                currentNode->node.borderRadiusTr = property_uint8;
            }
            break;
        case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
                currentNode->node.borderRadiusBl = property_uint8;
            }
            break;
        case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
                currentNode->node.borderRadiusBr = property_uint8;
            }
            break;

        // Set padding
        case PADDING_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_TOP | PROPERTY_FLAG_PAD_BOTTOM | PROPERTY_FLAG_PAD_LEFT | PROPERTY_FLAG_PAD_RIGHT;
                currentNode->node.padTop    = property_uint8;
                currentNode->node.padBottom = property_uint8;
                currentNode->node.padLeft   = property_uint8;
                currentNode->node.padRight  = property_uint8;
            }
            break;
        case PADDING_TOP_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_TOP;
                currentNode->node.padTop = property_uint8;
            }
            break;
        case PADDING_BOTTOM_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_BOTTOM;
                currentNode->node.padBottom = property_uint8;
            }
            break;
        case PADDING_LEFT_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_LEFT;
                currentNode->node.padLeft = property_uint8;
            }
            break;
        case PADDING_RIGHT_PROPERTY:
            if (String_To_u8(&property_uint8, ptext)) {
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_PAD_RIGHT;
                currentNode->node.padRight = property_uint8;
            }
            break;

        // Image source
        case IMAGE_SOURCE_PROPERTY:
            if (currentNode->type == NU_CANVAS && currentNode->type == NU_INPUT) break;

            int imageHandle = ImageResourceLoader_GetLoadedImageHandle(imageResourceLoader, ptext);

            // Image not loaded yet
            if (imageHandle == 0) {
                imageHandle = ImageResourceLoader_LoadImage(imageResourceLoader, ptext);
                currentNode->typeData.image.imageHandle = imageHandle;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE;
            }
            else { 
                currentNode->typeData.image.imageHandle = imageHandle;
                currentNode->overrideStyleFlags |= PROPERTY_FLAG_IMAGE;
            }
            break;

        // Input type property
        case INPUT_TYPE_PROPERTY:
            if (currentNode->type != NU_INPUT) break;
            currentNode->overrideStyleFlags |= PROPERTY_FLAG_INPUT_TYPE;
            InputText* inputText = Container_Get(&GUI.textInputs, currentNode->typeData.input.textInputHandle);
            if (strcmp(ptext, "number") == 0) {
                inputText->type = 1;
            } else {
                inputText->type = 0;
            }
            break;  

        default:
            break;
    }
}

int NU_Parse_Component(NodeP* currentNode, char* src, TokenArray* tokens, struct Array* textRefs, ImageResourceLoader* ImageResourceLoader)
{
    enum XMLGenCtx ctx = GENCTX_GLOBAL; 
    struct Text_Ref* current_text_ref;
    if (textRefs->size > 0) current_text_ref = ArrayGet(textRefs, 0);
    uint32_t text_content_ref_index = 0;
    uint32_t text_ref_index = 0;

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
                NodeType type = NU_TokenToNodeType(TokenArray_Get(tokens, i+1));
                if ((ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN || 
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD || 
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) && 
                    type != NU_ROW && 
                    type != NU_THEAD) 
                {
                    printf("%s\n", "[Generate_Tree] Error! children of <table> must be <row> or <thead>.");
                    return 0;
                }
                else if (ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD && type == NU_THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <table> cannot have multiple <thead>.");
                    return 0;
                }
                else if (ctx != GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN && type == NU_THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <thead> must have parent of type <table> and can only be the first child.");
                    return 0;
                }
                else if (!(ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD || 
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) && 
                    type == NU_ROW) 
                {
                    printf("%s\n", "[Generate_Tree] Error! <row> must have parent of type <table>."); 
                    return 0;
                }

                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                currentNode = TreeCreateNode(&GUI.tree, currentNode, type);

                // ----------------------------------------
                // --- Handle scenarios for different tags
                // ----------------------------------------
                if (currentNode->type == NU_WINDOW) { // If node is a window -> create SDL window
                    CreateSubwindow(&GUI.winManager, currentNode);
                }
                else if (currentNode->type == NU_INPUT) {
                    InputText inputText; InputText_Init(&inputText);
                    currentNode->typeData.input.textInputHandle = Container_Add(&GUI.textInputs, &inputText);
                }
                else if (currentNode->type == NU_TABLE) {
                    ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN;
                } 
                else if (currentNode->type == NU_THEAD) {
                    ctx = GENCTX_IN_CONTENT_OF_THEAD;
                }
                else if (currentNode->type == NU_ROW) {
                    if (ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) ctx = GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITHOUT_THEAD;
                    else ctx = GENCTX_IN_CONTENT_OF_ROW_IN_TABLE_WITH_THEAD;
                }
                else if(currentNode->type == NU_IMPORT) {
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
            if (currentNode->type == NU_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes
            else if (currentNode->type == NU_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_ROW) {          // <row> closes
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
            if (currentNode->type == NU_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes 
            else if (currentNode->type == NU_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_ROW) {          // <row> closes
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
            if (currentNode->type == NU_WINDOW || 
                currentNode->type == NU_BOX    ||
                currentNode->type == NU_BUTTON ||
                currentNode->type == NU_IMAGE) 
            {
                current_text_ref = ArrayGet(textRefs, text_ref_index);
                char c = src[current_text_ref->src_index];
                char* text = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';
                currentNode->node.textContent = StringArena_Add(&GUI.nodeTextArena, text);
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
                current_text_ref = ArrayGet(textRefs, text_ref_index++);
                char* ptext = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';

                // Parse property
                NU_Parse_Property(token, currentNode, ptext, current_text_ref, ImageResourceLoader);

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

int NU_Generate_Tree(char* src, TokenArray* tokens, struct Array* textRefs, ImageResourceLoader* imageResourceLoader)
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
    NodeP* rootNode = TreeCreate(&GUI.tree, NU_WINDOW);
    AssignRootWindow(&GUI.winManager, rootNode);

    // ---------------------------------
    // Get first property text reference
    // ---------------------------------
    struct Text_Ref* current_text_ref;
    if (textRefs->size > 0) current_text_ref = ArrayGet(textRefs, 0);
    uint32_t text_content_ref_index = 0;
    uint32_t text_ref_index = 0;





    // ---------------
    // (string -> int)
    // ---------------
    LinearStringmap componentFilepathToTokensMap;
    LinearStringmapInit(&componentFilepathToTokensMap, sizeof(TokenTextRefsPair), 16, 512);

    // -----------------------
    // Iterate over all tokens
    // -----------------------
    enum XMLGenCtx ctx = GENCTX_GLOBAL; 

    NodeP* currentNode = rootNode;
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
                NodeType type = NU_TokenToNodeType(TokenArray_Get(tokens, i+1));
                if ((ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN || 
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD || 
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) && 
                    type != NU_ROW && 
                    type != NU_THEAD) 
                {
                    printf("%s\n", "[Generate_Tree] Error! children of <table> must be <row> or <thead>.");
                    goto error;
                }
                else if (ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD && type == NU_THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <table> cannot have multiple <thead>.");
                    goto error;
                }
                else if (ctx != GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN && type == NU_THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <thead> must have parent of type <table> and can only be the first child.");
                    goto error;
                }
                else if (!(ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN ||
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD || 
                    ctx == GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_THEAD) && 
                    type == NU_ROW) 
                {
                    printf("%s\n", "[Generate_Tree] Error! <row> must have parent of type <table>."); 
                    goto error;
                }


                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                if (type != NU_IMPORT) {
                    currentNode = TreeCreateNode(&GUI.tree, currentNode, type);

                    // ----------------------------------------
                    // --- Handle scenarios for different tags
                    // ----------------------------------------
                    if (currentNode->type == NU_WINDOW) { // If node is a window -> create SDL window
                        CreateSubwindow(&GUI.winManager, currentNode);
                    } else if (currentNode->type == NU_INPUT) {
                        InputText inputText; InputText_Init(&inputText);
                        currentNode->typeData.input.textInputHandle = Container_Add(&GUI.textInputs, &inputText);
                    }
                    else if (currentNode->type == NU_TABLE) {
                        ctx = GENCTX_IN_CONTENT_OF_TABLE_WITHOUT_CHILDREN;
                    } 
                    else if (currentNode->type == NU_THEAD) {
                        ctx = GENCTX_IN_CONTENT_OF_THEAD;
                    }
                    else if (currentNode->type == NU_ROW) {
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
                if (!AssertTagCloseStartGrammar(tokens, i, NU_IMPORT)) goto error;

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
            if (currentNode->type == NU_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes
            else if (currentNode->type == NU_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_ROW) {          // <row> closes
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
            if (currentNode->type == NU_TABLE) ctx = GENCTX_GLOBAL;      // <table> closes 
            else if (currentNode->type == NU_THEAD) ctx = GENCTX_IN_CONTENT_OF_TABLE_WITH_THEAD; // <thead> closes
            else if (currentNode->type == NU_ROW) {          // <row> closes
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
            if (currentNode->type == NU_WINDOW || 
                currentNode->type == NU_BOX    ||
                currentNode->type == NU_BUTTON ||
                currentNode->type == NU_IMAGE) 
            {
                current_text_ref = ArrayGet(textRefs, text_ref_index);
                char c = src[current_text_ref->src_index];
                char* text = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';
                currentNode->node.textContent = StringArena_Add(&GUI.nodeTextArena, text);
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
                current_text_ref = ArrayGet(textRefs, text_ref_index++);
                char* ptext = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';

                // Import src property
                if (ctx != GENCTX_IN_TAG_OF_IMPORT) 
                {
                    // Parse property
                    NU_Parse_Property(token, currentNode, ptext, current_text_ref, imageResourceLoader);
                }
                else
                {
                    if (token == IMPORT_SRC_PROPERTY) 
                    {
                        char* filepath = ptext;
                        String componentSrc = FileReadUTF8(filepath);
                        if (componentSrc) {

                            void* found = LinearStringmapGet(&componentFilepathToTokensMap, filepath);
                        
                            if (!found) {
                                if (!componentSrc) break;

                                // Build (tokens, text refs) pair
                                TokenTextRefsPair tokenTextRefs;
                                tokenTextRefs.tokens = TokenArray_Create(512);
                                ArrayInit(&tokenTextRefs.textRefs, sizeof(struct Text_Ref), 128);

                                // Tokenise component
                                NU_Tokenise(componentSrc, &tokenTextRefs.tokens, &tokenTextRefs.textRefs);

                                // Add tokens to <component filepath, token array> map
                                LinearStringmapSet(&componentFilepathToTokensMap, filepath, &tokenTextRefs);

                                // Parse and build component
                                if (!NU_Parse_Component(currentNode, StringCstr(componentSrc), &tokenTextRefs.tokens, &tokenTextRefs.textRefs, imageResourceLoader)) {
                                    StringFree(componentSrc);
                                    goto error;
                                }
                            }
                            else {
                                TokenTextRefsPair* tokenTextRefs = (TokenTextRefsPair*)found;

                                // Parse and build component
                                if (!NU_Parse_Component(currentNode, StringCstr(componentSrc), &tokenTextRefs->tokens, &tokenTextRefs->textRefs, imageResourceLoader)) {
                                    StringFree(componentSrc);
                                    goto error;
                                }
                            }

                            StringFree(componentSrc);
                        }    
                        else {
                            printf("component file not found\n");
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

    LinearStringmapFree(&componentFilepathToTokensMap);
    return 1;


error:
    // Failure 
    LinearStringmapFree(&componentFilepathToTokensMap);
    return 0;
}

int NU_Internal_Load_XML(const char* filepath, ImageResourceLoader* imageResourceLoader)
{
    // Open XML source file and load into buffer
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;
    
    // Init token and text ref vectors
    TokenArray tokens = TokenArray_Create(8000);
    struct Array textRefs; ArrayInit(&textRefs, sizeof(struct Text_Ref), 2000);

    // Tokenise and generate
    NU_Tokenise(src, &tokens, &textRefs); 
    if (!NU_Generate_Tree(StringCstr(src), &tokens, &textRefs, imageResourceLoader)) {
        TokenArray_Free(&tokens);
        ArrayFree(&textRefs);
        StringFree(src);
        return 0;
    }

    // Free memory
    TokenArray_Free(&tokens);
    ArrayFree(&textRefs);
    StringFree(src);
    return 1;
}