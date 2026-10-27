#pragma once

// ---------------------------------------
// --- Macros to reduce code verbosity ---
// ---------------------------------------
#include "nu_stylesheet_structs.h"
#include "nu_stylesheet_tokens.h"
#define STYLE_APPLY_LAYOUT_FLAG(prop, layout_mask) if ((item->propertyFlags & (prop)) && !(node->overrideStyleFlags & (prop))) node->layoutFlags = (node->layoutFlags & ~(layout_mask)) | (item->layoutFlags & (layout_mask))
#define STYLE_SHOULD_APPLY_TO_NODE(mask) (item->propertyFlags & mask) && !(node->overrideStyleFlags & mask)

// This should be optimised (branchless)
static void Stylesheet_ApplyStyleItemToNode(StyleItem* item, Node* node)
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
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_GAP)) node->public.generic.gap = item->gap;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PREFERRED_WIDTH)) node->public.generic.preferredWidth = item->preferredWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MIN_WIDTH)) node->public.generic.minWidth = item->minWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MAX_WIDTH)) node->public.generic.maxWidth = item->maxWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PREFERRED_HEIGHT)) node->public.generic.preferredHeight = item->preferredHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MIN_HEIGHT)) node->public.generic.minHeight = item->minHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MAX_HEIGHT)) node->public.generic.maxHeight = item->maxHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_ALIGN_H)) node->horizontalAlignment = item->horizontalAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_ALIGN_V)) node->verticalAlignment = item->verticalAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_ALIGN_H)) node->horizontalTextAlignment = item->horizontalTextAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_ALIGN_V)) node->verticalTextAlignment = item->verticalTextAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_LEFT)) node->public.generic.left = item->left;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_RIGHT)) node->public.generic.right = item->right;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TOP)) node->public.generic.top = item->top;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BOTTOM)) node->public.generic.bottom = item->bottom;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BACKGROUND)) {
        node->public.generic.backgroundR = item->backgroundR;
        node->public.generic.backgroundG = item->backgroundG;
        node->public.generic.backgroundB = item->backgroundB;
        node->layoutFlags &= ~(HIDE_BACKGROUND * (!(node->overrideStyleFlags & PROPERTY_FLAG_HIDE_BACKGROUND) && !(item->propertyFlags & PROPERTY_FLAG_HIDE_BACKGROUND)));
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_COLOUR)) {
        node->public.generic.borderR = item->borderR;
        node->public.generic.borderG = item->borderG;
        node->public.generic.borderB = item->borderB;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_COLOUR)) {
        node->public.generic.textR = item->textR;
        node->public.generic.textG = item->textG;
        node->public.generic.textB = item->textB;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_TOP)) node->public.generic.borderTop = item->borderTop;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_BOTTOM)) node->public.generic.borderBottom = item->borderBottom;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_LEFT)) node->public.generic.borderLeft = item->borderLeft;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RIGHT)) node->public.generic.borderRight = item->borderRight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_TL)) node->public.generic.borderRadiusTl = item->borderRadiusTl;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_TR)) node->public.generic.borderRadiusTr = item->borderRadiusTr;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_BL)) node->public.generic.borderRadiusBl = item->borderRadiusBl;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BORDER_RADIUS_BR)) node->public.generic.borderRadiusBr = item->borderRadiusBr;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_TOP)) node->public.generic.padTop = item->padTop;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_BOTTOM)) node->public.generic.padBottom = item->padBottom;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_LEFT)) node->public.generic.padLeft = item->padLeft;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PAD_RIGHT)) node->public.generic.padRight = item->padRight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IMAGE) && node->type != NU_TYPE_CANVAS && node->type != NU_TYPE_INPUT) {
        node->special.image.imageHandle = item->imageHandle;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IMAGE_WIDTH_DIM) && node->type == NU_TYPE_IMAGE) {
        node->special.image.widthDimType = item->imageWidthDimType;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IMAGE_HEIGHT_DIM) && node->type == NU_TYPE_IMAGE) {
        node->special.image.heightDimType = item->imageHeightDimType;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IMAGE_FILTER) && node->type == NU_TYPE_IMAGE) {
        node->special.image.filter = item->imageFilter;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IMAGE_REPEAT) && node->type == NU_TYPE_IMAGE) {
        node->special.image.repeat = item->imageRepeat;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_INPUT_TYPE) && node->type == NU_TYPE_INPUT) {
        InputText* inputText = Container_Get(&GUI.textInputs, node->special.input.textInputHandle);
        inputText->type = item->inputType;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_FONT)) node->fontIdx = item->fontIdx;
}

void Stylesheet_ApplyVariations(Stylesheet* ss, StyleItem* item, Node* node, int screenWidth)
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

void Stylesheet_ApplyStyleToNode(Stylesheet* ss, Node* node)
{
    // Get containing screen width
    Window* win = DisplayManager_GetNodeWindow(&GUI.displayManager, node);
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

void Stylesheet_ApplyPseudoStyleToNode(Stylesheet* ss, Node* node, StylePseudoClass pseudo)
{
    if (node == NULL) return;
    Stylesheet_ApplyStyleToNode(ss, node);

    // Get containing screen width
    Window* win = DisplayManager_GetNodeWindow(&GUI.displayManager, node);
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

int Stylesheet_ApplyToBranch(Stylesheet* stylesheet, Node* root)
{
    // Traverse tree using DFS
    BreadthFirstSearch_Reset(&GUI.bfs, root);
    Node* node;
    while (BreadthFirstSearch_Next(&GUI.bfs, &node)) {
        Stylesheet_ApplyStyleToNode(stylesheet, node);
    }
    return 1; // success
}

int NU_Internal_Apply_Stylesheet(Stylesheet* stylesheet)
{
    // Traverse tree using DFS
    BreadthFirstSearch_Reset(&GUI.bfs, GUI.tree.root);
    Node* node;
    while (BreadthFirstSearch_Next(&GUI.bfs, &node)) {
        Stylesheet_ApplyStyleToNode(stylesheet, node);
    }
    return 1; // success
}



// -------------------------------------
// --- Parellelised Stylesheet Apply ---
// -------------------------------------
struct StylesheetPartitionApplyJob {
    Stylesheet* ss;
    Node* node;
};

static void ApplyStylesDFS(Stylesheet* stylesheet, Node* node)
{
    Stylesheet_ApplyStyleToNode(stylesheet, node);
    Node* child = node->firstChild;
    while (child) {
        Node* next = child->nextSibling;
        if (child->isPartitionRoot) { child = next; continue; } // Subtree belongs to another partition
        ApplyStylesDFS(stylesheet, child);
        child = next;
    }
}

static int Stylesheet_ApplyToPartition(void* args) {
    struct StylesheetPartitionApplyJob* data = args;
    ApplyStylesDFS(data->ss, data->node);
    return 1;
}

int NU_Internal_Apply_Stylesheet_Parallel(Stylesheet* stylesheet, ThreadPool* threadpool)
{
    TreePartitioner part;
    TreePartitioner_Partition(&part, GUI.tree.root, TREE_PARTITION_MAX);
    struct StylesheetPartitionApplyJob jobs[TREE_PARTITION_MAX];
    int jobCount = 0;
    Node* node;
    while ((node = TreePartitioner_GetNextPartition(&part, TREE_PARTITION_TRAVERSE_DIRECTION_NONE)) != NULL) {
        jobs[jobCount].ss = stylesheet;
        jobs[jobCount].node = node;
        ThreadPool_Submit(threadpool, Stylesheet_ApplyToPartition, &jobs[jobCount]);
        jobCount++;
    }
    ThreadPool_Wait(threadpool);
    return 1;
}
