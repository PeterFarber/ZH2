#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uSource;

layout(push_constant) uniform PushConstants {
    vec4 params; // x = filter radius (uv), y = additive weight (alpha)
} uPush;

void main() {
    float r = uPush.params.x;
    vec3 sum = vec3(0.0);
    sum += texture(uSource, vUV + vec2(-r, -r)).rgb;
    sum += texture(uSource, vUV + vec2(0.0, -r)).rgb * 2.0;
    sum += texture(uSource, vUV + vec2(r, -r)).rgb;
    sum += texture(uSource, vUV + vec2(-r, 0.0)).rgb * 2.0;
    sum += texture(uSource, vUV).rgb * 4.0;
    sum += texture(uSource, vUV + vec2(r, 0.0)).rgb * 2.0;
    sum += texture(uSource, vUV + vec2(-r, r)).rgb;
    sum += texture(uSource, vUV + vec2(0.0, r)).rgb * 2.0;
    sum += texture(uSource, vUV + vec2(r, r)).rgb;
    // Additive blend uses src alpha as the weight, so this scales the contribution.
    outColor = vec4(sum / 16.0, uPush.params.y);
}
