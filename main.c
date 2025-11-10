#include <nodus.h>




int main()
{
    // ---------------------------------------
    // --- Create GUI and apply stylesheet ---
    // ---------------------------------------
    if (!NU_Init()) return -1;
    if (!NU_Load_XML("app.xml")) return -1;
    if (!NU_Load_Stylesheet("app.css")) return -1;




    uint32_t rect = NU_Get_Node_By_Id("rect");
    char* string = "the string char";
    char* string_child = "the child string char";
    for (int i=0; i<1; i++){
        uint32_t added = NU_Create_Node(rect, REC);
        uint32_t added_child = NU_Create_Node(added, REC);
        uint32_t added_child2 = NU_Create_Node(added, REC);
        printf("child count: %d\n", NU_NODE(added)->child_count);
        printf("child capacity: %d\n", NU_NODE(added)->child_capacity);
        printf("first_child_index: %d\n", NU_NODE(added)->first_child_index);
        printf("node present: %d\n", NU_NODE(added)->node_present);
        NU_Set_Class(added, "temp");
        NU_Set_Class(added_child, "temp-child");
        NU_Set_Class(added_child2, "temp-child");
    }



    // ------------------------
    // --- Application loop ---
    // ------------------------
    while(NU_Running())
    {

    }

    // -------------------
    // --- Free Memory ---
    // -------------------
    NU_Quit();
}