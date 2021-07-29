//
// Created by nineball on 7/21/21.
//

#ifndef NINEENGINE_VERTEX_H
#define NINEENGINE_VERTEX_H
#include "string"
#include "iostream"

std::string vertexBase = "#version 460\n"
                         "layout (location = 0) in vec3 vPosition;\n"
                         "layout (location = 1) in vec3 vNormal;\n"
                         "layout (location = 2) in vec3 vColor;\n"
                         "layout (location = 3) in vec2 vTexCoord;\n"
                         "\n"
                         "layout(set = 0, binding = 0) uniform CameraBuffer{\n"
                         "    mat4 view;\n"
                         "    mat4 proj;\n"
                         "    mat4 viewproj;\n"
                         "} cameraData;\n"
                         "\n"
                         "struct ObjectData{\n"
                         "    mat4 model;\n"
                         "};\n"
                         "\n"
                         "//all object matrices\n"
                         "layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{\n"
                         "    ObjectData objects[];\n"
                         "} objectBuffer;\n";

//Passing in color directly
std::string vertexColor = "layout (location = 0) out vec3 outColor;\n"
                          "void main() {\n"
                          "    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;\n"
                          "    mat4 transformMatrix = (cameraData.viewproj * modelMatrix);\n"
                          "    gl_Position = transformMatrix * vec4(vPosition, 1.0);\n"
                          "    outColor = vColor;\n"
                          "}\n";

//Passing in color through texture
std::string vertexTexture = "layout (location = 0) out vec2 texCoord;\n"
                            "\n"
                            "layout(push_constant) uniform VertexData\n"
                            "{\n"
                            "    int texIdx;\n"
                            "    int entityID;\n"
                            "\n"
                            "} vertexData;\n"
                            "\n"
                            " void main() {\n"
                            "    mat4 modelMatrix = objectBuffer.objects[vertexData.entityID].model;\n"
                            "    mat4 transformMatrix = (cameraData.viewproj * modelMatrix);\n"
                            "    gl_Position = transformMatrix * vec4(vPosition, 1.0);\n"
                            "    texCoord = vTexCoord;\n"
                            "}";







#endif //NINEENGINE_VERTEX_H
