layout(location=0) in vec2 position;
layout(location=1) in vec2 texCoord;

uniform mat4 mvp;

out vec2 vs_texCoord;

void main() {
    vs_texCoord = texCoord;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
}
