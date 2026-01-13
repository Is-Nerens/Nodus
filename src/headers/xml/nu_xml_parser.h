#pragma once
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <datastructures/string.h>
#include <datastructures/linear_stringmap.h>
#include <filesystem/nu_file.h>
#include <utils/nu_convert.h>
#include "stylesheet/nu_stylesheet.h"
#include "nodus.h"


#include "nu_xml_tokens.h"
#include "nu_xml_grammar_assertions.h"
#include "nu_xml_tokeniser.h"

int NU_Generate_Tree(char* src, struct Vector* tokens, struct Vector* textRefs)
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
    Node root_node;
    NU_Apply_Node_Defaults(&root_node);
    root_node.tag = WINDOW;
    Node* root_window_node = NU_Tree_Append(&__NGUI.tree, &root_node, 0);
    AssignRootWindow(&__NGUI.winManager, root_window_node);

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
    uint8_t current_layer = 0; 
    Node* current_node = root_window_node;
    int i = 2; 
    while (i < tokens->size - 3)
    {
        const enum NU_XML_TOKEN token = *((enum NU_XML_TOKEN*) Vector_Get(tokens, i));
        
        // -------------------------
        // New tag -> Add a new node
        // -------------------------
        if (token == OPEN_TAG)
        {
            if (AssertNewTagGrammar(tokens, i))
            {
                // -----------------
                // Enforce tag rules
                // -----------------
                enum Tag tag = NU_Token_To_Tag(*((enum NU_XML_TOKEN*) Vector_Get(tokens, i+1)));
                if (ctx == 1 && tag != ROW && tag != THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! first child of <table> must be <row> or <thead>."); return 0;
                }
                else if (ctx == 2 && tag == THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <table> cannot have multiple <thead>."); return 0;
                }
                else if ((ctx == 2 || ctx == 3) && tag != ROW) {
                    printf("%s\n", "[Generate_Tree] Error! All children of <table> must be <row> (except optional first element <thead>)."); return 0;
                }
                else if (!(ctx == 1 || ctx == 2 || ctx == 3) && tag == ROW) {
                    printf("%s\n", "[Generate_Tree] Error! <row> must have parent of type <table>."); return 0;
                }
                else if (ctx != 1 && tag == THEAD) {
                    printf("%s\n", "[Generate_Tree] Error! <thead> can only be the first child of <table>."); return 0;
                }

                // ----------------------------------------
                // Create a new node and add it to the tree
                // ----------------------------------------
                Node newNode;
                NU_Apply_Node_Defaults(&newNode);
                newNode.tag = tag;
                newNode.parentIndex = __NGUI.tree.layers[current_layer].size - 1;
                current_node = NU_Tree_Append(&__NGUI.tree, &newNode, current_layer+1);

                // ----------------------------------------
                // --- Handle scenarios for different tags
                // ----------------------------------------
                if (current_node->tag == WINDOW) { // If node is a window -> create SDL window
                    CreateSubwindow(&__NGUI.winManager, current_node);
                }
                else if (current_node->tag == TABLE) {
                    current_node->inlineStyleFlags |= 1ULL << 0; // Enforce vertical direction
                    current_node->layoutFlags |= LAYOUT_VERTICAL;
                    ctx = 1;
                } 
                else if (current_node->tag == THEAD) {
                    current_node->inlineStyleFlags |= 1ULL << 1; // Enforce horizontal growth
                    current_node->layoutFlags |= GROW_HORIZONTAL;
                    ctx = 4;
                }
                else if (current_node->tag == ROW) {
                    current_node->inlineStyleFlags |= 1ULL << 1; // Enforce horizontal growth
                    current_node->layoutFlags |= GROW_HORIZONTAL;
                    if (ctx == 1 || ctx == 3) ctx = 6;
                    else ctx = 5;
                }
                else if (current_node->tag == CANVAS) { // Create canvas context
                    NU_Add_Canvas_Context(current_node->handle);
                }

                // -------------------------------
                // Add node to parent's child list
                // -------------------------------
                Node* parent_node = NU_Tree_Get(&__NGUI.tree, current_layer, current_node->parentIndex);
                if (current_node->tag != WINDOW) {
                    current_node->window = parent_node->window; // Inherit window from parent
                } 
                if (parent_node->childCount == 0) {
                    parent_node->firstChildIndex = current_node->index; // If first child in parent
                }
                if (parent_node->tag == ROW || parent_node->tag == THEAD) {
                    current_node->inlineStyleFlags |= 1ULL << 1;
                    current_node->layoutFlags |= GROW_VERTICAL;
                }
                parent_node->childCount += 1;
                parent_node->childCapacity += 1;


                // Move one layer deeper
                current_layer++;
                __NGUI.deepest_layer = MAX(__NGUI.deepest_layer, current_layer);

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
        // Open end tag -> tag closes -> move up one layer
        // -----------------------------------------------
        if (token == OPEN_END_TAG)
        {
            // ---------------------------
            // Enforce close grammar rules
            //----------------------------
            Node* open_node = NU_Tree_Get(&__NGUI.tree, current_layer, __NGUI.tree.layers[current_layer].size - 1);
            enum Tag openTag = open_node->tag;
            if (AssertTagCloseStartGrammar(tokens, i, openTag) != 0) {
                LinearStringmapFree(&imageFilepathToHandleMap);
                return 0;
            }

            // Multi node structure context switch
            if (open_node->tag == TABLE) ctx = 0;      // <table> closes
            else if (open_node->tag == THEAD) ctx = 2; // <thead> closes
            else if (open_node->tag == ROW) {          // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
            } 

            current_layer--; // Move one layer up (towards root)
            i+=3;            // Increment token index
            continue;
        }

        // -----------------------------------------------------
        // Close end tag -> tag self closes -> move up one layer
        // -----------------------------------------------------
        if (token == CLOSE_END_TAG)
        {

            // Multi node structure context switch
            if (current_node->tag == TABLE) ctx = 0;      // <table> closes 
            else if (current_node->tag == THEAD) ctx = 2; // <thead> closes
            else if (current_node->tag == ROW) {          // <row> closes
                if (ctx == 5) ctx = 2;
                else ctx = 3;
            } 

            current_layer--; // Move one layer up (towards root)
            
            // Continue ^
            i+=1; continue;
        }

        // ------------
        // Text content
        // ------------
        if (token == TEXT_CONTENT)
        {
            current_text_ref = Vector_Get(textRefs, text_ref_index++);
            char c = src[current_text_ref->src_index];
            char* text = &src[current_text_ref->src_index];
            src[current_text_ref->src_index + current_text_ref->char_count] = '\0';
            current_node->textContent = StringArena_Add(&__NGUI.node_text_arena, text);

            // Continue ^
            i+=1; continue;
        }

        // ---------------------------------------
        // Property tag -> assign property to node
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
                            current_node->id = StringsetAdd(&__NGUI.id_string_set, ptext);
                            StringmapSet(&__NGUI.id_node_map, ptext, &current_node->handle);
                        }
                        break;

                    // Set class
                    case CLASS_PROPERTY:
                        current_node->class = StringsetAdd(&__NGUI.class_string_set, ptext);
                        break;

                    // Set layout direction
                    case LAYOUT_DIRECTION_PROPERTY:
                        if (c == 'h') {
                            current_node->inlineStyleFlags |= 1ULL << 0;
                        }  
                        else {
                            current_node->inlineStyleFlags |= 1ULL << 0;
                            current_node->layoutFlags |= LAYOUT_VERTICAL;
                        }
                        break;

                    // Set growth
                    case GROW_PROPERTY:
                        switch(c)
                        {
                            case 'v':
                                current_node->inlineStyleFlags |= 1ULL << 1;
                                current_node->layoutFlags |= GROW_VERTICAL;
                                break;
                            case 'h':
                                current_node->inlineStyleFlags |= 1ULL << 1;
                                current_node->layoutFlags |= GROW_HORIZONTAL;
                                break;
                            case 'b':
                                current_node->inlineStyleFlags |= 1ULL << 1;
                                current_node->layoutFlags |= (GROW_HORIZONTAL | GROW_VERTICAL);
                                break;
                        }
                        break;
                    
                    // Set overflow behaviour
                    case OVERFLOW_V_PROPERTY:
                        if (c == 's') {
                            current_node->inlineStyleFlags |= 1ULL << 2;
                            current_node->layoutFlags |= OVERFLOW_VERTICAL_SCROLL;
                        }
                        break;
                    
                    case OVERFLOW_H_PROPERTY:
                        if (c == 's') {
                            current_node->inlineStyleFlags |= 1ULL << 3;
                            current_node->layoutFlags |= OVERFLOW_HORIZONTAL_SCROLL;
                        }                
                        break;

                    // Relative/Absolute positiong
                    case POSITION_PROPERTY:
                        if (current_text_ref->char_count == 8 && memcmp(ptext, "absolute", 8) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 4;
                            current_node->layoutFlags |= POSITION_ABSOLUTE;
                        }
                        break;

                    // Show/hide
                    case HIDE_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "true", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 5;
                            current_node->layoutFlags |= HIDDEN;
                        }
                        break;
                    
                    // Set gap 
                    case GAP_PROPERTY:
                        float property_float;
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 6;
                            current_node->gap = property_float;
                        }
                        break;
                    
                    // Set preferred width
                    case WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 7;
                            current_node->preferred_width = property_float;
                        }
                        break;

                    // Set min width
                    case MIN_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 8;
                            current_node->minWidth = property_float;
                        }
                        break;

                    // Set max width
                    case MAX_WIDTH_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 9;
                           current_node->maxWidth = property_float; 
                        }
                        break;

                    // Set preferred height
                    case HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 10;
                            current_node->preferred_height = property_float;
                        }
                        break;

                    // Set min height
                    case MIN_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 11;
                            current_node->minHeight = property_float;
                        }
                        break;

                    // Set max height
                    case MAX_HEIGHT_PROPERTY:
                        if (String_To_Float(&property_float, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 12;
                            current_node->maxHeight = property_float;
                        }
                        break;

                    // Set horizontal alignment
                    case ALIGN_H_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "left", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 13;
                            current_node->horizontalAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 13;
                            current_node->horizontalAlignment = 1;
                        } else if (current_text_ref->char_count == 5 && memcmp(ptext, "right", 5) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 13;
                            current_node->horizontalAlignment = 2;
                        }
                        break;

                    // Set vertical alignment
                    case ALIGN_V_PROPERTY:
                        if (current_text_ref->char_count == 3 && memcmp(ptext, "top", 3) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 14;
                            current_node->verticalAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 14;
                            current_node->verticalAlignment = 1;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "bottom", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 14;
                            current_node->verticalAlignment = 2;
                        }
                        break;

                    // Set horizontal text alignment
                    case TEXT_ALIGN_H_PROPERTY:
                        if (current_text_ref->char_count == 4 && memcmp(ptext, "left", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 15;
                            current_node->horizontalTextAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 15;
                            current_node->horizontalTextAlignment = 1;
                        } else if (current_text_ref->char_count == 5 && memcmp(ptext, "right", 5) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 15;
                            current_node->horizontalTextAlignment = 2;
                        }
                        break;

                    // Set vertical text alignment
                    case TEXT_ALIGN_V_PROPERTY:
                        if (current_text_ref->char_count == 3 && memcmp(ptext, "top", 3) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 16;
                            current_node->verticalTextAlignment = 0;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "center", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 16;
                            current_node->verticalTextAlignment = 1;
                        } else if (current_text_ref->char_count == 6 && memcmp(ptext, "bottom", 6) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 16;
                            current_node->verticalTextAlignment = 2;
                        }
                        break;

                    // Set absolute positioning
                    case LEFT_PROPERTY:
                        float abs_position; 
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->left = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 17;
                        }
                        break;
                    case RIGHT_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->right = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 18;
                        }
                        break;
                    case TOP_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->top = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 19;
                        }
                        break;
                    case BOTTOM_PROPERTY:
                        if (String_To_Float(&abs_position, ptext)) {
                            current_node->bottom = abs_position;
                            current_node->inlineStyleFlags |= 1ULL << 20;
                        }
                        break;

                    // Set background colour
                    case BACKGROUND_COLOUR_PROPERTY:
                        struct RGB rgb;
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inlineStyleFlags |= 1ULL << 21;
                            current_node->backgroundR = rgb.r;
                            current_node->backgroundG = rgb.g;
                            current_node->backgroundB = rgb.b;
                        } else if (current_text_ref->char_count == 4 && memcmp(ptext, "none", 4) == 0) {
                            current_node->inlineStyleFlags |= 1ULL << 22;
                            current_node->hideBackground = true;
                        }
                        break;

                    // Set border colour
                    case BORDER_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inlineStyleFlags |= 1ULL << 23;
                            current_node->borderR = rgb.r;
                            current_node->borderG = rgb.g;
                            current_node->borderB = rgb.b;
                        }
                        break;

                    // Set text colour
                    case TEXT_COLOUR_PROPERTY:
                        if (Parse_Hexcode(ptext, current_text_ref->char_count, &rgb)) {
                            current_node->inlineStyleFlags |= 1ULL << 24;
                            current_node->textR = rgb.r;
                            current_node->textG = rgb.g;
                            current_node->textB = rgb.b;
                        }
                        break;

                    // Set border width
                    case BORDER_WIDTH_PROPERTY:
                        uint8_t property_uint8;
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 25) | (1ULL << 26) | (1ULL << 27) | (1ULL << 28);
                            current_node->borderTop = property_uint8;
                            current_node->borderBottom = property_uint8;
                            current_node->borderLeft = property_uint8;
                            current_node->borderRight = property_uint8;
                        }
                        break;
                    case BORDER_TOP_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 25;
                            current_node->borderTop = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 26;
                            current_node->borderBottom = property_uint8;
                        }
                        break;
                    case BORDER_LEFT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 27;
                            current_node->borderLeft = property_uint8;
                        }
                        break;
                    case BORDER_RIGHT_WIDTH_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= 1ULL << 28;
                            current_node->borderRight = property_uint8;
                        }
                        break;

                    // Set border radii
                    case BORDER_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 29) | (1ULL << 30) | (1ULL << 31) | ((uint64_t)1ULL << 32);
                            current_node->borderRadiusTl = property_uint8;
                            current_node->borderRadiusTr = property_uint8;
                            current_node->borderRadiusBl = property_uint8;
                            current_node->borderRadiusBr = property_uint8;
                        }
                        break;
                    case BORDER_TOP_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 29);
                            current_node->borderRadiusTl = property_uint8;
                        }
                        break;
                    case BORDER_TOP_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 30);
                            current_node->borderRadiusTr = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_LEFT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= (1ULL << 31);
                            current_node->borderRadiusBl = property_uint8;
                        }
                        break;
                    case BORDER_BOTTOM_RIGHT_RADIUS_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 32);
                            current_node->borderRadiusBr = property_uint8;
                        }
                        break;

                    // Set padding
                    case PADDING_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 33) | ((uint64_t)1ULL << 34) | ((uint64_t)1ULL << 35) | ((uint64_t)1ULL << 36);
                            current_node->padTop    = property_uint8;
                            current_node->padBottom = property_uint8;
                            current_node->padLeft   = property_uint8;
                            current_node->padRight  = property_uint8;
                        }
                        break;
                    case PADDING_TOP_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 33);
                            current_node->padTop = property_uint8;
                        }
                        break;
                    case PADDING_BOTTOM_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 34);
                            current_node->padBottom = property_uint8;
                        }
                        break;
                    case PADDING_LEFT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 35);
                            current_node->padLeft = property_uint8;
                        }
                        break;
                    case PADDING_RIGHT_PROPERTY:
                        if (String_To_uint8_t(&property_uint8, ptext)) {
                            current_node->inlineStyleFlags |= ((uint64_t)1ULL << 36);
                            current_node->padRight = property_uint8;
                        }
                        break;

                    // Image source
                    case IMAGE_SOURCE_PROPERTY:
                        void* found = LinearStringmapGet(&imageFilepathToHandleMap, ptext);
                        if (found == NULL) {
                            GLuint image_handle = Image_Load(ptext);
                            if (image_handle) {
                                current_node->glImageHandle = image_handle;
                                LinearStringmapSet(&imageFilepathToHandleMap, ptext, &image_handle);
                            }
                        } 
                        else { current_node->glImageHandle = *(GLuint*)found; }
                        current_node->inlineStyleFlags |= ((uint64_t)1ULL << 37);
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
    return 1; // Success
}

int NU_Internal_Load_XML(char* filepath)
{
    // Open XML source file and load into buffer
    String src = FileReadUTF8(filepath);
    if (src == NULL) return 0;
    
    // Init token and text ref vectors
    struct Vector tokens;
    struct Vector textRefs;
    Vector_Reserve(&tokens, sizeof(enum NU_XML_TOKEN), 8000);
    Vector_Reserve(&textRefs, sizeof(struct Text_Ref), 2000);

    // Init UI tree 
    NU_Tree_Init(&__NGUI.tree, 100, 10);

    // Tokenise and generate
    NU_Tokenise(src, &tokens, &textRefs); 
    if (!NU_Generate_Tree(StringCstr(src), &tokens, &textRefs)) {
        Vector_Free(&tokens);
        Vector_Free(&textRefs);
        StringFree(src);
        return 0;
    }

    // Free memory
    Vector_Free(&tokens);
    Vector_Free(&textRefs);
    StringFree(src);

    return 1; // Success
}