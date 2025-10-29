$headersInclude = "headers"
$sdlLib = "lib\SDL3\lib" 
$sdlInclude = "lib\SDL3\include"
$glewInclude = "lib\glew\include"
$glewLib = "lib\glew\lib"
$freetypeInclude = "lib\freetype\include"
$freetypeLib = "lib\freetype\lib"
$harfbuzzInclude = "lib\harfbuzz\include"
$harfbuzzLib = "lib\harfbuzz\lib"

clang -std=c99 library.c `
-I"$headersInclude" `
-I"$glewInclude" `
-I"$sdlInclude" `
-I"$freetypeInclude" `
-I"$harfbuzzInclude" `
-L"$glewLib" `
-L"$sdlLib" `
-L"$freetypeLib" `
-L"$harfbuzzLib" `
-lglew32 -lSDL3 -lopengl32 -lgdi32 -lfreetype -lharfbuzz `
-shared `
-o "nodus\lib\nodus.dll" -Wno-deprecated-declarations

Remove-Item "nodus\lib\nodus.exp"