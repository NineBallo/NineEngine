cmake_minimum_required(VERSION 3.19)
project(NineEngine)

set(CMAKE_CXX_STANDARD 20)

#add_link_options("-fuse-ld=lld")

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
find_package(Vulkan REQUIRED FATAL_ERROR) # error

message(STATUS "Using ${CMAKE_GENERATOR}")
message(STATUS "Using ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Using ${CMAKE_LINKER}")

include(FetchContent)
set(GLFW_STANDALONE ON)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.4 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_GetProperties(glfw)

if(NOT glfw_POPULATED)
    FetchContent_Populate(glfw)
    add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
endif()

set(GLM_TEST_ENABLE OFF)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        0.9.9.8 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_GetProperties(glm)

if(NOT glm_POPULATED)
    FetchContent_Populate(glm)
    add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})
endif()

FetchContent_Declare(
        vk_bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG        master #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(vk_bootstrap)

FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG        v2.3.0   #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(vma)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/NineBallAYAYA/imgui
        GIT_TAG        docking   #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(imgui)

set(SPIRV_HEADERS_SKIP_INSTALL ON)
set(SPIRV_HEADERS_SKIP_EXAMPLES ON)
FetchContent_Declare(
        spirv-headers
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
        GIT_TAG sdk-1.2.198.0
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(spirv-headers)

set(SKIP_SPIRV_TOOLS_INSTALL ON)
set(SPIRV_TOOLS_BUILD_STATIC ON)
FetchContent_Declare(
        spirv-tools
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
        GIT_TAG        sdk-1.2.198.0 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)

FetchContent_MakeAvailable(spirv-tools)


set(ENABLE_CTEST OFF)
set(SKIP_GLSLANG_INSTALL ON)
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_HLSL ON)
FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang
        GIT_TAG        sdk-1.2.198.0 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(glslang)


set(SHADERC_SKIP_INSTALL ON)
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)

FetchContent_Declare(
        shaderc
        GIT_REPOSITORY https://github.com/google/shaderc
        GIT_TAG        v2021.3  #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(shaderc)

set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp
        GIT_TAG        v5.1.5 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)
FetchContent_GetProperties(assimp)

if(NOT assimp_POPULATED)
    FetchContent_Populate(assimp)
    add_subdirectory(${assimp_SOURCE_DIR} ${assimp_BINARY_DIR})
endif()

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG        master #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
        GIT_SHALLOW 1
)

FetchContent_MakeAvailable(stb)

message(STATUS "GIT modules fetched")

set(imgui_FILES ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)

add_executable(NineEngine ${imgui_FILES} src/NineEngine.cpp src/NineEngine.h src/Managers/Engine/Engine.h src/Managers/Engine/Engine.cpp src/Modules/IO/Keyboard.cpp src/Modules/IO/Keyboard.h src/Modules/IO/Mouse.cpp src/Modules/IO/Mouse.h src/Managers/ECS/ECS.h src/Modules/Graphics/Vulkan/src/Vulkan.cpp src/Modules/Graphics/Vulkan/include/Display.h src/Modules/Graphics/Vulkan/src/Display.cpp src/Modules/Graphics/Vulkan/include/Types.h src/Modules/Graphics/Vulkan/include/Initializers.h  src/Modules/Graphics/Vulkan/src/Initializers.cpp src/Modules/Graphics/Vulkan/include/Pipeline.h  src/Modules/Graphics/Vulkan/src/Pipeline.cpp src/Modules/Graphics/Vulkan/include/Mesh.h src/Modules/Graphics/Vulkan/src/Mesh.cpp src/Modules/Graphics/Vulkan/include/Device.h src/Modules/Graphics/Vulkan/src/Device.cpp src/Modules/Graphics/Vulkan/include/Common.h src/Managers/ECS/Common.h src/Modules/Graphics/Vulkan/include/Textures.h src/Modules/Graphics/Vulkan/src/Textures.cpp src/Modules/Graphics/Vulkan/shaders/Shaders.cpp src/Modules/Graphics/Vulkan/shaders/Shaders.h src/Modules/Graphics/Common/ImGuiHelpers.h src/Modules/Graphics/Common/ImGuiHelpers.cpp)

link_directories(${shaderc_BINARY_DIR} ${glfw_BINARY_DIR} ${glm_BINARY_DIR})
target_link_libraries(NineEngine vk-bootstrap::vk-bootstrap glfw glm vulkan shaderc assimp)

target_include_directories(NineEngine PUBLIC
        ${vma_SOURCE_DIR}/src ${imgui_SOURCE_DIR} ${stb_SOURCE_DIR} ${assimp_SOURCE_DIR}/include ${glfw_SOURCE_DIR}/include
        ${glm_SOURCE_DIR})


file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/shaders/*.comp"
        )

if(NOT (EXISTS ${CMAKE_BINARY_DIR}/shaders))
    message(STATUS "Shader dir not found, generating...")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders/Vert)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders/Frag)
endif()

if(NOT (EXISTS ${CMAKE_BINARY_DIR}/models))
    message(STATUS "Model dir not found, generating...")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/models)
endif()

if(NOT (EXISTS ${CMAKE_BINARY_DIR}/models/textures))
    message(STATUS "Texture dir not found, generating...")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/models/textures)
endif()

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Modules/Graphics/Vulkan/models/* ${CMAKE_BINARY_DIR}/models)

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Modules/Graphics/Vulkan/models/textures/* ${CMAKE_BINARY_DIR}/models/textures)

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Modules/Graphics/Vulkan/shaders/Vert/* ${CMAKE_BINARY_DIR}/shaders/Vert)

add_custom_command(TARGET NineEngine PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Modules/Graphics/Vulkan/shaders/Frag/* ${CMAKE_BINARY_DIR}/shaders/Frag)

#SHADER STUFF
message(STATUS "Building shaders")
find_program(GLSLC glslc HINTS /usr/bin /usr/local/bin)

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/Modules/Graphics/Vulkan/shaders/*.frag"
        "${CMAKE_SOURCE_DIR}/src/Modules/Graphics/Vulkan/shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/src/Modules/Graphics/Vulkan/shaders/*.comp"
        )

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    message(STATUS ${SPIRV})

    ##execute glslang command to compile that specific shader
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(NineEngine Shaders)

target_include_directories(NineEngine PRIVATE src/Modules/Graphics/Vulkan/include src/Handler/include src/Managers/ECS/ src/Managers/Engine)