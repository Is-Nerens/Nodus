#pragma once

static void NU_Stylesheet_Find_Match(NodeP* node, NU_Stylesheet* ss, int* match_index_list)
{
    int count = 0;

    // Tag match first (lowest priority)
    void* tag_found = HashmapGet(&ss->tag_item_hashmap, &node->type); 
    if (tag_found != NULL) { 
        match_index_list[count++] = (int)*(uint32_t*)tag_found;
    }

    // Class match second
    if (node->node.class != NULL) { 
        char* stored_class = LinearStringsetGet(&ss->class_string_set, node->node.class);
        if (stored_class != NULL) {
            void* class_found = HashmapGet(&ss->class_item_hashmap, &stored_class);
            if (class_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)class_found;
            }
        }
    }

    // ID match last (highest priority)
    if (node->node.id != NULL) { 
        char* stored_id = LinearStringsetGet(&ss->id_string_set, node->node.id);
        if (stored_id != NULL) {
            void* id_found = HashmapGet(&ss->id_item_hashmap, &stored_id);
            if (id_found != NULL) {
                match_index_list[count++] = (int)*(uint32_t*)id_found;
            }
        }
    }
}

#define STYLE_APPLY_LAYOUT_FLAG(mask) node->node.layoutFlags = (node->node.layoutFlags & ~mask) | (item->layoutFlags & mask)
#define STYLE_SHOULD_APPLY_TO_NODE(mask) (item->propertyFlags & mask) && !(node->node.inlineStyleFlags & mask)

static void NU_Apply_Style_Item_To_Node(NodeP* node, NU_Stylesheet_Item* item)
{
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_LAYOUT_VERTICAL)) STYLE_APPLY_LAYOUT_FLAG(LAYOUT_VERTICAL); 
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_GROW)) {
        STYLE_APPLY_LAYOUT_FLAG(GROW_HORIZONTAL); // Grow horizontal WHY DOES THIS OVERWRITE THE NODE's VERTICAL LAYOUT DIRECTION?
        STYLE_APPLY_LAYOUT_FLAG(GROW_VERTICAL); // Grow vertical
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_VERTICAL_SCROLL)) STYLE_APPLY_LAYOUT_FLAG(OVERFLOW_VERTICAL_SCROLL); // Overflow vertical scroll (or not)
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_HORIZONTAL_SCROLL)) STYLE_APPLY_LAYOUT_FLAG(OVERFLOW_HORIZONTAL_SCROLL); // Overflow horizontal scroll (or not)
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_POSITION_ABSOLUTE)) STYLE_APPLY_LAYOUT_FLAG(POSITION_ABSOLUTE); // Absolute positioning (or not)
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_HIDDEN)) STYLE_APPLY_LAYOUT_FLAG(HIDDEN); // Hidden or not
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IGNORE_MOUSE)) STYLE_APPLY_LAYOUT_FLAG(IGNORE_MOUSE); // Ignore mouse or not
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_GAP)) node->node.gap = item->gap;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PREFERRED_WIDTH)) node->node.preferred_width = item->preferred_width;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MIN_WIDTH)) node->node.minWidth = item->minWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MAX_WIDTH)) node->node.maxWidth = item->maxWidth;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_PREFERRED_HEIGHT)) node->node.preferred_height = item->preferred_height;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MIN_HEIGHT)) node->node.minHeight = item->minHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_MAX_HEIGHT)) node->node.maxHeight = item->maxHeight;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_ALIGN_H)) node->node.horizontalAlignment = item->horizontalAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_ALIGN_V)) node->node.verticalAlignment = item->verticalAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_ALIGN_H)) node->node.horizontalTextAlignment = item->horizontalTextAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TEXT_ALIGN_V)) node->node.verticalTextAlignment = item->verticalTextAlignment;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_LEFT)) node->node.left = item->left;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_RIGHT)) node->node.right = item->right;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_TOP)) node->node.top = item->top;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BOTTOM)) node->node.bottom = item->bottom;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_BACKGROUND) && !(node->node.inlineStyleFlags & PROPERTY_FLAG_HIDE_BACKGROUND)) {
        node->node.backgroundR = item->backgroundR;
        node->node.backgroundG = item->backgroundG;
        node->node.backgroundB = item->backgroundB;
    }
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_HIDE_BACKGROUND) && !(node->node.inlineStyleFlags & PROPERTY_FLAG_BACKGROUND)) STYLE_APPLY_LAYOUT_FLAG(HIDE_BACKGROUND); // Hide background (or not)
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
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_IMAGE)) node->typeData.image.glImageHandle = item->glImageHandle;
    if (STYLE_SHOULD_APPLY_TO_NODE(PROPERTY_FLAG_INPUT_TYPE)) node->typeData.input.inputText.type = item->inputType;
    node->node.fontId = item->fontId; // set font 
}

void NU_Apply_Stylesheet_To_Node(NodeP* node, NU_Stylesheet* ss)
{
    int match_index_list[3] = {-1, -1, -1};
    NU_Stylesheet_Find_Match(node, ss, &match_index_list[0]);

    int i = 0;
    while (match_index_list[i] != -1) {
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, (uint32_t)match_index_list[i]);
        i += 1;

        // --- Apply style ---
        NU_Apply_Style_Item_To_Node(node, item);
    }
}

void NU_Apply_Tag_Style_To_Node(NodeP* node, NU_Stylesheet* ss)
{
    // Tag match first (lowest priority)
    void* tag_found = HashmapGet(&ss->tag_item_hashmap, &node->type); 
    if (tag_found != NULL) { 
        uint32_t index = *(uint32_t*)tag_found;
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
        NU_Apply_Style_Item_To_Node(node, item);
    }
}

void NU_Apply_Pseudo_Style_To_Node(NodeP* node, NU_Stylesheet* ss, enum NU_Pseudo_Class pseudo)
{
    // Tag pseudo style match and apply
    NU_Stylesheet_Tag_Pseudo_Pair key = { node->type, pseudo };
    void* tag_pseudo_found = HashmapGet(&ss->tag_pseudo_item_hashmap, &key);
    if (tag_pseudo_found != NULL) {
        uint32_t index = *(uint32_t*)tag_pseudo_found;
        NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
        NU_Apply_Style_Item_To_Node(node, item);
    }

    // Class pseudo style match and apply
    if (node->node.class != NULL) {
        char* stored_class = LinearStringsetGet(&ss->class_string_set, node->node.class);
        if (stored_class != NULL) {
            NU_Stylesheet_String_Pseudo_Pair key = { stored_class, pseudo };
            void* class_pseudo_found = HashmapGet(&ss->class_pseudo_item_hashmap, &key);
            if (class_pseudo_found != NULL) {
                uint32_t index = *(uint32_t*)class_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }

    // Id pseudo style match and apply
    if (node->node.id != NULL) {
        char* stored_id = LinearStringsetGet(&ss->id_string_set, node->node.id);
        if (stored_id != NULL) {
            NU_Stylesheet_String_Pseudo_Pair key = { stored_id, pseudo };
            void* id_pseudo_found = HashmapGet(&ss->id_pseudo_item_hashmap, &key);
            if (id_pseudo_found != NULL) {
                uint32_t index = *(uint32_t*)id_pseudo_found;
                NU_Stylesheet_Item* item = (NU_Stylesheet_Item*)Vector_Get(&ss->items, index);
                NU_Apply_Style_Item_To_Node(node, item);
            }
        }
    }
}
