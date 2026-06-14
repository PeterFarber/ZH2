#version 450

// 5x5 box blur of the raw SSAO buffer to remove the per-pixel sampling noise.

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uAO;

layout(push_constant) uniform PushConstants {
    vec4 texel; // xy = 1 / texture size
} uPush;

void main() {
    vec2 t = uPush.texel.xy;
    float sum = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            sum += texture(uAO, vUV + vec2(float(x), float(y)) * t).r;
        }
    }
    outColor = vec4(sum / 25.0);
}
