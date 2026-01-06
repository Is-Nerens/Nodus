# Nodus - Retained Mode UI Library

Nodus is a lightweight, high-performance retained GUI library for C.
It uses XML templates for layout, CSS for styling, and a DOM-style tree model that can be manipulated with code.

<br>

Special thanks to **Nick Barker** for the work he did on the [Clay](https://github.com/nicbarker/clay) library<br>
<br>


### Dependencies
I dislike dependencies as much as the next person but, <br>
Nodus requires: [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h), [SDL3](https://github.com/libsdl-org/SDL), [FreeType](https://github.com/freetype), [GLEW](https://github.com/nigels-com/glew) (stb_image is included with release)

<br>

### Features:
- XML templates and CSS styling
- Flexbox layout model for responsive layouts
- Text rendering using FreeType with support for subpixel rendering, producing sharp text on lower resolution displays
- Canvas API that supports drawing
- JavasScript like UI events to detect things liek resizing, mouse hovering, clicks etc
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

<br>

### Usage Example
Setting up Nodus, and creating a few UI elements <br>

#### XML Template
```xml
<window width="800" height="600">
    <rect class="container" grow="b">
        <canvas id="interactive_chart" grow="b"/>
        <button id="btn" grow="h">PRESS ME!</button>
    </rect>
</window>
```


#### CSS Stylesheet
```css
@font font-normal {
    src: ./fonts/Inter/Inter_Variable_Weight.ttf;
    size: 14;
    weight: 400;
}
.container {
    padding: 16;
}
button {
    background: #3a7afe;
    text-colour: #ffffff;
    border-radius: 4;
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
    printf("%s/n", message);
}

int main()
{
    // initialise
    if (!NU_Init()) return -1;
    if (!NU_Load_XML("app.xml")) return -1;
    if (!NU_Load_Stylesheet("app.css")) return -1;

    // get node handles
    uint32_t container = NU_Get_Node_By_Id("container");
    uint32_t button = NU_Get_Node_By_Id("btn");

    // register click event
    char* message = "pressed me!";
    NU_Register_Event(button, message, Press, NU_EVENT_ON_CLICK); 

    // app loop
    while(NU_Running())
    {
        NU_Unblock();
    }

    // free
    NU_Quit();
}
```


<br>

## Documentation (In Progress)

#### UI Nodes:
- window
- rect
- button
- table, thead, row
- image
- canvas

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

---

### Size
| Property | Description |
|--------|-------------|
| `width` `height` | Fixed size |
| `min-width` `min-height` | Minimum size |
| `max-width` `max-height` | Maximum size |

---

### Alignment
| Property | Description | Values |
|--------|-------------|----|
| `align-h` | Horizontal child alignment | String `left` `right` `center` |
| `align-v` | Vertical child alignment | String `top` `bottom` `center` |
| `text-align-h` | Horizontal text alignment | String `left` `right` `center` |
| `text-align-v` | Vertical text alignment | String `top` `bottom` `center` |

---

### Colours
| Property | Values |
|--------|-------|
| `background` | Hex code `#xxxxxx` |
| `border-colour` | Hex code `#xxxxxx` |
| `text-colour` | Hex code `#xxxxxx` |

---

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

---

### Padding
| Property | Description | Values |
|--------|-------|-----|
| `padding` | All sides | Int |
| `padding-top` `bottom` `left` `right` | Individual sides | Int |
