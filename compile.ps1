$headersInclude = "src\headers"
$sdlLib = "src\thirdParty\SDL3\lib" 
$sdlInclude = "src\thirdParty\SDL3\include"
$glewInclude = "src\thirdParty\glew\include"
$glewLib = "src\thirdParty\glew\lib"
$freetypeInclude = "src\thirdParty\freetype\include"
$freetypeLib = "src\thirdParty\freetype\lib"
clang -std=c99 -O3 "src\nodus_library.c" `
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
Remove-Item nodus\lib\nodus.exp -Force


# Replace old nodus.dll with updated one
$sourceDll = "nodus\lib\nodus.dll"
$destDll = "app\nodus.dll"
if (Test-Path $sourceDll) {
    if (Test-Path $destDll) {
        Remove-Item $destDll -Force
    }
    Copy-Item $sourceDll $destDll -Force
    Write-Host "✅ Updated nodus.dll in build folder."
} else {
    Write-Host "❌ nodus.dll not found in $sourceDll"
}