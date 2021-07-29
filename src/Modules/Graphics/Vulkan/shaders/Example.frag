#version 450
#extension GL_EXT_nonuniform_qualifier : require

//Input color
layout (location = 0) in vec3 color;
layout (location = 1) in vec2 uv;

//output write
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform  SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;

layout(set = 2, binding = 0) uniform sampler samp;
layout(set = 2, binding = 1) uniform texture2D textures[];


layout(push_constant) uniform ObjectData
{
    int texIdx;

} ObjData;



void main()
{
    //vec3 color = texture(tex1[0],texCoord).xyz;
    //outFragColor = vec4(inColor + sceneData.ambientColor.xyz, 1.0f);
    outFragColor = texture(sampler2D(textures[ObjData.texIdx], samp), uv);
}