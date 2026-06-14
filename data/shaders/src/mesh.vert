#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aTangent;
layout(location = 3) in vec2 aUV;

layout(push_constant) uniform PushConstants {
    mat4 model;
} uPush;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec4 vTangent;
layout(location = 3) out vec2 vUV;

void main() {
    vec4 worldPos = uPush.model * vec4(aPos, 1.0);
    mat3 normalMat = mat3(uPush.model);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(normalMat * aNormal);
    vTangent = vec4(normalize(normalMat * aTangent.xyz), aTangent.w);
    vUV = aUV;
    gl_Position = uCamera.viewProj * worldPos;
}
