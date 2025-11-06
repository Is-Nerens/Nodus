$headersInclude = "headers"
$sdlLib = "lib\SDL3\lib" 
$sdlInclude = "lib\SDL3\include"
$glewInclude = "lib\glew\include"
$glewLib = "lib\glew\lib"
$freetypeInclude = "lib\freetype\include"
$freetypeLib = "lib\freetype\lib"

clang -std=c99 -O2 library.c `
-I"$headersInclude" `
-I"$glewInclude" `
-I"$sdlInclude" `
-I"$freetypeInclude" `
-L"$glewLib" `
-L"$sdlLib" `
-L"$freetypeLib" `
-lglew32 -lSDL3 -lopengl32 -lgdi32 -lfreetype `
-shared `
-o "nodus\lib\nodus.dll" -Wno-deprecated-declarations

Remove-Item "nodus\lib\nodus.exp"