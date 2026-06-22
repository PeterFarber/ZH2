#version 450

layout(location = 0) in vec2 vUV;

layout(set = 0, binding = 0) uniform MaterialUBO {
    vec4 baseColor;
    vec4 mrno;
    vec4 emissive;
    vec4 alpha;   // x mode (0 opaque, 1 mask, 2 blend), y cutoff
    vec4 uvXform;
} uMaterial;

layout(set = 0, binding = 1) uniform sampler2D uBaseColorTex;

// Depth-only shadow pass: alpha-masked casters discard holes before depth write.
void main() {
    vec2 uv = vUV * uMaterial.uvXform.xy + uMaterial.uvXform.zw;
    float opacity = uMaterial.baseColor.a * texture(uBaseColorTex, uv).a;
    if (uMaterial.alpha.x > 0.5 && uMaterial.alpha.x < 1.5 && opacity < uMaterial.alpha.y) {
        discard;
    }
}
