# Nodus

Nodus is a lightweight, high-performance GUI library written in C.
It uses XML templates for layout, CSS for styling, and an HTML DOM-style tree model that can be transformed through code.

Nodus is designed for developers who want stylable super performant UI, multiple native windows, and full control over performance and memory.
The layout engine is flexbox based - it is easy to create reactive UIs.
Everything is driven by node tags - Foe example: subwindows can be created defining a <window> tag.

This library is in the very early stages of development. The vision is to create a comprehensive but super simple UI library that runs on
Windows MacOS and Linux. The project targets desktop development as there are shockingly few options out there to create native 
Desktop apps, especially for the C language. Who wants to use QT and c++, gross!

## Features:
- XML-based UI layout
- CSS-style styling system
- DOM-like node tree
- Event-driven architecture
- Pure C API
- Canvas API

XML Template Example
XML Layout
<window width="800" height="600">
    <div class="container">
        <canvas id="interactive chart"/>
        <button id="btn_ok">OK</button>
    </div>
</window>


CSS Stylesheet Example
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
