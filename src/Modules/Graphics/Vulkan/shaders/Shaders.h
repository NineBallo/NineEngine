//
// Created by nineball on 7/21/21.
//

#ifndef NINEENGINE_SHADERS_H
#define NINEENGINE_SHADERS_H
#include <string>
#include <vector>
#include "shaderc/shaderc.hpp"

//vertex, fragment
std::pair<std::string, std::string> assembleShaders(uint32_t flags);


std::vector<uint32_t> compileShader(const std::string& source_name, shaderc_shader_kind kind,
                   const std::string& source, bool optimize = false);

std::string preProcessShader(const std::string& shader, const std::string& name, shaderc_shader_kind kind);



#endif //NINEENGINE_SHADERS_H
