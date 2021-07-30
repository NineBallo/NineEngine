//
// Created by nineball on 7/21/21.
//

#ifndef NINEENGINE_SHADERS_H
#define NINEENGINE_SHADERS_H
#include <string>
#include <vector>
#include "shaderc/shaderc.hpp"

#define NE_SHADER_TEXTURE_BIT 1 //0001
#define NE_SHADER_COLOR_BIT 2   //0010
#define NE_SHADER_PUSHCONSTANTS_BIT 4 //0100
#define NE_SHADER_BINDING_BIT 8 //1000

//vertex, fragment
std::pair<std::string, std::string> assembleShaders(uint32_t flags);


std::vector<uint32_t> compileShader(const std::string& source_name, shaderc_shader_kind kind,
                   const std::string& source, bool optimize = false);

std::string preProcessShader(const std::string& shader, const std::string& name, shaderc_shader_kind kind);



#endif //NINEENGINE_SHADERS_H
