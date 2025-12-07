layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoord;

uniform mat4 modelViewMatrix;
uniform mat3 modelViewNormalMatrix;
uniform mat4 mvp;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texCoord;

void main() {
    vs_texCoord = texCoord;
    vs_normal = normalize(modelViewNormalMatrix * normal);
    vs_position = vec3(modelViewMatrix * vec4(position, 1.0));
    gl_Position = mvp * vec4(position, 1.0);
}
