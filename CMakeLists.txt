cmake_minimum_required(VERSION 3.19)
project(NineEngine)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)


find_package(Vulkan REQUIRED FATAL_ERROR) # error
find_package(glfw3 REQUIRED FATAL_ERROR) # error
find_package(glm REQUIRED FATAL_ERROR) # error


add_executable(NineEngine src/Graphics/Vulkan/Vulkan.cpp src/Graphics/Vulkan/Vulkan.h src/Graphics/Vulkan/LeStuff/Pipeline.cpp src/Graphics/Vulkan/LeStuff/Pipeline.h src/Graphics/Vulkan/LeStuff/Swapchain.cpp src/Graphics/Vulkan/LeStuff/Swapchain.h src/Graphics/Vulkan/LeStuff/Renderpass.cpp src/Graphics/Vulkan/LeStuff/Renderpass.h src/Graphics/Vulkan/LeStuff/Device.cpp src/Graphics/Vulkan/LeStuff/Device.h src/Graphics/Vulkan/LeStuff/Instance.cpp src/Graphics/Vulkan/LeStuff/Instance.h src/main.cpp)
target_link_libraries (NineEngine glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)


add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/Graphics/Vulkan/Shaders/compiled/* $<TARGET_FILE_DIR:NineEngine>/Shaders)
