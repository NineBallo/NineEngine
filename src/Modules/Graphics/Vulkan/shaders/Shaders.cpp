//
// Created by nineball on 7/21/21.
//

#include "Shaders.h"

#include "Vertex.h"
#include "Fragment.h"
#include "Types.h"

std::pair<std::string, std::string> assembleShaders(uint32_t flags) {
    std::string vertex = vertexBase;
    std::string fragment = fragmentBase;

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
        vertex += vertexTexture;
        if(bindless) {
            fragment += fragmentTexture;
        }
        else {
            fragment += fragmentTextureBinding;
        }

    }
    else {
        vertex += vertexColor;
        fragment += fragmentColor;
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