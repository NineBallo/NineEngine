cmake_minimum_required(VERSION 3.8)
project(NEVK)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

find_package(Vulkan REQUIRED FATAL_ERROR) # error
find_package(glfw3 REQUIRED FATAL_ERROR) # error
find_package(glm REQUIRED FATAL_ERROR) # error

#Include dirs
include_directories(include)

#VK_BOOTSTRAP
include(FetchContent)

FetchContent_Declare(
        vk_bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG        master #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)
FetchContent_MakeAvailable(vk_bootstrap)

FetchContent_Declare(
        spirv-headers
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
        GIT_TAG master
)
FetchContent_MakeAvailable(spirv-headers)

FetchContent_Declare(
        spirv-tools
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
        GIT_TAG        master #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)

FetchContent_MakeAvailable(spirv-tools)


file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/shaders/*.comp"
        )


message(STATUS "GIT modules fetched")

#BUILDING PROJECT
add_library(NEVK STATIC src/Vulkan.cpp include/Display.h src/Display.cpp include/Types.h include/Initializers.h src/Initializers.cpp include/Pipeline.h src/Pipeline.cpp)

target_link_libraries(NEVK vk-bootstrap::vk-bootstrap glfw vulkan)


#SHADER STUFF

message(STATUS "Building shaders")
find_program(GLSLC glslc HINTS /usr/bin /usr/local/bin)

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp"
        )


if(NOT (EXISTS ${CMAKE_BINARY_DIR}/shaders))
    message(STATUS "Shader dir not found, generating...")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
endif()

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

add_dependencies(NEVK Shaders)