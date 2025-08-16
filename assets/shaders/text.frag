uniform sampler2D spriteSheetTexture;

in vec2 vs_texCoord;

out vec4 fragColor;

void main() {
    fragColor = texture(spriteSheetTexture, vs_texCoord);
}
