#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uHdr;

layout(push_constant) uniform PushConstants {
    vec4 params; // x exposure, y mode (0 none, 1 reinhard, 2 aces), z gamma
} uPush;

vec3 Reinhard(vec3 c) {
    return c / (c + vec3(1.0));
}

vec3 ACESFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(uHdr, vUV).rgb * uPush.params.x;
    int mode = int(uPush.params.y + 0.5);
    vec3 mapped;
    if (mode == 1) {
        mapped = Reinhard(hdr);
    } else if (mode == 2) {
        mapped = ACESFilm(hdr);
    } else {
        mapped = hdr;
    }
    float gamma = max(uPush.params.z, 1e-3);
    mapped = pow(mapped, vec3(1.0 / gamma));
    outColor = vec4(mapped, 1.0);
}
