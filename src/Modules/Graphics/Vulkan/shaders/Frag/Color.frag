
#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inColor;
layout (location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D combinedSampler[];

layout(set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;

layout(push_constant) uniform FragData
{
    int texIdx;
    int entityID;
} fragData;

void main()
{
   outColor = vec4(inColor.xyz + sceneData.ambientColor.xyz, 1.f);
}