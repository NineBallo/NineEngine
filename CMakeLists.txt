cmake_minimum_required(VERSION 3.19)
project(NineEngine)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE on)


find_package(Vulkan REQUIRED FATAL_ERROR) # error
find_package(glfw3 REQUIRED FATAL_ERROR) # error
find_package(glm REQUIRED FATAL_ERROR) # error


add_executable(NineEngine src/Graphics/Vulkan/Vulkan.cpp src/Graphics/Vulkan/Vulkan.h Application/app.cpp Application/app.h src/ThirdParty/stb_image.h src/Graphics/Vulkan/Extra/Loader.cpp src/Graphics/Vulkan/Core/Renderer.cpp src/Graphics/Vulkan/Core/Renderer.h src/Graphics/Vulkan/Core/Device/NEDevice.cpp src/Graphics/Vulkan/Core/Device/NEDevice.h src/Graphics/Vulkan/Core/Device/NEWindow.cpp src/Graphics/Vulkan/Core/Device/NEWindow.h src/Graphics/Vulkan/Core/Device/NESwapchain.cpp src/Graphics/Vulkan/Core/Device/NESwapchain.h src/Graphics/Vulkan/Core/NEInstance.cpp src/Graphics/Vulkan/Core/NEInstance.h src/Graphics/Vulkan/Core/Device/NEPipeline.cpp src/Graphics/Vulkan/Core/Device/NEPipeline.h src/Graphics/Vulkan/Core/Device/NERenderpass.cpp src/Graphics/Vulkan/Core/Device/NERenderpass.h src/Managers/EntityManager.cpp src/Managers/EntityManager.h)
target_link_libraries (NineEngine glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)
target_include_directories(NineEngine PRIVATE src/ThirdParty)

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/Graphics/Vulkan/Shaders/compiled/* $<TARGET_FILE_DIR:NineEngine>/Shaders)
