#pragma once

// ---------------------------------------
// --- Macros to reduce code verbosity ---
// ---------------------------------------
#include "nu_stylesheet_structs.h"
#include "nu_stylesheet_tokens.h"
#define STYLE_APPLY_LAYOUT_FLAG(prop, layout_mask) if ((item->propertyFlags & (prop)) && !(node->overrideStyleFlags & (prop))) node->layoutFlags = (node->layoutFlags & ~(layout_mask)) | (item->layoutFlags & (layout_mask))
#define STYLE_SHOULD_APPLY_TO_NODE(mask) (item->propertyFlags & mask) && !(node->overrideStyleFlags & mask)

// This should be optimised (branchless)
static void Stylesheet_ApplyStyleItemToNode(StyleItem* item, NodeP* node)
{
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_LAYOUT_DIR, LAYOUT_VERTICAL);
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_GROW, GROW_HORIZONTAL);
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_GROW, GROW_VERTICAL);
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_VERTICAL_SCROLL, OVERFLOW_VERTICAL_SCROLL);     // Overflow vertical scroll (or not)
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_HORIZONTAL_SCROLL, OVERFLOW_HORIZONTAL_SCROLL); // Overflow horizontal scroll (or not)
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_POSITION_ABSOLUTE, POSITION_ABSOLUTE);          // Absolute positioning (or not)
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_HIDDEN, HIDDEN);                                // Hidden or not
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_IGNORE_MOUSE, IGNORE_MOUSE);                    // Ignore mouse or not
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_HIDE_BACKGROUND, HIDE_BACKGROUND);              // Hide background (or not)
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_GAP)) node->node.gap = item->gap;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PREFERRED_WIDTH)) node->node.prefWidth = item->prefWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MIN_WIDTH)) node->node.minWidth = item->minWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MAX_WIDTH)) node->node.maxWidth = item->maxWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PREFERRED_HEIGHT)) node->node.prefHeight = item->prefHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MIN_HEIGHT)) node->node.minHeight = item->minHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MAX_HEIGHT)) node->node.maxHeight = item->maxHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_ALIGN_H)) node->horizontalAlignment = item->horizontalAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_ALIGN_V)) node->verticalAlignment = item->verticalAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_ALIGN_H)) node->horizontalTextAlignment = item->horizontalTextAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_ALIGN_V)) node->verticalTextAlignment = item->verticalTextAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_LEFT)) node->node.left = item->left;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_RIGHT)) node->node.right = item->right;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TOP)) node->node.top = item->top;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BOTTOM)) node->node.bottom = item->bottom;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BACKGROUND)) {
        node->node.backgroundR = item->backgroundR;
        node->node.backgroundG = item->backgroundG;
        node->node.backgroundB = item->backgroundB;
        node->layoutFlags &= ~(HIDE_BACKGROUND * (!(node->overrideStyleFlags & PROPERTY_FLAG_HIDE_BACKGROUND) && !(item->propertyFlags & PROPERTY_FLAG_HIDE_BACKGROUND)));
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_COLOUR)) {
        node->node.borderR = item->borderR;
        node->node.borderG = item->borderG;
        node->node.borderB = item->borderB;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_COLOUR)) {
        node->node.textR = item->textR;
        node->node.textG = item->textG;
        node->node.textB = item->textB;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_TOP)) node->node.borderTop = item->borderTop;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_BOTTOM)) node->node.borderBottom = item->borderBottom;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_LEFT)) node->node.borderLeft = item->borderLeft;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RIGHT)) node->node.borderRight = item->borderRight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_TL)) node->node.borderRadiusTl = item->borderRadiusTl;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_TR)) node->node.borderRadiusTr = item->borderRadiusTr;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_BL)) node->node.borderRadiusBl = item->borderRadiusBl;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_BR)) node->node.borderRadiusBr = item->borderRadiusBr;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_TOP)) node->node.padTop = item->padTop;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_BOTTOM)) node->node.padBottom = item->padBottom;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_LEFT)) node->node.padLeft = item->padLeft;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_RIGHT)) node->node.padRight = item->padRight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IMAGE) && node->type != NU_CANVAS && node->type != NU_INPUT) {
        node->typeData.image.imageHandle = item->imageHandle;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_INPUT_TYPE) && node->type == NU_INPUT) {
        InputText* inputText = Container_Get(&GUI.textInputs, node->typeData.input.textInputHandle);
        inputText->type = item->inputType;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_FONT)) node->fontId = item->fontId;
}

void Stylesheet_ApplyVariations(Stylesheet* ss, StyleItem* item, NodeP* node, int screenWidth)
{
    for (int i=0; i<ss->screenQueries.size; i++)
    {
        StylesheetScreenQuery* query = Array_Get(&ss->screenQueries, i);
        bool applies = false;
        applies |= query->comparator == STYLE_GREATER       && screenWidth > query->screenWidth;
        applies |= query->comparator == STYLE_LESS          && screenWidth < query->screenWidth;
        applies |= query->comparator == STYLE_GREATER_EQUAL && screenWidth >= query->screenWidth;
        applies |= query->comparator == STYLE_LESS_EQUAL    && screenWidth <= query->screenWidth;
        if (item->nextVariationIndex != -1 && item->screenQueryIndex < i) {
            item = Array_Get(&ss->items, item->nextVariationIndex);
        }
        if (applies && item->screenQueryIndex == i) {
            Stylesheet_ApplyStyleItemToNode(item, node);
        }
    }
}

void Stylesheet_ApplyStyleToNode(Stylesheet* ss, NodeP* node)
{
    // Get containing screen width
    NU_Window* win = GetNodeWindow(&GUI.winManager, node);
    int screenWidth = win->screenWidth;

    // Create keys
    StyleItemKey tagItemKey = {
        .classID = -1,
        .idID = -1,
        .tag = node->type,
        .pseudoClass = -1
    };
    StyleItemKey classItemKey = {
        .classID = -1,
        .idID = -1,
        .tag = -1,
        .pseudoClass = -1
    };
    StyleItemKey idItemKey = {
        .classID = -1,
        .idID = -1,
        .tag = -1,
        .pseudoClass = -1
    };

    // Get the style items
    StyleItem* defaultItem = Array_Get(&ss->items, 0);
    StyleItem* tagItem = NULL;
    StyleItem* classItem = NULL;
    StyleItem* idItem = NULL;

    int* itemIndex = Hashmap_Get(&ss->itemIndexMap, &tagItemKey);
    if (itemIndex) {
        tagItem = Array_Get(&ss->items, *itemIndex);
    }
    if (node->class) {
        int* classID = LinearStringmap_Get(&ss->classIdMap, node->class);
        if (classID) {
            classItemKey.classID = *classID;
            itemIndex = Hashmap_Get(&ss->itemIndexMap, &classItemKey);
            if (itemIndex) {
                classItem = Array_Get(&ss->items, *itemIndex);
            }
        }
    }
    if (node->id) {
        int* idID = LinearStringmap_Get(&ss->idIdMap, node->id);
        if (idID) {
            idItemKey.idID = *idID;
            itemIndex = Hashmap_Get(&ss->itemIndexMap, &idItemKey);
            if (itemIndex) {
                idItem = Array_Get(&ss->items, *itemIndex);
            }
        }
    }

    // Apply style items in order
    Stylesheet_ApplyStyleItemToNode(defaultItem, node);
    Stylesheet_ApplyVariations(ss, defaultItem, node, screenWidth);
    if (tagItem) {
        Stylesheet_ApplyStyleItemToNode(tagItem, node);
        Stylesheet_ApplyVariations(ss, tagItem, node, screenWidth);
    }
    if (classItem) {
        Stylesheet_ApplyStyleItemToNode(classItem, node);
        Stylesheet_ApplyVariations(ss, classItem, node, screenWidth);
    }
    if (idItem) {
        Stylesheet_ApplyStyleItemToNode(idItem, node);
        Stylesheet_ApplyVariations(ss, idItem, node, screenWidth);
    }
}

void Stylesheet_ApplyPseudoStyleToNode(Stylesheet* ss, NodeP* node, StylePseudoClass pseudo)
{
    if (node == NULL) return;
    Stylesheet_ApplyStyleToNode(ss, node);

    // Get containing screen width
    NU_Window* win = GetNodeWindow(&GUI.winManager, node);
    int screenWidth = win->screenWidth;

    // Create keys
    StyleItemKey tagItemKey = {
        .classID = -1,
        .idID = -1,
        .tag = node->type,
        .pseudoClass = pseudo
    };
    StyleItemKey classItemKey = {
        .classID = -1,
        .idID = -1,
        .tag = -1,
        .pseudoClass = pseudo
    };
    StyleItemKey idItemKey = {
        .classID = -1,
        .idID = -1,
        .tag = -1,
        .pseudoClass = pseudo
    };

    // Get the style items
    StyleItem* tagPseudoItem = NULL;
    StyleItem* classPseudoItem = NULL;
    StyleItem* idPseudoItem = NULL;

    int* itemIndex = Hashmap_Get(&ss->itemIndexMap, &tagItemKey);
    if (itemIndex) {
        tagPseudoItem = Array_Get(&ss->items, *itemIndex);
    }
    if (node->class) {
        int* classID = LinearStringmap_Get(&ss->classIdMap, node->class);
        if (classID) {
            classItemKey.classID = *classID;
            itemIndex = Hashmap_Get(&ss->itemIndexMap, &classItemKey);
            if (itemIndex) {
                classPseudoItem = Array_Get(&ss->items, *itemIndex);
            }
        }
    }
    if (node->id) {
        int* idID = LinearStringmap_Get(&ss->idIdMap, node->id);
        if (idID) {
            idItemKey.idID = *idID;
            itemIndex = Hashmap_Get(&ss->itemIndexMap, &idItemKey);
            if (itemIndex) {
                idPseudoItem = Array_Get(&ss->items, *itemIndex);
            }
        }
    }

    // Apply pseudo style items in order
    if (tagPseudoItem) {
        Stylesheet_ApplyStyleItemToNode(tagPseudoItem, node);
        Stylesheet_ApplyVariations(ss, tagPseudoItem, node, screenWidth);
    }
    if (classPseudoItem) {
        Stylesheet_ApplyStyleItemToNode(classPseudoItem, node);
        Stylesheet_ApplyVariations(ss, classPseudoItem, node, screenWidth);
    }
    if (idPseudoItem) {
        Stylesheet_ApplyStyleItemToNode(idPseudoItem, node);
        Stylesheet_ApplyVariations(ss, idPseudoItem, node, screenWidth);
    }
}

int Stylesheet_ApplyToBranch(Stylesheet* stylesheet, NodeP* root)
{
    // Traverse tree using DFS
    BreadthFirstSearch_Reset(&GUI.bfs, root);
    NodeP* node;
    while (BreadthFirstSearch_Next(&GUI.bfs, &node)) {
        Stylesheet_ApplyStyleToNode(stylesheet, node);
    }
    return 1; // success
}

int NU_Internal_Apply_Stylesheet(Stylesheet* stylesheet)
{
    // Traverse tree using DFS
    BreadthFirstSearch_Reset(&GUI.bfs, GUI.tree.root);
    NodeP* node;
    while (BreadthFirstSearch_Next(&GUI.bfs, &node)) {
        Stylesheet_ApplyStyleToNode(stylesheet, node);
    }
    return 1; // success
}
