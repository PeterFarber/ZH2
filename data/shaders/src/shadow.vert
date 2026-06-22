#version 450

layout(location = 0) in vec3 aPos;
layout(location = 3) in vec2 aUV;

layout(location = 0) out vec2 vUV;

layout(push_constant) uniform PushConstants {
    mat4 lightMvp;
} uPush;

void main() {
    vUV = aUV;
    gl_Position = uPush.lightMvp * vec4(aPos, 1.0);
}
