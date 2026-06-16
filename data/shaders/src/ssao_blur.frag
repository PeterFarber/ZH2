#version 450

// 9x9 box blur of the raw SSAO buffer to remove the per-pixel sampling noise.
// A wider kernel smooths the hemisphere kernel's rotated-noise so contact AO
// reads as a soft gradient instead of hatching near edges.

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uAO;

layout(push_constant) uniform PushConstants {
    vec4 texel; // xy = 1 / texture size
} uPush;

void main() {
    vec2 t = uPush.texel.xy;
    float sum = 0.0;
    for (int x = -4; x <= 4; ++x) {
        for (int y = -4; y <= 4; ++y) {
            sum += texture(uAO, vUV + vec2(float(x), float(y)) * t).r;
        }
    }
    outColor = vec4(sum / 81.0);
}
