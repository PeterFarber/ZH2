#version 450

// Final present pass. Input is the tonemapped LDR image in display (gamma)
// space. When enabled, applies FXAA in perceptual space; always converts the
// result back to linear so the sRGB swapchain re-encodes to the same display
// values (no double gamma).

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uImage;

layout(push_constant) uniform PushConstants {
    vec4 params; // xy = 1 / texture size, z = enabled (0/1)
} uPush;

float Luma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

vec3 ToLinear(vec3 c) {
    return pow(max(c, vec3(0.0)), vec3(2.2));
}

void main() {
    vec2 texel = uPush.params.xy;
    vec3 rgbM = texture(uImage, vUV).rgb;

    if (uPush.params.z < 0.5) {
        outColor = vec4(ToLinear(rgbM), 1.0);
        return;
    }

    vec3 rgbNW = texture(uImage, vUV + vec2(-texel.x, -texel.y)).rgb;
    vec3 rgbNE = texture(uImage, vUV + vec2(texel.x, -texel.y)).rgb;
    vec3 rgbSW = texture(uImage, vUV + vec2(-texel.x, texel.y)).rgb;
    vec3 rgbSE = texture(uImage, vUV + vec2(texel.x, texel.y)).rgb;

    float lNW = Luma(rgbNW);
    float lNE = Luma(rgbNE);
    float lSW = Luma(rgbSW);
    float lSE = Luma(rgbSE);
    float lM = Luma(rgbM);
    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));

    vec2 dir;
    dir.x = -((lNW + lNE) - (lSW + lSE));
    dir.y = ((lNW + lSW) - (lNE + lSE));

    float dirReduce = max((lNW + lNE + lSW + lSE) * 0.25 * 0.0625, 1.0 / 128.0);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * texel;

    vec3 rgbA = 0.5 * (texture(uImage, vUV + dir * (1.0 / 3.0 - 0.5)).rgb +
                       texture(uImage, vUV + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(uImage, vUV + dir * -0.5).rgb + texture(uImage, vUV + dir * 0.5).rgb);

    float lB = Luma(rgbB);
    vec3 result = (lB < lMin || lB > lMax) ? rgbA : rgbB;
    outColor = vec4(ToLinear(result), 1.0);
}
