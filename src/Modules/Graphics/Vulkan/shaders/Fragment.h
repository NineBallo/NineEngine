//
// Created by nineball on 7/21/21.
//

#ifndef NINEENGINE_FRAGMENT_H
#define NINEENGINE_FRAGMENT_H
#include <string>


//output write
std::string fragmentBase = "#version 450\n"
                           "layout (location = 0) out vec4 outFragColor;\n"
                           "\n"
                           "layout(set = 0, binding = 1) uniform  SceneData{\n"
                           "    vec4 fogColor; // w is for exponent\n"
                           "    vec4 fogDistances; //x for min, y for max, zw unused.\n"
                           "    vec4 ambientColor;\n"
                           "    vec4 sunlightDirection; //w for sun power\n"
                           "    vec4 sunlightColor;\n"
                           "} sceneData;";

//Build with sampler and texcoord passthrough
std::string fragmentTexture = "layout (location = 0) in vec2 texCoord;"
                              "layout(set = 2, binding = 0) uniform sampler2D tex1;"
                              ""
                              "void main()\n"
                              "{\n"
                              "    vec3 color = texture(tex1,texCoord).xyz;\n"
                              "    outFragColor = vec4(color + sceneData.ambientColor.xyz, 1.0f);\n"
                              "}";

//Build with direct passthrough
std::string fragmentColor = "layout (location = 0) in vec3 inColor;\n"
                            "outFragColor = vec4(inColor + sceneData.ambientColor.xyz, 1.0f);\n";


#endif //NINEENGINE_FRAGMENT_H
