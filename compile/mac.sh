#!/bin/bash

# Paths
srcInclude="src"
sdlLib="src/libraries/sdl3/mac"
sdlInclude="src/libraries/sdl3/include"
glewInclude="src/libraries/glew/include"
glewLib="src/libraries/glew/mac"
freetypeInclude="src/libraries/freetype/include"
freetypeLib="src/libraries/freetype/mac"
vulkanSdk="/Users/$(whoami)/VulkanSDK/1.4.363.0/macOS"

# Compile
clang -std=c99 -O3 -g -ftrivial-auto-var-init=zero\
    "src/nodus.c" \
    "src/mmap/nu_memory_map.c" \
    -I"$srcInclude" \
    -I"$glewInclude" \
    -I"$sdlInclude" \
    -I"$freetypeInclude" \
    -I"$srcInclude/mmap" \
    src/shaders/build/*.c \
    "$glewLib/libGLEW.a" \
    "$freetypeLib/libfreetype.a" \
    "$sdlLib/libSDL3.a" \
    "$vulkanSdk/lib/libvulkan.1.dylib" \
    -lbz2 \
    -lz \
    -framework AVFoundation \
    -framework AudioToolbox \
    -framework CoreAudio \
    -framework CoreFoundation \
    -framework CoreGraphics \
    -framework CoreHaptics \
    -framework CoreMedia \
    -framework CoreVideo \
    -framework GameController \
    -framework IOKit \
    -framework Metal \
    -framework QuartzCore \
    -framework Security \
    -framework UniformTypeIdentifiers \
    -framework UserNotifications \
    -framework Foundation \
    -framework AppKit \
    -framework OpenGL \
    -framework Cocoa \
    -framework ForceFeedback \
    -framework Carbon \
    -shared \
    -Wl,-rpath,@loader_path \
    -o "nodus/mac/libnodus.dylib" \
    -Wno-deprecated-declarations

# Remove the .exp file if it exists (equivalent to Windows .exp)
rm -f nodus/mac/libnodus.exp

# Copy nodus.h into nodus/mac/include
cp src/nodus.h nodus/include/
