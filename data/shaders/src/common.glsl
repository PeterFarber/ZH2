#ifndef HOCKEY_COMMON_GLSL
#define HOCKEY_COMMON_GLSL

// Shared declarations for the mesh/PBR shaders. Include-only; never compiled
// on its own. Descriptor layout convention used across the renderer:
//   set 0 = global (camera, scene/lights, shadow map)
//   set 1 = material (material UBO + PBR textures)
//   set 2 = per-pass inputs (post-process)

const float PI = 3.14159265359;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 position; // xyz = world-space camera position
    vec4 clip;     // x = near, y = far, z = render width, w = render height
} uCamera;

struct GpuLight {
    vec4 positionRange; // xyz position, w range (0 => directional)
    vec4 direction;     // xyz direction, w type (0 dir, 1 point, 2 spot)
    vec4 color;         // rgb color, a intensity
    vec4 spot;          // x cosInner, y cosOuter
};

#define HK_MAX_LIGHTS 16
#define HK_MAX_CASCADES 4

layout(set = 0, binding = 1) uniform SceneUBO {
    vec4 ambient;        // rgb ambient color, a intensity
    vec4 params;         // x light count, y cascade count, z 1/atlasRes, w pcf radius
    vec4 cascadeSplits;  // view-space far distance of each cascade
    mat4 cascadeViewProj[HK_MAX_CASCADES];
    GpuLight lights[HK_MAX_LIGHTS];
} uScene;

layout(set = 0, binding = 2) uniform sampler2DShadow uShadowMap;

// Screen-space ambient occlusion (1 = unoccluded). White when SSAO is disabled.
layout(set = 0, binding = 3) uniform sampler2D uAO;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-5);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Returns lit visibility (1 = lit) from the cascaded directional shadow map.
// Cascades are packed into a 2x2 atlas; the cascade is chosen by view depth.
float SampleSunShadow(vec3 worldPos, float NdotL) {
    int cascadeCount = int(uScene.params.y + 0.5);
    if (cascadeCount <= 0) {
        return 1.0;
    }

    float viewDepth = -(uCamera.view * vec4(worldPos, 1.0)).z;
    int cascade = cascadeCount - 1;
    for (int i = 0; i < cascadeCount; ++i) {
        if (viewDepth < uScene.cascadeSplits[i]) {
            cascade = i;
            break;
        }
    }

    vec4 lightClip = uScene.cascadeViewProj[cascade] * vec4(worldPos, 1.0);
    vec3 proj = lightClip.xyz / lightClip.w;
    if (proj.z > 1.0 || proj.z < 0.0) {
        return 1.0;
    }
    vec2 uv = proj.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 1.0;
    }
    // Map cascade UV into its half-atlas tile (col = cascade&1, row = cascade>>1).
    vec2 tile = vec2(float(cascade & 1), float(cascade >> 1));
    vec2 atlasUV = (uv + tile) * 0.5;

    float bias = clamp(0.0015 * tan(acos(clamp(NdotL, 0.0, 1.0))), 0.0, 0.005);
    float texel = uScene.params.z;
    int radius = int(uScene.params.w + 0.5);

    float sum = 0.0;
    float taps = 0.0;
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel;
            sum += texture(uShadowMap, vec3(atlasUV + offset, proj.z - bias));
            taps += 1.0;
        }
    }
    return sum / max(taps, 1.0);
}

#endif
