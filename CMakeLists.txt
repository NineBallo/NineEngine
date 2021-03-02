cmake_minimum_required(VERSION 3.19)
project(NineEngine)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_BUILD_TYPE Debug)



find_package(Vulkan REQUIRED FATAL_ERROR) # error

find_package(glfw3 REQUIRED FATAL_ERROR) # error

find_package(glm REQUIRED FATAL_ERROR) # error

add_executable(NineEngine Sandbox/main.cpp src/Graphics/Vulkan/Bootstrap/VulkanInstance.cpp src/Graphics/Vulkan/Bootstrap/VulkanInstance.h src/Devices/Window.cpp src/Devices/Window.h src/Application.cpp src/Application.h src/NineEngine.h Sandbox/main.h src/Graphics/Vulkan/Bootstrap/Surface.cpp src/Graphics/Vulkan/Bootstrap/Surface.h src/Graphics/Vulkan/Vulkan.cpp src/Graphics/Vulkan/Vulkan.h src/Graphics/Vulkan/Pipelines/GraphicsPipeline.cpp src/Graphics/Vulkan/Pipelines/GraphicsPipeline.h src/Graphics/Vulkan/vkGlobalPool.h src/Graphics/Vulkan/Devices/Device.cpp src/Graphics/Vulkan/Devices/Device.h src/Graphics/Vulkan/RenderPass/Swapchain.cpp src/Graphics/Vulkan/RenderPass/Swapchain.h src/Graphics/Vulkan/Pipelines/RenderPass.cpp src/Graphics/Vulkan/Pipelines/RenderPass.h)

target_link_libraries (NineEngine glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/Graphics/Vulkan/Shaders/glsl/compiled/* $<TARGET_FILE_DIR:NineEngine>/Shaders)
