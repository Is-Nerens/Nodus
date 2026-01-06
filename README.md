# Nodus

Nodus is a lightweight, high-performance retained GUI library for C.
It uses XML templates for layout, CSS for styling, and a DOM-style tree model that can be manipulated with code.

<br></br>
Special thanks to Nick Barker for the work he did on the [Clay](https://github.com/nicbarker/clay) library<br>
<br></br>

## Features:
- XML templates and CSS styling
- Flexbox layout model for responsive layouts
- Text rendering using FreeType with support for subpixel rendering, producing sharp text on lower resolution displays
- Canvas API that supports drawing
- JavasScript like UI events to detect things liek resizing, mouse hovering, clicks etc
- Multi-window support is as easy as creating a <window> tag (All child elements of a window will be drawn in that window)

## Features Still In Development
- Text inputs
- Auto scroll behaviour
- Scrollbar customisation
- DPI Scalling
- Expanded canvas API functionality (text, and more tools)
- XML components: reusable UI components constructed from xml template files
- Linux and MacOS support
- Debugging tools
- Library Documentation

#### XML Template Example
```xml
<window width="800" height="600">
    <div class="container" grow="b">
        <canvas id="interactive_chart" grow="b"/>
        <button id="btn_ok" grow="h">OK</button>
    </div>
</window>
```


#### CSS Stylesheet Example
```css
@font font-normal {
    src: ./fonts/Inter/Inter_Variable_Weight.ttf;
    size: 14;
    weight: 400;
}
.container {
    pad: 16;
}
button {
    backgroundColour: #3a7afe;
    textColour: #ffffff;
    borderRadius: 4;
}
```
