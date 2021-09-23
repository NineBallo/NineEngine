//
// Created by nineball on 7/21/21.
//

#include "Shaders.h"

#include "Types.h"

#include <fstream>
#include <sstream>
#include <iostream>

std::string readFile(std::string path) {
    std::ifstream file(path);

    //, std::ios::ate | std::ios::binary

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    //Create a buffer the size of the file
    size_t fileSize = (size_t) file.tellg();
    std::string buffer;
    buffer.reserve(fileSize);

    //Go to beginning of file them write to buffer
    std::ostringstream ss;
    ss << file.rdbuf();
    file.close();

    buffer = ss.str();

    return buffer;
}

std::pair<std::string, std::string> assembleShaders(uint32_t flags) {
    std::string vertex;
    std::string fragment;

    bool bindless = true;
    bool textured = false;

    if((flags & NE_SHADER_TEXTURE_BIT) == NE_SHADER_TEXTURE_BIT) {
        textured = true;
    }
    if((flags & NE_FLAG_BINDING_BIT) == NE_FLAG_BINDING_BIT) {
        bindless = false;
    }

    ///Assembly time
    if(textured) {
        vertex = readFile("./shaders/Vert/Texture.vert");

        if(bindless) {
            fragment = readFile("./shaders/Frag/Texture.frag");
        }
        else {
            fragment = readFile("./shaders/Frag/TextureBinding.frag");
        }
    }
    else {
        vertex = readFile("./shaders/Vert/Color.vert");
        fragment = readFile("./shaders/Frag/Color.frag");
    }

    //Vertex, Fragment
    std::pair<std::string, std::string> assembledShaders;

    assembledShaders.first = preProcessShader(vertex,  "Vertex", shaderc_glsl_vertex_shader);
    assembledShaders.second = preProcessShader(fragment, "Fragment", shaderc_glsl_fragment_shader);


    return assembledShaders;
}

std::string preProcessShader(const std::string& shader, const std::string& name, shaderc_shader_kind kind) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    shaderc::PreprocessedSourceCompilationResult result =
            compiler.PreprocessGlsl(shader, kind, name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << result.GetErrorMessage();
    }

    return {result.cbegin(), result.cend()};
}

std::vector<uint32_t> compileShader(const std::string& source_name, shaderc_shader_kind kind,
                                    const std::string& source, bool optimize) {

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    if(optimize) options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

    if(module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << module.GetErrorMessage();
        return {};
    }

    return {module.cbegin(), module.cend()};
}