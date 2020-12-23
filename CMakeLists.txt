cmake_minimum_required(VERSION 3.19)
project(NineEngine)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

find_package(glfw3 REQUIRED FATAL_ERROR) # error

add_executable(NineEngine src/main.cpp src/Devices/PhysicalDevice.cpp src/Devices/PhysicalDevice.h src/Graphics/Vulkan/Instance.cpp src/Graphics/Vulkan/Instance.h src/Graphics/Window.cpp src/Graphics/Window.h)

target_link_libraries (NineEngine glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)
