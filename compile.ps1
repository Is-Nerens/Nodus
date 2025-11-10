# Compile nodus into a library
& ".\compile_library.ps1"



# Replace old nodus.dll with updated one
$sourceDll = "nodus\lib\nodus.dll"
$destDll = "build\nodus.dll"
if (Test-Path $sourceDll) {
    if (Test-Path $destDll) {
        Remove-Item $destDll -Force
    }
    Copy-Item $sourceDll $destDll -Force
    Write-Host "✅ Updated nodus.dll in build folder."
} else {
    Write-Host "❌ nodus.dll not found in $sourceDll"
}



# Compile test app
$sdlLib = "lib\SDL3\lib" 
$sdlInclude = "lib\SDL3\include"
$glewInclude = "lib\glew\include"
$glewLib = "lib\glew\lib"
$freetypeInclude = "lib\freetype\include"
$freetypeLib = "lib\freetype\lib"
$nodusInclude = "nodus\include"
$nodusLib = "nodus\lib"

clang -std=c99 -O2 main.c `
-I"$glewInclude" `
-I"$sdlInclude" `
-I"$freetypeInclude" `
-I"$nodusInclude" `
-L"$glewLib" `
-L"$sdlLib" `
-L"$freetypeLib" `
-L"$nodusLib" `
-lglew32 -lSDL3 -lopengl32 -lgdi32 -lfreetype -fopenmp -lnodus `
-o build/app.exe -Wno-deprecated-declarations
