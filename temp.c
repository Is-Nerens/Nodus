static void NU_PositionChildrenHorizontally(Node* parent, NU_Layer* childlayer)
{
    // layout dir -> top to bottom
    if (parent->layoutFlags & LAYOUT_VERTICAL)
    {   
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            Node* child = NU_Layer_Get(childlayer, i); 
            if (child->nodeState == 2 || child->tag == WINDOW) continue;

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaning_width = (parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight) - child->width;
                float x_align_offset = remaning_width * 0.5f * (float)parent->horizontalAlignment;
                child->x = parent->x + parent->padLeft + parent->borderLeft + x_align_offset;
            }
            else { // position absolute
                child->x = parent->x + parent->padLeft + parent->borderLeft;
                if (child->left > 0.0f) {
                    child->x = parent->x + child->left + parent->padLeft + parent->borderLeft;
                }
                else if (child->right > 0.0f) {
                    float inner_width = parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight;
                    child->x = parent->x + inner_width - child->width - child->right;
                }
            }
        }
    }

    // layout dir -> left to right
    else
    {
        // calculate remaining width (optimise this by caching this value inside parent's content width variable)
        float remainingWidth = (parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight);
        remainingWidth -= (!!(parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL)) * 8.0f;
        int numChildrenAffectingWidth = 0;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
            Node* child = NU_Layer_Get(childlayer, i);
            if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
            remaining_width -= child->width; numChildrenAffectingWidth++;
        }
        remainingWidth -= parent->gap * (numChildrenAffectingWidth - 1);



        // position children horizontally
        float cursorX = 0.0f;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            Node* child = NU_Layer_Get(childlayer, i); 
            if (child->nodeState == 2 || child->tag == WINDOW) continue;
            
            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float x_align_offset = remaining_width * 0.5f * (float)parent->horizontalAlignment;
                child->x = parent->x + parent->padLeft + parent->borderLeft + cursorX + x_align_offset;
                cursorX += child->width + parent->gap;
            }
            else { // position absolute 
                child->x = parent->x + parent->padLeft + parent->borderLeft;
                if (child->left > 0.0f) {
                    child->x = parent->x + child->left + parent->padLeft + parent->borderLeft;
                }
                else if (child->right > 0.0f) {
                    float inner_width = parent->width - parent->padLeft - parent->padRight - parent->borderLeft - parent->borderRight;
                    child->x = parent->x + inner_width - child->width - child->right;
                }
            }
        }
    }
}

static void NU_PositionChildrenVertically(Node* parent, NU_Layer* childlayer)
{
    float y_scroll_offset = 0.0f;
    if (parent->layoutFlags & OVERFLOW_VERTICAL_SCROLL && 
        parent->childCount > 0 && 
        parent->contentHeight > parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom) 
    {
        float track_h = parent->height - parent->borderTop - parent->borderBottom;
        float inner_height_w_pad = track_h - parent->padTop - parent->padBottom;
        float inner_proportion_of_content_height = inner_height_w_pad / parent->contentHeight;
        float thumb_h = inner_proportion_of_content_height * track_h;
        float content_scroll_range = parent->contentHeight - inner_height_w_pad;
        float thumb_scroll_range = track_h - thumb_h;
        float scroll_factor = content_scroll_range / max(thumb_scroll_range, 1.0f);
        y_scroll_offset += (-parent->scrollV * (track_h - thumb_h)) * scroll_factor;

        // undo effect of scroll offset for table header row
        Node* first_child = NU_Layer_Get(childlayer, parent->firstChildIndex);
        if (first_child->nodeState != 2 && first_child->tag == THEAD) {
            first_child->y -= y_scroll_offset;
        }
    }

    // layout dir -> left to right
    if (!(parent->layoutFlags & LAYOUT_VERTICAL))
    {   
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            Node* child = NU_Layer_Get(childlayer, i); 
            if (child->nodeState == 2 || child->tag == WINDOW) continue;

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float remaining_height = (parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom) - child->height;
                float y_align_offset = remaining_height * 0.5f * (float)parent->verticalAlignment;
                child->y += parent->y + parent->padTop + parent->borderTop + y_align_offset + y_scroll_offset;
            }
            else { // position absolute 
                child->y = parent->y + parent->padTop + parent->borderTop;
                if (child->top > 0.0f) {
                    child->y = parent->y + child->top + parent->padTop + parent->borderTop;
                }
                else if (child->bottom > 0.0f) {
                    float inner_height = parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom;
                    child->y = parent->y + inner_height - child->height - child->bottom;
                }
            }
        }
    }

    // layout dir -> top to bottom
    else
    {
        // calculate remaining height (optimise this by caching this value inside parent's content height variable)
        float remainingHeight = (parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom);
        remainingHeight -= (!!(parent->layoutFlags & OVERFLOW_HORIZONTAL_SCROLL)) * 8.0f;
        int numChildrenAffectingHeight = 0;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++) {
            Node* child = NU_Layer_Get(childlayer, i);
            if (child->nodeState == 2 || child->tag == WINDOW || child->layoutFlags & POSITION_ABSOLUTE) continue;
            remainingHeight -= child->height; numChildrenAffectingHeight++;
        }
        remainingHeight -= parent->gap * (numChildrenAffectingHeight - 1);



        // position children vertically
        float cursorY = 0.0f;
        for (uint16_t i=parent->firstChildIndex; i<parent->firstChildIndex + parent->childCount; i++)
        {
            Node* child = NU_Layer_Get(childlayer, i);
            if (child->nodeState == 2 || child->tag == WINDOW) continue;

            if (!(child->layoutFlags & POSITION_ABSOLUTE)) { // position relative
                float y_align_offset = remaining_height * 0.5f * (float)parent->verticalAlignment;
                child->y += parent->y + parent->padTop + parent->borderTop + cursorY + y_align_offset + y_scroll_offset;
                cursorY += child->height + parent->gap;
            }
            else { // position abosolute
                child->y = parent->y + parent->padTop + parent->borderTop;
                if (child->top > 0.0f) {
                    child->y = parent->y + child->top + parent->padTop + parent->borderTop;
                }
                else if (child->bottom > 0.0f) {
                    float inner_height = parent->height - parent->padTop - parent->padBottom - parent->borderTop - parent->borderBottom;
                    child->y = parent->y + inner_height - child->height - child->bottom;
                }
            }
        }
    }
}

static void NU_Calculate_Positions()
{
    for (uint16_t l=0; l<=__NGUI.deepest_layer; l++) 
    {
        NU_Layer* parentlayer = &__NGUI.tree.layers[l];
        NU_Layer* childlayer = &__NGUI.tree.layers[l+1];
        
        // Iterate over parent layer
        for (uint32_t p=0; p<parentlayer->size; p++) // For node in layer
        {   
            Node* parent = NU_Layer_Get(parentlayer, p);
            if (parent->nodeState == 0 || parent->nodeState == 2) continue;

            if (parent->tag == WINDOW)
            {
                parent->x = 0;
                parent->y = 0;
            }

            NU_PositionChildrenHorizontally(parent, childlayer);
            NU_PositionChildrenVertically(parent, childlayer);
        }
    }
}