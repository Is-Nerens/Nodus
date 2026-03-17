$srcInclude = "src"
$sdlLib = "src\libraries\SDL3\lib" 
$sdlInclude = "src\libraries\SDL3\include"
$glewInclude = "src\libraries\glew\include"
$glewLib = "src\libraries\glew\lib"
$freetypeInclude = "src\libraries\freetype\include"
$freetypeLib = "src\libraries\freetype\lib"
clang -std=c99 -O3 -fopenmp "src\nz_library.c" `
-I"$srcInclude" `
-I"$glewInclude" `
-I"$sdlInclude" `
-I"$freetypeInclude" `
-L"$glewLib" `
-L"$sdlLib" `
-L"$freetypeLib" `
-lglew32 -lSDL3 -lopengl32 -lgdi32 -lfreetype -ladvapi32 `
"-Wl,/SUBSYSTEM:WINDOWS" `
-shared `
-o "nodus\lib\nodus.dll" -Wno-deprecated-declarations
Remove-Item nodus\lib\nodus.exp -Force


# Replace old nodus.dll with updated one
$sourceDll = "nodus\lib\nodus.dll"
$destDll = "app-test\build\nodus.dll"
if (Test-Path $sourceDll) {
    if (Test-Path $destDll) {
        Remove-Item $destDll -Force
    }
    Copy-Item $sourceDll $destDll -Force
    Write-Host "✅ Updated nodus.dll in build folder."
} else {
    Write-Host "❌ nodus.dll not found in $sourceDll"
}