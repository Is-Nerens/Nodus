#pragma once

void NU_DissociateNode(NodeP* node)
{
    NU_Unregister_All_Non_Iterated_Events(node);

    switch(node->type) {
        case NU_WINDOW:
            //SDL_DestroyWindow(node->node.window);
            break;
        case NU_CANVAS:
            NU_DeleteCanvasContext(node->typeData.canvas.ctxHandle);
            break;
        case NU_INPUT:
            InputText* inputText = Container_Get(&GUI.textInputs, node->typeData.input.textInputHandle);
            InputText_Free(inputText);
            Container_Remove(&GUI.textInputs,node->typeData.input.textInputHandle);
            break;
        default:
            break;
    }
    if (node->id != NULL) {
        Stringmap_Delete(&GUI.id_node_map, node->id);
    }
    if (node == GUI.hovered_node) {
        GUI.hovered_node = NULL;
    } 
    if (node == GUI.mouse_down_node) {
        GUI.mouse_down_node = NULL;
    }
    if (node == GUI.scroll_hovered_node) {
        GUI.scroll_hovered_node = NULL;
    }
    if (node == GUI.scroll_mouse_down_node) {
        GUI.scroll_mouse_down_node = NULL;
    }
    if (node == GUI.focused_node) {
        GUI.focused_node = NULL;
    }
}