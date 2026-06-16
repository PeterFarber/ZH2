#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vTangent;
layout(location = 3) in vec2 vUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialUBO {
    vec4 baseColor;
    vec4 mrno;     // x metallic, y roughness, z normalStrength, w occlusionStrength
    vec4 emissive; // rgb color, a strength
    vec4 alpha;    // x mode (0 opaque, 1 mask, 2 blend), y cutoff
    vec4 uvXform;  // xy tiling (scale), zw offset
} uMaterial;

layout(set = 1, binding = 1) uniform sampler2D uBaseColorTex;
layout(set = 1, binding = 2) uniform sampler2D uNormalTex;
layout(set = 1, binding = 3) uniform sampler2D uMetalRoughTex;
layout(set = 1, binding = 4) uniform sampler2D uOcclusionTex;
layout(set = 1, binding = 5) uniform sampler2D uEmissiveTex;

vec3 ComputeNormal(vec2 uv) {
    vec3 n = normalize(vNormal);
    vec3 t = normalize(vTangent.xyz - dot(vTangent.xyz, n) * n);
    vec3 b = cross(n, t) * vTangent.w;
    vec3 tn = texture(uNormalTex, uv).xyz * 2.0 - 1.0;
    tn.xy *= uMaterial.mrno.z;
    return normalize(mat3(t, b, n) * tn);
}

void main() {
    // Apply per-material UV tiling (scale) + offset to all surface maps.
    vec2 uv = vUV * uMaterial.uvXform.xy + uMaterial.uvXform.zw;

    vec4 base = uMaterial.baseColor * texture(uBaseColorTex, uv);
    // Alpha mask (mode 1) clips texels below the cutoff. Blend (mode 2) must NOT
    // clip: it relies on the output alpha for framebuffer blending instead.
    if (uMaterial.alpha.x > 0.5 && uMaterial.alpha.x < 1.5 && base.a < uMaterial.alpha.y) {
        discard;
    }

    vec3 albedo = base.rgb;
    vec4 mr = texture(uMetalRoughTex, uv);
    float metallic = clamp(uMaterial.mrno.x * mr.b, 0.0, 1.0);
    float roughness = clamp(uMaterial.mrno.y * mr.g, 0.04, 1.0);
    float ao = mix(1.0, texture(uOcclusionTex, uv).r, uMaterial.mrno.w);

    vec3 N = ComputeNormal(uv);
    vec3 V = normalize(uCamera.position.xyz - vWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    int count = int(uScene.params.x);
    for (int i = 0; i < count && i < HK_MAX_LIGHTS; ++i) {
        GpuLight light = uScene.lights[i];
        vec3 L;
        float attenuation = 1.0;
        float shadow = 1.0;
        if (light.direction.w < 0.5) {
            L = normalize(-light.direction.xyz);
            shadow = SampleSunShadow(vWorldPos, max(dot(N, L), 0.0));
        } else {
            vec3 toLight = light.positionRange.xyz - vWorldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4);
            float range = max(light.positionRange.w, 1e-3);
            attenuation = clamp(1.0 - (dist * dist) / (range * range), 0.0, 1.0);
            attenuation *= attenuation;
            // Spot lights (type 2, direction.w > 1.5) get a smooth cone falloff
            // between their inner and outer cone angles (spot.x/spot.y hold the
            // cosines). Point lights (type 1) keep the full distance falloff.
            if (light.direction.w > 1.5) {
                vec3 spotDir = normalize(light.direction.xyz);
                float cosAngle = dot(-L, spotDir);
                float coneRange = max(light.spot.x - light.spot.y, 1e-4);
                float cone = clamp((cosAngle - light.spot.y) / coneRange, 0.0, 1.0);
                attenuation *= cone * cone;
            }
            shadow = SampleLocalShadow(light, vWorldPos, max(dot(N, L), 0.0));
        }

        vec3 H = normalize(V + L);
        vec3 radiance = light.color.rgb * light.color.a * attenuation * shadow;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
        vec3 specular = numerator / denom;

        vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kd * albedo / PI + specular) * radiance * NdotL;
    }

    // Screen-space AO modulates only the ambient term.
    vec2 screenUV = gl_FragCoord.xy / max(uCamera.clip.zw, vec2(1.0));
    float screenAO = texture(uAO, screenUV).r;
    vec3 ambient = uScene.ambient.rgb * uScene.ambient.a * albedo * ao * screenAO;
    vec3 emissive = uMaterial.emissive.rgb * uMaterial.emissive.a * texture(uEmissiveTex, uv).rgb;
    outColor = vec4(ambient + Lo + emissive, base.a);
}
