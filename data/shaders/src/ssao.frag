#version 450

// Screen-space ambient occlusion from the depth buffer. Matrix-free aside from
// the projection diagonal: it reconstructs view-space positions from linear
// depth, derives a surface normal from screen-space derivatives, and samples a
// depth-scaled hemisphere kernel. Sampling a real hemisphere (instead of a flat
// UV-space disc) keeps flat surfaces unoccluded, which avoids the moire/banding
// a fixed-radius depth comparison produces on receding planes. Output is a
// visibility factor (1 = unoccluded) consumed by the PBR pass to attenuate
// ambient light.

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uDepth;

layout(push_constant) uniform PushConstants {
    vec4 params;  // x near, y far, z world radius, w intensity
    vec4 params2; // x sample count, y normal bias, z proj[0][0], w proj[1][1]
} uPush;

const float TWO_PI = 6.28318530718;

float Linearize(float depth, float near, float far) {
    return near * far / (far - depth * (far - near));
}

// Decorrelated per-pixel hash for the kernel rotation. Unlike interleaved
// gradient noise (which is vertically correlated), this is white-ish noise so a
// box blur can fully remove the resulting variance instead of leaving stripes.
float Hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Reconstructs a view-space position (camera looks down -Z) from a UV and its
// linear depth, using the signed projection diagonal terms.
vec3 ViewPosition(vec2 uv, float linearZ, float proj00, float proj11) {
    vec2 ndc = uv * 2.0 - 1.0;
    return vec3(ndc.x * linearZ / proj00, ndc.y * linearZ / proj11, -linearZ);
}

void main() {
    float near = uPush.params.x;
    float far = uPush.params.y;
    float worldRadius = uPush.params.z;
    float intensity = uPush.params.w;
    int samples = int(uPush.params2.x);
    float bias = uPush.params2.y;
    float proj00 = uPush.params2.z;
    float proj11 = uPush.params2.w;

    float depth = texture(uDepth, vUV).r;
    if (depth >= 1.0) {
        outColor = vec4(1.0); // background: fully visible
        return;
    }

    float centerZ = Linearize(depth, near, far);
    vec3 centerPos = ViewPosition(vUV, centerZ, proj00, proj11);

    // Approximate the surface normal from the reconstructed position field. When
    // the depth field is locally flat/quantized the derivatives collapse and the
    // cross product is ill-defined; treat that as an unoccluded flat surface so
    // depth-precision banding never turns into AO stripes.
    vec3 faceNormal = cross(dFdx(centerPos), dFdy(centerPos));
    float faceLen = length(faceNormal);
    if (faceLen < 1e-6) {
        outColor = vec4(1.0);
        return;
    }
    vec3 normal = faceNormal / faceLen;
    vec3 toCamera = normalize(-centerPos);
    if (dot(normal, toCamera) < 0.0) {
        normal = -normal;
    }

    // World radius -> UV radius at this depth (perspective). 0.5 maps the NDC
    // half-extent to UV. Clamp so very near surfaces do not blow up the kernel.
    vec2 uvRadius = min(vec2(0.5 * abs(proj00), 0.5 * abs(proj11)) * worldRadius / centerZ, vec2(0.04));

    float angleBase = Hash(gl_FragCoord.xy) * TWO_PI;

    float occlusion = 0.0;
    for (int i = 0; i < samples; ++i) {
        float t = (float(i) + 0.5) / float(samples);
        float angle = angleBase + t * TWO_PI * 4.0;
        vec2 offset = vec2(cos(angle), sin(angle)) * uvRadius * sqrt(t);
        vec2 sampleUV = vUV + offset;

        float sampleDepth = texture(uDepth, sampleUV).r;
        if (sampleDepth >= 1.0) {
            continue;
        }
        float sampleZ = Linearize(sampleDepth, near, far);
        vec3 samplePos = ViewPosition(sampleUV, sampleZ, proj00, proj11);

        vec3 toSample = samplePos - centerPos;
        float dist = length(toSample);
        if (dist < 1e-4) {
            continue;
        }
        // How far the sample sits in front of the tangent plane (hemisphere AO).
        float nDotS = dot(normal, toSample / dist);
        // Smoothly ignore samples beyond the world radius (depth discontinuities).
        float rangeCheck = smoothstep(0.0, 1.0, worldRadius / dist);
        occlusion += max(nDotS - bias, 0.0) * rangeCheck;
    }

    float ao = 1.0 - (occlusion / float(samples)) * intensity;
    outColor = vec4(clamp(ao, 0.0, 1.0));
}
