cmake_minimum_required(VERSION 3.19)
project(NineEngine)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE on)


find_package(Vulkan REQUIRED FATAL_ERROR) # error
find_package(glfw3 REQUIRED FATAL_ERROR) # error
find_package(glm REQUIRED FATAL_ERROR) # error


add_executable(NineEngine src/NineEngine.cpp src/NineEngine.h src/Graphics/Vulkan/Vulkan.cpp src/Graphics/Vulkan/Vulkan.h Application/app.cpp Application/app.h src/ThirdParty/stb_image.h src/Graphics/Vulkan/Renderer.cpp src/Graphics/Vulkan/Renderer.h src/Graphics/Vulkan/Core/src/NEDevice.cpp src/Graphics/Vulkan/Core/NEDevice.h src/Graphics/Vulkan/Core/src/NEInstance.cpp src/Graphics/Vulkan/Core/NEInstance.h src/Graphics/Vulkan/Core/src/NEPipeline.cpp src/Graphics/Vulkan/Core/NEPipeline.h src/Graphics/Vulkan/Core/src/NERenderpass.cpp src/Graphics/Vulkan/Core/NERenderpass.h src/Managers/ECS/EntityManager.h src/Managers/ECS/ComponentArray.h src/Managers/ECS/Shared.h src/Managers/ECS/ComponentManager.h src/Managers/ECS/SystemManager.h src/Managers/ECS/Coordinator.h src/Graphics/Vulkan/Core/NEShared.h src/Graphics/Vulkan/Helper/Loader.h src/Graphics/Vulkan/Helper/Device.h src/Graphics/Vulkan/Helper/Pipeline.h src/Graphics/Vulkan/Helper/Display.h src/Graphics/Vulkan/Helper/src/Device.cpp src/Graphics/Vulkan/Helper/src/Loader.cpp src/Graphics/Vulkan/Helper/src/Pipeline.cpp src/Graphics/Vulkan/Helper/src/Display.cpp src/Graphics/Vulkan/Core/src/NEDisplay.cpp src/Graphics/Vulkan/Core/NEDisplay.h src/Managers/Engine/Engine.h src/Managers/Engine/Engine.cpp)
target_link_libraries (NineEngine glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)
target_include_directories(NineEngine PRIVATE src/ThirdParty src/Handler/include src/Managers/ECS/ src/Managers/Engine)

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/src/Graphics/Vulkan/Shaders/compiled/* $<TARGET_FILE_DIR:NineEngine>/Shaders)
