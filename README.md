# NineEngine

This aims to be a hopefully straightforward engine to use that will probably end up having a lot of niche features and random options for personal learning and tinkering reasons.

## Building
### Depedencies
Cmake should handle most dependencies except for:
- vulkan, glfw, and glm

### Compiling
- Run "cmake ./"
- Run "make -j (cores)"

## RoadMap
- ~~Finish refactoring all the vulkan code into "bite-sized" chunks/classes~~
- ~~Get a triangle lol~~
- Lightweight Cuda/Vulkan/opencl raytracer (Have some ideas I want to experiment with)
- Utilize the igpu for some calculations, this will take some research.
- Add opengl at some point
- Maybe add DX12 but unsure
