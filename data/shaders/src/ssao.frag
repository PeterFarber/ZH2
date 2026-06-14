#version 450

// Screen-space ambient occlusion from the depth buffer. Matrix-free: linearizes
// depth and accumulates occlusion from nearby samples that sit closer to the
// camera. Output is a visibility factor (1 = unoccluded) consumed by the PBR
// pass to attenuate ambient light.

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uDepth;

layout(push_constant) uniform PushConstants {
    vec4 params;  // x near, y far, z radius (uv), w intensity
    vec4 params2; // x sample count, y bias
} uPush;

float Linearize(float depth, float near, float far) {
    return near * far / (far - depth * (far - near));
}

float Hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    float near = uPush.params.x;
    float far = uPush.params.y;
    float depth = texture(uDepth, vUV).r;
    if (depth >= 1.0) {
        outColor = vec4(1.0);
        return;
    }

    float centerZ = Linearize(depth, near, far);
    float radius = uPush.params.z;
    int samples = int(uPush.params2.x);
    float bias = uPush.params2.y;
    float angle = Hash(vUV) * 6.2831853;

    float occlusion = 0.0;
    for (int i = 0; i < samples; ++i) {
        float t = (float(i) + 0.5) / float(samples);
        float rad = radius * t;
        float theta = angle + t * 6.2831853 * 5.0;
        vec2 offset = vec2(cos(theta), sin(theta)) * rad;

        float sampleDepth = texture(uDepth, vUV + offset).r;
        if (sampleDepth >= 1.0) {
            continue;
        }
        float sampleZ = Linearize(sampleDepth, near, far);
        float diff = centerZ - sampleZ; // > 0 => sample is an occluder in front
        float rangeCheck = 1.0 - smoothstep(0.0, 1.0, abs(diff) / (radius * centerZ + 1e-4));
        if (diff > bias) {
            occlusion += rangeCheck;
        }
    }

    float ao = 1.0 - (occlusion / float(samples)) * uPush.params.w;
    outColor = vec4(clamp(ao, 0.0, 1.0));
}
