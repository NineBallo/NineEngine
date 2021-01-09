cmake_minimum_required(VERSION 3.19)
project(NineEngine)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_BUILD_TYPE Debug)

find_package(Vulkan REQUIRED FATAL_ERROR) # error

find_package(glfw3 REQUIRED FATAL_ERROR) # error

add_executable(NineEngine Sandbox/main.cpp src/Graphics/Vulkan/Devices/PhysicalDevice.cpp src/Graphics/Vulkan/Devices/PhysicalDevice.h src/Graphics/Vulkan/Boilerplate/VulkanInstance.cpp src/Graphics/Vulkan/Boilerplate/VulkanInstance.h src/Devices/Window.cpp src/Devices/Window.h src/Application.cpp src/Application.h src/Graphics/Vulkan/Devices/LogicalDevice.cpp src/Graphics/Vulkan/Devices/LogicalDevice.h src/NineEngine.h Sandbox/main.h src/Graphics/Vulkan/Boilerplate/Surface.cpp src/Graphics/Vulkan/Boilerplate/Surface.h src/Graphics/Vulkan/Vulkan.cpp src/Graphics/Vulkan/Vulkan.h src/Graphics/Vulkan/Boilerplate/GraphicsPipeline.cpp src/Graphics/Vulkan/Boilerplate/GraphicsPipeline.h src/Graphics/Vulkan/vkGlobalPool.h)

target_link_libraries (NineEngine glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/Graphics/Vulkan/Shaders $<TARGET_FILE_DIR:NineEngine>/Shaders)
