layout(location=0) in vec2 position;
layout(location=1) in vec2 texCoord;
layout(location=2) in vec4 color;

uniform mat4 mvp;

out vec2 vs_texCoord;
out vec4 vs_color;

void main() {
    vs_texCoord = texCoord;
    vs_color = color;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
}
