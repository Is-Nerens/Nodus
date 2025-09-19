
$headersInclude = "headers"
$sdlLib = "lib\SDL3\lib" 
$sdlInclude = "lib\SDL3\include"
$glewInclude = "lib\glew\include"
$glewLib = "lib\glew\lib"
$nanovgInclude = "lib\nanoVG"
$freetypeInclude = "lib\freetype\include"
$freetypeLib = "lib\freetype\lib"

# Use nanovg.c instead of nanovg_gl.c
clang -std=c99 main.c `
"lib\nanoVG\nanovg.c" `
-I"$headersInclude" `
-I"$glewInclude" `
-I"$sdlInclude" `
-I"$nanovgInclude" `
-I"$freetypeInclude" `
-L"$glewLib" `
-L"$sdlLib" `
-L"$freetypeLib" `
-lglew32 -lSDL3 -lopengl32 -lgdi32 -lfreetype `
-o build/app.exe -Wno-deprecated-declarations `


