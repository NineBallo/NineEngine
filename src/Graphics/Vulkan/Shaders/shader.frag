#version 450
#extension GL_ARB_separate_shader_objects : enable



//import fragColor from the vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 1) uniform sampler2D texSampler;


//output a color represented as a vec4 (r,g,b,a) to framebuffer 0
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}