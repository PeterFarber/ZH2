#version 450

layout(location = 0) in vec3 aPos;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 color;
} uPush;

layout(location = 0) out vec4 vColor;

void main() {
    vColor = uPush.color;
    gl_Position = uPush.mvp * vec4(aPos, 1.0);
}
