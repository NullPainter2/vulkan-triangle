glslc assets/shader.vert -o ./exe/vert.spv
glslc assets/shader.frag -o ./exe/frag.spv
g++ -std=c++17 src/linux/main.cpp -Wno-write-strings -Wpedantic -g -o exe/main -lX11
