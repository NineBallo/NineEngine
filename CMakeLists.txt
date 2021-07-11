cmake_minimum_required(VERSION 3.8)
project(NineEngine)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(src/Modules/Graphics/Vulkan)

add_executable(NineEngine src/NineEngine.cpp src/NineEngine.h src/ThirdParty/stb_image.h src/Managers/ECS/EntityManager.h src/Managers/ECS/ComponentArray.h src/Managers/ECS/Shared.h src/Managers/ECS/ComponentManager.h src/Managers/ECS/SystemManager.h src/Managers/ECS/Coordinator.h src/Managers/Engine/Engine.h src/Managers/Engine/Engine.cpp)
target_link_libraries (NineEngine NEVK)
target_include_directories(NineEngine PRIVATE src/ThirdParty src/Handler/include src/Managers/ECS/ src/Managers/Engine)