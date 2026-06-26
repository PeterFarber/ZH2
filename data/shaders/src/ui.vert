#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

layout(push_constant) uniform PushConstants {
    mat4 transform;
    vec4 viewportAndTranslation; // x/y target size, z/w RmlUi translation.
} uPush;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 vColor;

void main() {
    vec4 pixel = vec4(aPos + uPush.viewportAndTranslation.zw, 0.0, 1.0);
    pixel = uPush.transform * pixel;
    vec2 position = pixel.xy;
    if (abs(pixel.w) > 0.00001) {
        position /= pixel.w;
    }

    vec2 targetSize = max(uPush.viewportAndTranslation.xy, vec2(1.0));
    vec2 clip = vec2((position.x / targetSize.x) * 2.0 - 1.0,
                     (position.y / targetSize.y) * 2.0 - 1.0);
    gl_Position = vec4(clip, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
