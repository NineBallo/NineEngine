#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;


layout (location = 1) in vec3 diffuse;

layout(set = 2, binding = 0) uniform sampler samp;
layout(set = 2, binding = 1) uniform texture2D tex;

layout(set = 0, binding = 1) uniform  SceneData{
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
    outColor = vec4(texture(sampler2D(tex, samp), uv).xyz * (diffuse + sceneData.ambientColor.xyz), 1.f);
}