#version 460
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;


layout (location = 0) out vec2 texCoord;
layout (location = 1) out vec3 diffuse;


//Camera data
layout(set = 0, binding = 0) uniform CameraBuffer{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} cameraData;

layout(set = 0, binding = 1) uniform  SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;

//All object matrices\n"
struct ObjectData{
    mat4 model;
};


layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{
    ObjectData objects[];
} objectBuffer;


layout(push_constant) uniform VertexData
{
    int texIdx;
    int entityID;

} vertexData;


void main() {

    mat4 modelMatrix = objectBuffer.objects[vertexData.entityID].model;
    mat4 transformMatrix = (cameraData.viewproj * modelMatrix);

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    vec3 normalWorldSpace = normalize(normalMatrix * vNormal);

    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(sceneData.sunlightDirection.xyz - vec3(modelMatrix * vec4(vPosition, 1.0f)));

    float diff = max(dot(normalWorldSpace, lightDir), 0.0);
    diffuse = diff * sceneData.sunlightColor.xyz * sceneData.sunlightDirection.w;



    gl_Position = transformMatrix * vec4(vPosition, 1.0);
    texCoord = vTexCoord;
}