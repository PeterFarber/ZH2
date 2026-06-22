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
    vec4 spot;          // x cosInner, y cosOuter, z shadow index/enabled (<0 => none)
};

#define HK_MAX_LIGHTS 16
#define HK_MAX_CASCADES 4
// Local (spot/point) shadows share one depth atlas laid out as a grid of
// perspective tiles. A spot uses 1 tile; a point uses 6 cube-face tiles.
#define HK_MAX_LOCAL_TILES 16

layout(set = 0, binding = 1) uniform SceneUBO {
    vec4 ambient;        // rgb ambient color, a intensity
    vec4 params;         // x light count, y cascade count, z 1/atlasRes, w pcf radius
    vec4 cascadeSplits;  // view-space far distance of each cascade
    vec4 cascadeTexelSizes; // world-space texel size for each directional cascade
    mat4 cascadeViewProj[HK_MAX_CASCADES];
    GpuLight lights[HK_MAX_LIGHTS];
    vec4 localShadowParams;                       // x 1/atlasRes, y pcf radius, z grid dim, w enabled
    vec4 cascadeShadowParams;      // x blend scale, y min blend
    vec4 directionalShadowParams;  // x normal offset scale, y min, z max
    vec4 directionalShadowBias;    // x base, y slope, z min, w max
    vec4 contactShadowParams;      // x distance, y strength, z normal offset scale, w normal offset min
    vec4 contactShadowBias;        // x base, y slope, z min, w max
    vec4 localShadowBias;          // x scale, y min, z max
    mat4 localShadowViewProj[HK_MAX_LOCAL_TILES]; // per-tile light view-proj
} uScene;

layout(set = 0, binding = 2) uniform sampler2DShadow uShadowMap;

// Screen-space ambient occlusion (1 = unoccluded). White when SSAO is disabled.
layout(set = 0, binding = 3) uniform sampler2D uAO;

// Local-light (spot/point) shadow atlas. White when local shadows are disabled.
layout(set = 0, binding = 4) uniform sampler2DShadow uLocalShadowMap;

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

float SampleSunShadowCascade(int cascade, int cascadeCount, vec3 shadowPos, float bias, out float valid) {
    valid = 0.0;
    vec4 lightClip = uScene.cascadeViewProj[cascade] * vec4(shadowPos, 1.0);
    vec3 proj = lightClip.xyz / lightClip.w;
    if (proj.z > 1.0 || proj.z < 0.0) {
        return 1.0;
    }
    vec2 uv = proj.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 1.0;
    }
    valid = 1.0;
    // Map cascade UV into the atlas. A single cascade owns the full atlas;
    // multiple cascades are packed into a 2x2 atlas.
    vec2 tile = vec2(float(cascade & 1), float(cascade >> 1));
    float tileScale = cascadeCount == 1 ? 1.0 : 0.5;
    vec2 atlasUV = cascadeCount == 1 ? uv : (uv + tile) * 0.5;

    float texel = uScene.params.z;
    int radius = int(uScene.params.w + 0.5);
    vec2 tileMin = cascadeCount == 1 ? vec2(texel) : tile * 0.5 + vec2(texel);
    vec2 tileMax = cascadeCount == 1 ? vec2(1.0 - texel) : (tile + vec2(1.0)) * tileScale - vec2(texel);

    float sum = 0.0;
    float taps = 0.0;
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel;
            vec2 sampleUV = clamp(atlasUV + offset, tileMin, tileMax);
            sum += texture(uShadowMap, vec3(sampleUV, proj.z - bias));
            taps += 1.0;
        }
    }
    return sum / max(taps, 1.0);
}

// Returns lit visibility (1 = lit) from the cascaded directional shadow map.
// Cascades are packed into a 2x2 atlas; the cascade is chosen by view depth.
float SampleSunShadow(vec3 worldPos, vec3 normal, float NdotL, bool contactShadows) {
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

    // Offset the receiver query by a fraction of the cascade's world texel size.
    // A fixed world-space offset makes near cascades visibly detach contact
    // shadows, while far cascades need a little more room to avoid acne.
    float texelWorld = max(uScene.cascadeTexelSizes[cascade], 0.001);
    float normalOffset = clamp(texelWorld * uScene.directionalShadowParams.x,
                               uScene.directionalShadowParams.y,
                               uScene.directionalShadowParams.z);
    vec3 unitNormal = normalize(normal);
    vec3 shadowPos = worldPos + unitNormal * normalOffset;
    float bias = clamp(uScene.directionalShadowBias.x +
                       uScene.directionalShadowBias.y * (1.0 - clamp(NdotL, 0.0, 1.0)),
                       uScene.directionalShadowBias.z,
                       uScene.directionalShadowBias.w);
    float currentValid = 0.0;
    float shadow = SampleSunShadowCascade(cascade, cascadeCount, shadowPos, bias, currentValid);

    // Optional contact shadowing re-samples close receivers with a smaller
    // normal offset/bias. This keeps the broad stable cascade sample while
    // tightening object-to-ground contact when the quality setting enables it.
    if (contactShadows && viewDepth < uScene.contactShadowParams.x) {
        vec3 contactPos = worldPos + unitNormal * clamp(texelWorld * uScene.contactShadowParams.z,
                                                        uScene.contactShadowParams.w,
                                                        normalOffset);
        float contactBias = clamp(uScene.contactShadowBias.x +
                                  uScene.contactShadowBias.y * (1.0 - clamp(NdotL, 0.0, 1.0)),
                                  uScene.contactShadowBias.z,
                                  uScene.contactShadowBias.w);
        float contactValid = 0.0;
        float contact = SampleSunShadowCascade(cascade, cascadeCount, contactPos, contactBias, contactValid);
        if (contactValid > 0.5) {
            shadow = min(shadow, mix(1.0, contact, uScene.contactShadowParams.y));
        }
    }

    // Blend into the next cascade near a split. The renderer overlaps cascade
    // fit regions, so both maps should contain nearby casters and this removes
    // hard rectangular split edges on broad receivers like the rink ice.
    if (cascade + 1 < cascadeCount) {
        float split = uScene.cascadeSplits[cascade];
        float prevSplit = cascade == 0 ? uCamera.clip.x : uScene.cascadeSplits[cascade - 1];
        float blendRange = max((split - prevSplit) * uScene.cascadeShadowParams.x,
                               uScene.cascadeShadowParams.y);
        float blend = clamp((viewDepth - (split - blendRange)) / blendRange, 0.0, 1.0);
        if (blend > 0.0) {
            float nextValid = 0.0;
            float nextShadow = SampleSunShadowCascade(cascade + 1, cascadeCount, shadowPos, bias, nextValid);
            if (nextValid > 0.5) {
                shadow = mix(shadow, nextShadow, blend);
            }
        }
    }

    return shadow;
}

// Returns lit visibility (1 = lit) for a spot/point light from the shared local
// shadow atlas. Spot lights (type 2) read their single tile; point lights
// (type 1) select one of six cube-face tiles by the light->fragment direction.
float SampleLocalShadow(GpuLight light, vec3 worldPos, float NdotL, bool contactShadows) {
    if (uScene.localShadowParams.w < 0.5 || light.spot.z < -0.5) {
        return 1.0;
    }
    int baseTile = int(light.spot.z + 0.5);
    int tile = baseTile;
    // Point light (type 1, direction.w in [0.5, 1.5]) picks the dominant axis.
    if (light.direction.w < 1.5) {
        vec3 d = worldPos - light.positionRange.xyz;
        vec3 ad = abs(d);
        if (ad.x >= ad.y && ad.x >= ad.z) {
            tile = baseTile + (d.x > 0.0 ? 0 : 1);
        } else if (ad.y >= ad.z) {
            tile = baseTile + (d.y > 0.0 ? 2 : 3);
        } else {
            tile = baseTile + (d.z > 0.0 ? 4 : 5);
        }
    }

    vec4 clip = uScene.localShadowViewProj[tile] * vec4(worldPos, 1.0);
    if (clip.w <= 0.0) {
        return 1.0;
    }
    vec3 proj = clip.xyz / clip.w;
    if (proj.z > 1.0 || proj.z < 0.0) {
        return 1.0;
    }
    vec2 uv = proj.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 1.0;
    }

    int grid = int(uScene.localShadowParams.z + 0.5);
    float invGrid = 1.0 / float(max(grid, 1));
    vec2 tileCoord = vec2(float(tile % grid), float(tile / grid));
    vec2 tileMin = tileCoord * invGrid;
    vec2 atlasUV = tileMin + uv * invGrid;

    float bias = clamp(uScene.localShadowBias.x * tan(acos(clamp(NdotL, 0.0, 1.0))),
                       uScene.localShadowBias.y,
                       uScene.localShadowBias.z);
    float texel = uScene.localShadowParams.x;
    int radius = int(uScene.localShadowParams.y + 0.5);
    // Keep PCF taps inside this tile so they cannot bleed into a neighbour.
    vec2 clampMin = tileMin + vec2(texel);
    vec2 clampMax = tileMin + vec2(invGrid) - vec2(texel);

    float sum = 0.0;
    float taps = 0.0;
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            vec2 s = clamp(atlasUV + vec2(float(x), float(y)) * texel, clampMin, clampMax);
            sum += texture(uLocalShadowMap, vec3(s, proj.z - bias));
            taps += 1.0;
        }
    }
    return sum / max(taps, 1.0);
}

#endif
