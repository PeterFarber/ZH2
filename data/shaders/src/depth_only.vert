#version 450

layout(location = 0) in vec3 aPos;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} uPush;

void main() {
    gl_Position = uPush.mvp * vec4(aPos, 1.0);
}
