#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uSource;

layout(push_constant) uniform PushConstants {
    vec4 texel; // xy = 1 / source size, z = brightness threshold
} uPush;

void main() {
    vec2 t = uPush.texel.xy;
    vec3 sum = texture(uSource, vUV).rgb * 4.0;
    sum += texture(uSource, vUV + vec2(t.x, t.y)).rgb;
    sum += texture(uSource, vUV + vec2(-t.x, t.y)).rgb;
    sum += texture(uSource, vUV + vec2(t.x, -t.y)).rgb;
    sum += texture(uSource, vUV + vec2(-t.x, -t.y)).rgb;
    vec3 color = sum / 8.0;
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color *= max(luma - uPush.texel.z, 0.0) / max(luma, 1e-4);
    outColor = vec4(color, 1.0);
}
