//
// Created by nineball on 7/21/21.
//

#ifndef NINEENGINE_FRAGMENT_H
#define NINEENGINE_FRAGMENT_H
#include <string>


//output write
std::string fragmentBase = "#version 450\n"
                           ""
                           "layout (location = 0) out vec4 outColor;\n"
                           "\n"
                           "layout(set = 0, binding = 1) uniform  SceneData{\n"
                           "    vec4 fogColor; // w is for exponent\n"
                           "    vec4 fogDistances; //x for min, y for max, zw unused.\n"
                           "    vec4 ambientColor;\n"
                           "    vec4 sunlightDirection; //w for sun power\n"
                           "    vec4 sunlightColor;\n"
                           "} sceneData;\n";

//Build with sampler and texcoord passthrough
std::string fragmentTexture = "#extension GL_EXT_nonuniform_qualifier : require\n"
                              "layout (location = 0) in vec2 uv;\n"
                              "layout(set = 2, binding = 0) uniform sampler2D combinedSampler[];\n"
                              "\n"
                              "layout(push_constant) uniform FragData\n"
                              "{\n"
                              "     int texIdx;\n"
                              "     int entityID;\n"
                              "\n"
                              "} fragData;\n"
                              ""
                              "void main()\n"
                              "{\n"
                              "    outColor = texture(combinedSampler[fragData.texIdx], uv) + sceneData.ambientColor;\n"
                              "}";

//Build with sampler and texcoord passthrough
std::string fragmentTextureBinding = "layout (location = 0) in vec2 uv;\n"
                                     "layout(set = 2, binding = 0) uniform sampler samp;\n"
                                     "layout(set = 2, binding = 1) uniform texture2D tex;\n"
                                     "\n"
                                     ""
                                     "void main()\n"
                                     "{\n"
                                     "    outColor = texture(sampler2D(tex, samp), uv) + sceneData.ambientColor;\n"
                                     "}";


//Build with direct passthrough
std::string fragmentColor = "layout (location = 0) in vec3 inColor;\n"
                            "outFragColor = vec4(inColor + sceneData.ambientColor.xyz, 1.0f);\n";


#endif //NINEENGINE_FRAGMENT_H
