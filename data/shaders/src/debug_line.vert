#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
} uPush;

layout(location = 0) out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uPush.viewProj * vec4(aPos, 1.0);
}
