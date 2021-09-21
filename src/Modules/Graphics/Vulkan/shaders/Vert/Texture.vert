#version 460
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;


layout (location = 0) out vec2 texCoord;

//Camera data
layout(set = 0, binding = 0) uniform CameraBuffer{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} cameraData;


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
    gl_Position = transformMatrix * vec4(vPosition, 1.0);
    texCoord = vTexCoord;
}