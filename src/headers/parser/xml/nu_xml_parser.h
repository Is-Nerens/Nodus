#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <ctype.h>
#include <filesystem/nu_file.h>
#include <utils/nu_convert.h>
#include "../nu_token_array.h"
#include "nu_xml_tokens.h"
#include "nu_xml_grammar_assertions.h"
#include "nu_xml_tokeniser.h"

int NU_Generate_Tree(char* src, TokenArray* tokens, struct Vector* textRefs)
{
    // --------------------
    // Enforce root grammar
    // --------------------
    if (AssertRootGrammar(tokens) != 0) {
        return 0; // Failure 
    }

    // -----------------------
    // Create root window node
    // -----------------------
    NodeP* rootNode = TreeCreate(&__NGUI.tree, NU_WINDOW);
    AssignRootWindow(&__NGUI.winManager, rootNode);

    // ---------------------------------
    // Get first property text reference
    // ---------------------------------
    struct Text_Ref* current_text_ref;
    if (textRefs->size > 0) current_text_ref = Vector_Get(textRefs, 0);
    uint32_t text_content_ref_index = 0;
    uint32_t text_ref_index = 0;


    // ---------------
    // (string -> int)
    // ---------------
    LinearStringmap imageFilepathToHandleMap;
    LinearStringmapInit(&imageFilepathToHandleMap, sizeof(GLuint), 20, 512);

    // -----------------------
    // Iterate over all tokens
    // -----------------------
    int ctx = 0; 
    // 0 = default
    // 1 = <table> just opened
    // 2 = in <table> with <thead>
    // 3 = in table without <thead>
    // 4 = in <thead>
    // 5 = in <row> in <table> with <thead>
    // 6 = in <row> in <table> without <thead>
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
                if (ctx == 1 && type != NU_ROW && type != NU_THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! first child of <table> must be <row> or <thead>."); return 0;
                }
                else if (ctx == 2 && type == NU_THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <table> cannot have multiple <thead>."); return 0;
                }
                else if ((ctx == 2 || ctx == 3) && type != NU_ROW) {
                    printf("%s\n", "[Generate_Tree] Error! All children of <table> must be <row> (except optional first element <thead>)."); return 0;
                }
                else if (!(ctx == 1 || ctx == 2 || ctx == 3) && type == NU_ROW) {
                    printf("%s\n", "[Generate_Tree] Error! <row> must have parent of type <table>."); return 0;
                }
                else if (ctx != 1 && type == NU_THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <thead> can only be the first child of <table>."); return 0;
                }

                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                currentNode = TreeCreateNode(&__NGUI.tree, currentNode, type);
                // ----------------------------------------
                // --- Handle scenarios for different tags
                // ----------------------------------------
                if (currentNode->type == NU_WINDOW) { // If node is a window -> create SDL window
                    CreateSubwindow(&__NGUI.winManager, currentNode);
                }
                else if (currentNode->type == NU_TABLE) {
                    ctx = 1;
                } 
                else if (currentNode->type == NU_THEAD) {
                    ctx = 4;
                }
                else if (currentNode->type == NU_ROW) {
                    if (ctx == 1 || ctx == 3) ctx = 6;
                    else ctx = 5;
                }
                else if (currentNode->type == NU_CANVAS) { // Create canvas context
                    NU_Add_Canvas_Context(&currentNode->node);
                }

                // Continue ^
                i+=2; continue;
            }
            else
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                return 0;
            }
        }

        // -----------------------------------------------
        // Open end type -> type closes -> move up one layer
        // -----------------------------------------------
        if (token == OPEN_END_TAG)
        {
            // ---------------------------
            // Enforce close grammar rules
            //----------------------------
            if (AssertTagCloseStartGrammar(tokens, i, currentNode->type) != 0) {
                LinearStringmapFree(&imageFilepathToHandleMap);
                return 0;
            }

            // Multi node structure context switch
            if (currentNode->type == NU_TABLE) ctx = 0;      // <table> closes
            else if (currentNode->type == NU_THEAD) ctx = 2; // <thead> closes
            else if (currentNode->type == NU_ROW) {          // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
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
            // Multi node structure context switch
            if (currentNode->type == NU_TABLE) ctx = 0;      // <table> closes 
            else if (currentNode->type == NU_THEAD) ctx = 2; // <thead> closes
            else if (currentNode->type == NU_ROW) {          // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
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
                current_text_ref = Vector_Get(textRefs, text_ref_index);
                char c = src[current_text_ref->src_index];
                char* text = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';
                currentNode->node.textContent = StringArena_Add(&__NGUI.node_text_arena, text);
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
            if (AssertPropertyGrammar(tokens, i) == 0)
            {
                // -----------------------
                // Get property value text
                // -----------------------
                current_text_ref = Vector_Get(textRefs, text_ref_index++);
                char c = src[current_text_ref->src_index];
                char* ptext = &src[current_text_ref->src_index];
                src[current_text_ref->src_index + current_text_ref->char_count] = '\0';

                // Set the node property
                switch (token)
                {
                    // Set id
                    case ID_PROPERTY:
                        char* id_get = StringsetGet(&__NGUI.id_string_set, ptext);
                        if (id_get == NULL) {
                            currentNode->node.id = StringsetAdd(&__NGUI.id_string_set, ptext);
                            StringmapSet(&__NGUI.id_node_map, ptext, &currentNode);
                        }
                        break;

                    // Set class
                    case CLASS_PROPERTY:
                        currentNode->node.class = StringsetAdd(&__NGUI.class_string_set, ptext);
                        break;

                    // Set layout direction
                    case LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'v') {
                            currentNode->node.layoutFlags |= LAYOUT_VERTICAL;
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
                        }
                        else if (c == 'h') { 
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_LAYOUT_VERTICAL;
                        }
                        break;

                    // Set growth
                    case GROW_PROPERTY:
                        switch(c)
                        {
                            case 'v':
                                currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_GROW;
                                currentNode->node.layoutFlags |= GROW_VERTICAL;
                                break;
                            case 'h':
                                currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_GROW;
                                currentNode->node.layoutFlags |= GROW_HORIZONTAL;
                                break;
                            case 'b':
                                currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_GROW;
                                currentNode->node.layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                                break;
                        }
                        break;
                    
                    // Set overflow behaviour
                    case OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_VERTICAL_SCROLL;
                            currentNode->node.layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                        }
                        break;
                    
                    case OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_HORIZONTAL_SCROLL;
                            currentNode->node.layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                        }                
                        break;

                    // Relative/Absolute positiong
                    case POSITION_PROPERTY:
                        if (strcmp(ptext, "absolute") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_POSITION_ABSOLUTE;
                            currentNode->node.layoutFlags |= POSITION_ABSOLUTE;
                        }
                        break;

                    // Show/hide
                    case HIDE_PROPERTY:
                        if (strcmp(ptext, "true") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_HIDDEN;
                            currentNode->node.layoutFlags |= HIDDEN;
                        }
                        break;

                    // Ignore mouse
                    case IGNORE_MOUSE_PROPERTY:
                        if (strcmp(ptext, "true") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_IGNORE_MOUSE;
                            currentNode->node.layoutFlags |= IGNORE_MOUSE;
                        }
                        break;
                    
                    // Set gap 
                    case GAP_PROPERTY:
                        float property_float;
                        if (String_To_Float(&property_float, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_GAP;
                            currentNode->node.gap = property_float;
                        }
                        break;
                    
                    // Set preferred width
                    case WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_PREFERRED_WIDTH;
                            currentNode->node.preferred_width = property_float;
                        }
                        break;

                    // Set min width
                    case MIN_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_MIN_WIDTH;
                            currentNode->node.minWidth = property_float;
                        }
                        break;

                    // Set max width
                    case MAX_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_MAX_WIDTH;
                           currentNode->node.maxWidth = property_float; 
                        }
                        break;

                    // Set preferred height
                    case HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_PREFERRED_HEIGHT;
                            currentNode->node.preferred_height = property_float;
                        }
                        break;

                    // Set min height
                    case MIN_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_MIN_HEIGHT;
                            currentNode->node.minHeight = property_float;
                        }
                        break;

                    // Set max height
                    case MAX_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_MAX_HEIGHT;
                            currentNode->node.maxHeight = property_float;
                        }
                        break;

                    // Set horizontal alignment
                    case ALIGN_H_PROPERTY:
                        if (strcmp(ptext, "left") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_ALIGN_H;
                            currentNode->node.horizontalAlignment = 0;
                        } else if (strcmp(ptext, "center") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_ALIGN_H;
                            currentNode->node.horizontalAlignment = 1;
                        } else if (strcmp(ptext, "right") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_ALIGN_H;
                            currentNode->node.horizontalAlignment = 2;
                        }
                        break;

                    // Set vertical alignment
                    case ALIGN_V_PROPERTY:
                        if (strcmp(ptext, "top") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_ALIGN_V;
                            currentNode->node.verticalAlignment = 0;
                        } else if (strcmp(ptext, "center") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_ALIGN_V;
                            currentNode->node.verticalAlignment = 1;
                        } else if (strcmp(ptext, "bottom") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_ALIGN_V;
                            currentNode->node.verticalAlignment = 2;
                        }
                        break;

                    // Set horizontal text alignment
                    case TEXT_ALIGN_H_PROPERTY:
                        if (strcmp(ptext, "left") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                            currentNode->node.horizontalTextAlignment = 0;
                        } else if (strcmp(ptext, "center") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                            currentNode->node.horizontalTextAlignment = 1;
                        } else if (strcmp(ptext, "right") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_H;
                            currentNode->node.horizontalTextAlignment = 2;
                        }
                        break;

                    // Set vertical text alignment
                    case TEXT_ALIGN_V_PROPERTY:
                        if (strcmp(ptext, "top") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                            currentNode->node.verticalTextAlignment = 0;
                        } else if (strcmp(ptext, "center") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                            currentNode->node.verticalTextAlignment = 1;
                        } else if (strcmp(ptext, "bottom") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TEXT_ALIGN_V;
                            currentNode->node.verticalTextAlignment = 2;
                        }
                        break;

                    // Set absolute positioning
                    case LEFT_PROPERTY:
                        float abs_position; 
                        if (String_To_Float(&abs_position, ptext)) {
                            currentNode->node.left = abs_position;
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_LEFT;
                        }
                        break;
                    case RIGHT_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            currentNode->node.right = abs_position;
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_RIGHT;
                        }
                        break;
                     case TOP_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            currentNode->node.top = abs_position;
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TOP;
                        }
                        break;
                    case BOTTOM_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            currentNode->node.bottom = abs_position;
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BOTTOM;
                        }
                        break;

                    // Set background colour
                    case BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BACKGROUND;
                            currentNode->node.backgroundR = rgb.r;
                            currentNode->node.backgroundG = rgb.g;
                            currentNode->node.backgroundB = rgb.b;
                        } else if (strcmp(ptext, "none") == 0) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_HIDE_BACKGROUND;
                            currentNode->node.layoutFlags |= HIDE_BACKGROUND;
                        }
                        break;

                    // Set border colour
                    case BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_COLOUR;
                            currentNode->node.borderR = rgb.r;
                            currentNode->node.borderG = rgb.g;
                            currentNode->node.borderB = rgb.b;
                        }
                        break;

                    // Set text colour
                    case TEXT_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_TEXT_COLOUR;
                            currentNode->node.textR = rgb.r;
                            currentNode->node.textG = rgb.g;
                            currentNode->node.textB = rgb.b;
                        }
                        break;

                    // Set border width
                    case BORDER_WIDTH_PROPERTY:
                        uint8_t property_uint8;
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_TOP | PROPERTY_FLAG_BORDER_BOTTOM | PROPERTY_FLAG_BORDER_LEFT | PROPERTY_FLAG_BORDER_RIGHT;
                            currentNode->node.borderTop = property_uint8;
                            currentNode->node.borderBottom = property_uint8;
                            currentNode->node.borderLeft = property_uint8;
                            currentNode->node.borderRight = property_uint8;
                        }
                        break;
                    case BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_TOP;
                            currentNode->node.borderTop = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_BOTTOM;
                            currentNode->node.borderBottom = property_uint8;
                        }
                        break;
                    case BORDER_LEFT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_LEFT;
                            currentNode->node.borderLeft = property_uint8;
                        }
                        break;
                    case BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_RIGHT;
                            currentNode->node.borderRight = property_uint8;
                        }
                        break;

                    // Set border radii
                    case BORDER_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL | PROPERTY_FLAG_BORDER_RADIUS_TR | PROPERTY_FLAG_BORDER_RADIUS_BL | PROPERTY_FLAG_BORDER_RADIUS_BR;
                            currentNode->node.borderRadiusTl = property_uint8;
                            currentNode->node.borderRadiusTr = property_uint8;
                            currentNode->node.borderRadiusBl = property_uint8;
                            currentNode->node.borderRadiusBr = property_uint8;
                        }
                        break;
                    case BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TL;
                            currentNode->node.borderRadiusTl = property_uint8;
                        }
                        break;
                    case BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_TR;
                            currentNode->node.borderRadiusTr = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_BL;
                            currentNode->node.borderRadiusBl = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_BORDER_RADIUS_BR;
                            currentNode->node.borderRadiusBr = property_uint8;
                        }
                        break;

                    // Set padding
                    case PADDING_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_PAD_TOP | PROPERTY_FLAG_PAD_BOTTOM | PROPERTY_FLAG_PAD_LEFT | PROPERTY_FLAG_PAD_RIGHT;
                            currentNode->node.padTop    = property_uint8;
                            currentNode->node.padBottom = property_uint8;
                            currentNode->node.padLeft   = property_uint8;
                            currentNode->node.padRight  = property_uint8;
                        }
                        break;
                    case PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_PAD_TOP;
                            currentNode->node.padTop = property_uint8;
                        }
                        break;
                    case PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_PAD_BOTTOM;
                            currentNode->node.padBottom = property_uint8;
                        }
                        break;
                    case PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_PAD_LEFT;
                            currentNode->node.padLeft = property_uint8;
                        }
                        break;
                    case PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_PAD_RIGHT;
                            currentNode->node.padRight = property_uint8;
                        }
                        break;

                    // Image source
                    case IMAGE_SOURCE_PROPERTY:
                        if (currentNode->type == NU_INPUT) break;
                        void* found = LinearStringmapGet(&imageFilepathToHandleMap, ptext);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(ptext);
                            if (image_handle) {
                                currentNode->typeData.image.glImageHandle = image_handle;
                                LinearStringmapSet(&imageFilepathToHandleMap, ptext, &image_handle);
                                currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_IMAGE;
                            }
                        } 
                        else { 
                            currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_IMAGE;
                            currentNode->typeData.image.glImageHandle = *(GLuint*)found; 
                        }
                        break;

                    // Input type property
                    case INPUT_TYPE_PROPERTY:
                        if (currentNode->type != NU_INPUT) break;
                        currentNode->node.inlineStyleFlags |= PROPERTY_FLAG_INPUT_TYPE;
                        if (strcmp(ptext, "number") == 0) {
                            currentNode->typeData.input.inputText.type = 1;
                        } else {
                            currentNode->typeData.input.inputText.type = 0;
                        }
                        break;  

                    default:
                        break;
                }

                // Continue ^
                i+=3; continue;
            }
            else // Failure 
            {
                LinearStringmapFree(&imageFilepathToHandleMap);
                return 0;
            }
        }

        // Continue ^
        i+=1;
    }
    LinearStringmapFree(&imageFilepathToHandleMap);
    return 1;
}

int NU_Internal_Load_XML(char* filepath)
{
    // Open XML source file and load into buffer
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;
    
    // Init token and text ref vectors
    TokenArray tokens = TokenArray_Create(8000);
    struct Vector textRefs; Vector_Reserve(&textRefs, sizeof(struct Text_Ref), 2000);

    // Tokenise and generate
    NU_Tokenise(src, &tokens, &textRefs); 
    if (!NU_Generate_Tree(StringCstr(src), &tokens, &textRefs)) {
        TokenArray_Free(&tokens);
        Vector_Free(&textRefs);
        StringFree(src);
        return 0;
    }

    // Free memory
    TokenArray_Free(&tokens);
    Vector_Free(&textRefs);
    StringFree(src);
    return 1;
}