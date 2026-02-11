# Nodus - Retained Mode UI Library

Nodus is a lightweight, high-performance retained GUI library for C.
It uses XML templates for layout, CSS for styling, and a DOM-style tree model that can be manipulated with code.
Special thanks to **Nick Barker** for the work he did on the [Clay](https://github.com/nicbarker/clay) library.<br>

<br>

### Dependencies
Nodus requires: [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h), [SDL3](https://github.com/libsdl-org/SDL), [FreeType](https://github.com/freetype), [GLEW](https://github.com/nigels-com/glew) (stb_image is included with release)

<br>

### Features:
- XML templates and CSS styling
- Flexbox layout model for responsive layouts
- Text rendering using FreeType with support for subpixel rendering, producing sharp text on lower resolution displays
- Canvas API that supports drawing
- UI events to detect resizing, mouse hovering, clicks and more
- Multi-window support is as easy as creating a <window> tag (All child elements of a window will be drawn in that window)

<br>

### Features Still In Development
- Text inputs
- Auto scroll behaviour
- Scrollbar customisation
- DPI Scalling
- Expanded canvas API functionality (text, and more tools)
- XML components: reusable UI components constructed from xml template files
- Linux and MacOS support
- Debugging tools
- Library Documentation
- Dependency Reduction (if possible)
- Pseudo styles do not apply to id selectors

<br>

### Usage Example
#### XML Template
```xml
<window dir="v">

    <!-- toolbar -->
    <box id="toolbar"> 
        <button>file</button>
        <button>edit</button>
        <button>export</button>
    </box>

    <!-- content -->
    <box grow="b" dir="h">

        <!-- sidebar -->
        <box id="sidebar">
            <box id="sidebar-label">Sidebar</box>
        </box>

        <!-- right content -->
        <box id="content">
            <canvas id="interactive-chart" grow="b"/>
            <button id="btn" grow="h">PRESS ME!</button>
        </box>

    <box>
</window>
```


#### CSS Stylesheet
```css
@font font-normal {
    src: fonts/font.ttf; /* Provide a font file! */
    size: 18;
    weight: 400;
}
button {
    background: #1b445f;
    text-colour: #ffffff;
    border-radius: 4;
    padding: 3;
    width: 100;
    text-align-h: center;
}
button:hover {
    background: #234e6b;
}
button:press {
    background: #133144;
}
#toolbar {
    background: #235c81;
    grow: h;
    gap: 5
    padding: 4;
}
#sidebar {
    align-h: center;
    dir: v;
    background: #123044;
    width: 280; 
    grow: v; 
    padding: 4;
}
#sidebar-label {
    background: none;
    text-align-h: center;
    padding: 5;
    grow: h;
}
#content {
    background: #000000;
    dir: v;
    grow: b;
    padding: 5;
    gap: 5;
}
canvas {
    border-radius: 5;
    background: #141414;
}
```

#### Main.c
```c
#include <nodus.h>
#include <stdio.h>
#include <stdint.h>

void Press(NU_Event event, void* args)
{   
    char* message = (char*)args;
    printf("%s\n", message);
}

int main()
{
    // initialise
    if (!NU_Create_Gui("app.xml", "app.css")) return -1;
    
    // get node handles
    Node* button = NU_Get_Node_By_Id("btn");
    Node* chart = NU_Get_Node_By_Id("interactive-chart");

    // register click event
    char* message = "pressed me!";
    NU_Register_Event(button, message, Press, NU_EVENT_ON_CLICK); 

    // draw a border rect on the chart
    NU_RGB border_col; border_col.r = 0.8588f; border_col.g = 0.1490f; border_col.b = 0.1490f;
    NU_RGB fill_col; border_col.r = 0.8588f; border_col.g = 0.9490f; border_col.b = 0.9490f;
    NU_Border_Rect(chart, 100, 100, 200, 200, 3, &border_col, &fill_col);

    // app loop
    while(NU_Running())
    {

    }

    // free
    NU_Quit();
}
```


<br>

## Documentation (In Progress)

#### UI Nodes:
- window
- box
- button
- input
- canvas
- image
- table, thead, row

## Style & Layout Properties

### Core
| Property | Description | Values |
|--------|-------------|-----|
| `id` | Unique element identifier | String |
| `class` | Style class name | String |
| `dir` | Layout direction (Horizontal by default) | String `h` `v` |
| `grow` | Flex grow behaviour | String `h` `v` `b` |
| `gap` | Spacing between children | Int |
| `overflow-h` | Horizontal overflow | String `true` `false` |
| `overflow-v` | Vertical overflow | String `true` `false` |
| `position` | `relative` positioned nodes are placed according to the normal layout flow. `absolute` positioned nodes are placed with respect to their parent | String `relative` `absolute` |
| `left` `right` `top` `bottom` | Absolute positioning offsets | Int |
| `hide` | Visibility toggle | String `true` `false` |

### Size
| Property | Description |
|--------|-------------|
| `width` `height` | Fixed size |
| `min-width` `min-height` | Minimum size |
| `max-width` `max-height` | Maximum size |

### Alignment
| Property | Description | Values |
|--------|-------------|----|
| `align-h` | Horizontal child alignment | String `left` `right` `center` |
| `align-v` | Vertical child alignment | String `top` `bottom` `center` |
| `text-align-h` | Horizontal text alignment | String `left` `right` `center` |
| `text-align-v` | Vertical text alignment | String `top` `bottom` `center` |

### Colours
| Property | Values |
|--------|-------|
| `background` | Hex code `#xxxxxx` |
| `border-colour` | Hex code `#xxxxxx` |
| `text-colour` | Hex code `#xxxxxx` |

### Border
| Property | Description | Values |
|--------|-------|-----|
| `border` | Border width (all sides) | Int |
| `border-top` `bottom` `left` `right` | Border width (individual sides) | Int |
| `border-radius` | Border radius (all corners) | Int |
| `border-radius-top-left` | Border radius (top-left corner) | Int |
| `border-radius-top-right` | Border radius (top-right corner) | Int |
| `border-radius-bottom-left` | Border radius (bottom-left corner) | Int |
| `border-radius-bottom-right` | Border radius (bottom-right corner) | Int |

### Padding
| Property | Description | Values |
|--------|-------|-----|
| `padding` | All sides | Int |
| `padding-top` `bottom` `left` `right` | Individual sides | Int |

### Input
| Property | Description | Values |
|-----|-----|-----|
|`input-type`|Enforces typing format| Sring `number` `text` [Default `text`]|


<br>

---

## General functions (in C)
| Function | Description |
| ---- | ---- | 
| int `NU_Create_Gui`(char* xml_filepath, char* css_filepath) | Creates the global GUI object. Returns 0 if there is an error, or incorrect xml/css format|
| void `NU_Quit`() | Frees memory used by Nodus |
| int `NU_Running`() | Returns true while the GUI is running |
| void `NU_Unblock`() | Unblocks the main thread. Allows while(NU_Running) { } iteration to execute. This function is thread safe. |
| void `NU_Render`() | Triggers a full GUI re-render. Typically used to re-render canvas content. This function is thread safe. |

<br>

---

## DOM functions (in C)

| Function | Description |
| ----- | ---- | 
| Node* `NU_Get_Node_By_Id`(char* id) | Returns a Node* with the associated ID. If the node does not exist the function will return NULL | 
| NU_Nodelist `NU_Get_Nodes_By_Class`(char* class) | Returns a NU_Nodelist containing all `Node*` that have the given class |
| NU_Nodelist `NU_Get_Nodes_By_Tag`(NodeType type) | Returns a NU_Nodelist containing all `Node*` that have the given tag |
| void `NU_Set_Class`(Node* node, char* class_name) | Updates/Sets the class of a Node* |
| Node* `NU_PARENT`(Node* node) | Returns the parent (Node*) of the given Node* |
| Node* `NU_CHILD`(Node* node, uint32_t childIndex) | Returns the child (Node*) of a given Node* found at the specified childIndex |
| uint32_t `NU_CHILD_COUNT`(Node* node) | Returns the number of children (uint32_t) that a node (Node*) has |
| uint32_t `NU_DEPTH`(Node* node) | Returns the nested depth (uint32_t) of a node (Node*). The depth is zero indexed: [0... n]|
| Node* `NU_CREATE_NODE`(Node* parent, NodeType type) | Returns a Node* of a new created node with the given NodeType. The created Node* is appended to the `end` of it's parent |
| void `NU_DELETE_NODE`(Node* node) | Deletes a Node* and all of its nested children from the DOM. `NOTE!` Do not use a Node* after deletion! This is akin to a `use-after-free` |
| const char* `NU_INPUT_TEXT_CONTENT`(Node* node) | Returns a char* containing the text content of an `Input` node. `NOTE!` if a non *input* node is passed as an argument -> the function will return NULL|
| void `NU_SHOW`(Node* node) | Sets the visibility of a node to `true`|
| void `NU_HIDE`(Node* node) | Sets the visibility of a node to `false`|


<br>

---

## Canvas API functions (in C)
Thhe coordinate system of a canvas starts at (0,0) top-left. The Y coordinate specifies the distance from the top.
| Function | Description |
| ---------------- | ---- | 
| void `NU_Clear_Canvas`(Node* canvas) | Clears all drawn content of a canvas (Node*) |
| void `NU_Border_Rect`(Node* canvas, float x, y, w, h, thickness, NU_RGB* border_col, fill_col) | Draws a border rect on a canvas (Node*)|
| void `NU_Line`(Node* canvas, float x1, y1, x2, y2, thickness, NU_RGB* col) | Draws a line on a canvas (Node*)|
| void `NU_Dashed_Line`(Node* canvas, float x1, y1, x2, y2, uint8_t* dash_pattern <pointer to an array of [seg0, space0, seg1, space1, seg2...]>, uint32_t dash_pattern_len, NU_RGB* col) | Draws a dashed line on a canvas (Node*)|

