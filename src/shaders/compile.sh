
#!/usr/bin/env bash
set -e

mkdir -p src/shaders/build

for shader in src/shaders/*.{vert,frag,geom,tesc,tese,comp}; do
    [[ -f "$shader" ]] || continue

    name=$(basename "$shader")
    base="${name%.*}"
    stage="${name##*.}"

    echo "Compiling $shader..."

    glslc "$shader" -o "src/shaders/build/$name.spv"

    xxd -i "src/shaders/build/$name.spv" \
        > "src/shaders/build/${base}_${stage}_spv.c"

    rm "src/shaders/build/$name.spv"
done

echo "Shader compilation complete."
