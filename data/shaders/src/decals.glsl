// Projected decals for the forward PBR path.
// flags.x = affectsBaseColor
// flags.y = affectsNormals
// flags.z = alpha mode: 0 opaque, 1 mask, 2 blend
// flags.w = alpha cutoff

struct DecalGPU {
    mat4 worldToDecal;
    mat4 decalToWorld;
    vec4 baseColor;
    vec4 uvXform;
    vec4 flags;
    vec4 params;
};

layout(set = 2, binding = 0) uniform DecalUBO {
    vec4 params;
    DecalGPU decals[32];
} uDecals;

layout(set = 2, binding = 1) uniform sampler2D uDecalBaseColorTex[32];
layout(set = 2, binding = 2) uniform sampler2D uDecalNormalTex[32];

vec3 DecodeDecalNormal(DecalGPU decal, vec2 uv, int index, vec3 surfaceNormal) {
    vec3 tangentNormal = texture(uDecalNormalTex[index], uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= decal.params.x;

    vec3 tangent = normalize(decal.decalToWorld[0].xyz);
    vec3 bitangent = normalize(decal.decalToWorld[2].xyz);
    vec3 projectorNormal = normalize(decal.decalToWorld[1].xyz);
    mat3 decalBasis = mat3(tangent, bitangent, projectorNormal);

    vec3 worldNormal = normalize(decalBasis * tangentNormal);
    if (dot(worldNormal, surfaceNormal) < 0.0) {
        worldNormal = -worldNormal;
    }
    return worldNormal;
}

void ApplyDecals(inout vec3 albedo, inout vec3 normal, vec3 worldPos) {
    int count = min(int(uDecals.params.x + 0.5), 32);
    for (int i = 0; i < count; ++i) {
        DecalGPU decal = uDecals.decals[i];
        vec3 local = (decal.worldToDecal * vec4(worldPos, 1.0)).xyz;
        if (any(greaterThan(abs(local), vec3(0.5)))) {
            continue;
        }

        vec2 uv = local.xz + vec2(0.5);
        uv = uv * decal.uvXform.xy + decal.uvXform.zw;

        vec4 decalBase = decal.baseColor * texture(uDecalBaseColorTex[i], uv);
        if (decal.flags.z > 0.5 && decal.flags.z < 1.5 && decalBase.a < decal.flags.w) {
            continue;
        }

        float alpha = clamp(decalBase.a, 0.0, 1.0);
        if (decal.flags.x > 0.5) {
            albedo = mix(albedo, decalBase.rgb, alpha);
        }
        if (decal.flags.y > 0.5) {
            vec3 decalNormal = DecodeDecalNormal(decal, uv, i, normal);
            normal = normalize(mix(normal, decalNormal, alpha));
        }
    }
}
