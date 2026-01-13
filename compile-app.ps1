# Compile nodus into a library
".\compile.ps1"
# Compile nodus into a library










# Compile test app
$sdlLib = "src\thirdParty\SDL3\lib" 
$sdlInclude = "src\thirdParty\SDL3\include"
$glewInclude = "src\thirdParty\glew\include"
$glewLib = "src\thirdParty\glew\lib"
$freetypeInclude = "src\thirdParty\freetype\include"
$freetypeLib = "src\thirdParty\freetype\lib"
$nodusInclude = "nodus\include"
$nodusLib = "nodus\lib"

clang -std=c99 -O2 test.c `
-I"$glewInclude" `
-I"$sdlInclude" `
-I"$freetypeInclude" `
-I"$nodusInclude" `
-L"$glewLib" `
-L"$sdlLib" `
-L"$freetypeLib" `
-L"$nodusLib" `
-lglew32 -lSDL3 -lopengl32 -lgdi32 -lfreetype -fopenmp -lnodus `
-o app/app.exe -Wno-deprecated-declarations
