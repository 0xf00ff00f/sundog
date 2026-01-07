layout(location=0) in vec3 position;
layout(location=1) in vec2 offs;
layout(location=2) in vec4 color;

uniform mat4 mvp;
uniform float aspectRatio;

out vec4 vs_color;
out vec2 vs_texCoord;

void main() {
    vec4 centerClip = mvp * vec4(position, 1.0);

    vec2 offsetClip = offs;
    offsetClip.x /= aspectRatio;

    vs_color = color;
    vs_texCoord = normalize(offs);
    gl_Position = centerClip + vec4(offsetClip * centerClip.w, 0.0, 0.0);
}
