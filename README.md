Nodus

Nodus is a lightweight, high-performance GUI library written in C.
It uses XML templates for layout, CSS for styling, and an HTML DOM-style tree model that can be transformed through code.

Nodus is designed for developers who want stylable super performant UI, multiple native windows, and full control over performance and memory.
The layout engine is flexbox based - it is easy to create reactive UIs.
Everything is driven by node tags - Foe example: subwindows can be created defining a <window> tag.


Features

- XML-based UI layout

- CSS-style styling system

- DOM-like node tree

- Event-driven architecture

- Pure C API

- Canvas API

Example
XML Layout
<window width="800" height="600">
    <div class="container">
        <canvas id="interactive chart"/>
        <button id="btn_ok">OK</button>
    </div>
</window>

CSS Styling
.container {
    pad: 16px;
}

button {
    backgroundColour: #3a7afe;
    textColour: #ffffff;
    borderRadius: 4px;
}

Features In Development
- Text inputs
- Auto scroll behaviour
- Expanded canvas API
- XML components: reusable UI components constructed from xml template files
