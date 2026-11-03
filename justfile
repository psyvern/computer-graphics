install:
    -git clone https://github.com/assimp/assimp
    -git clone https://github.com/nothings/stb
    mkdir -p build
    cd build && cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1

[working-directory: "build"]
build:
    make

run: build
    ./build/main

clean:
    rm -rf \
        assimp \
        stb \
        build
