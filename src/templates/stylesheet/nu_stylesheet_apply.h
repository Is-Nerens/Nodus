#pragma once

// ---------------------------------------
// --- Macros to reduce code verbosity ---
// ---------------------------------------
#define STYLE_APPLY_LAYOUT_FLAG(prop, layout_mask) if ((item->propertyFlags & (prop)) && !(node->overrideStyleFlags & (prop))) node->layoutFlags = (node->layoutFlags & ~(layout_mask)) | (item->layoutFlags & (layout_mask))
#define STYLE_SHOULD_APPLY_TO_NODE(mask) (item->propertyFlags & mask) && !(node->overrideStyleFlags & mask)

static void NU_Apply_Style_Item_To_Node(NodeP* node, NU_Stylesheet_Item* item)
{
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_LAYOUT_VERTICAL, LAYOUT_VERTICAL); 
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_GROW, GROW_HORIZONTAL);
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_GROW, GROW_VERTICAL);
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_VERTICAL_SCROLL, OVERFLOW_VERTICAL_SCROLL);     // Overflow vertical scroll (or not)
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_HORIZONTAL_SCROLL, OVERFLOW_HORIZONTAL_SCROLL); // Overflow horizontal scroll (or not)
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_POSITION_ABSOLUTE, POSITION_ABSOLUTE);          // Absolute positioning (or not)
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_HIDDEN, HIDDEN);                                 // Hidden or not
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_IGNORE_MOUSE, IGNORE_MOUSE);                     // Ignore mouse or not
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
    }
    STYLE_APPLY_LAYOUT_FLAG(PROPERTY_FLAG_HIDE_BACKGROUND, HIDE_BACKGROUND); // Hide background (or not)
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
        node->typeData.image.glImageHandle = item->glImageHandle;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_INPUT_TYPE)) node->typeData.input.inputText.type = item->inputType;
    node->fontId = item->fontId; // set font 
}

void NU_Apply_Stylesheet_To_Node(NodeP* node, NU_Stylesheet* ss)
{   
    // 1. Apply default style
    NU_Apply_Style_Item_To_Node(node, &ss->defaultStyleItem);

    // 2. Apply tag match
    void* tag_found = HashmapGet(&ss->tag_item_hashmap, &node->type); 
    if (tag_found != NULL) { 
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, *(u32*)tag_found);
        NU_Apply_Style_Item_To_Node(node, item);
    }

    // 3. Apply class match
    if (node->class != NULL) { 
        char* stored_class = LinearStringsetGet(&ss->class_string_set, node->class);
        if (stored_class != NULL) {
            void* class_found = HashmapGet(&ss->class_item_hashmap, &stored_class);
            if (class_found != NULL) {
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, *(u32*)class_found);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }

    // 4. Apply ID match
    if (node->id != NULL) { 
        char* stored_id = LinearStringsetGet(&ss->id_string_set, node->id);
        if (stored_id != NULL) {
            void* id_found = HashmapGet(&ss->id_item_hashmap, &stored_id);
            if (id_found != NULL) {
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, *(u32*)id_found);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }
}

void NU_Apply_Pseudo_Style_To_Node(NodeP* node, NU_Stylesheet* ss, enum NU_Pseudo_Class pseudo)
{
    // Tag pseudo style match and apply
    NU_Stylesheet_Tag_Pseudo_Pair key = { node->type, pseudo };
    void* tag_pseudo_found = HashmapGet(&ss->tag_pseudo_item_hashmap, &key);
    if (tag_pseudo_found != NULL) {
        u32 index = *(u32*)tag_pseudo_found;
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
        NU_Apply_Style_Item_To_Node(node, item);
    }

    // Class pseudo style match and apply
    if (node->class != NULL) {
        char* stored_class = LinearStringsetGet(&ss->class_string_set, node->class);
        if (stored_class != NULL) {
            NU_Stylesheet_String_Pseudo_Pair key = { stored_class, pseudo };
            void* class_pseudo_found = HashmapGet(&ss->class_pseudo_item_hashmap, &key);
            if (class_pseudo_found != NULL) {
                u32 index = *(u32*)class_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }

    // Id pseudo style match and apply
    if (node->id != NULL) {
        char* stored_id = LinearStringsetGet(&ss->id_string_set, node->id);
        if (stored_id != NULL) {
            NU_Stylesheet_String_Pseudo_Pair key = { stored_id, pseudo };
            void* id_pseudo_found = HashmapGet(&ss->id_pseudo_item_hashmap, &key);
            if (id_pseudo_found != NULL) {
                u32 index = *(u32*)id_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }
}
