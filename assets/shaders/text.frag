uniform sampler2D spriteSheetTexture;

in vec2 vs_texCoord;
in vec4 vs_color;

out vec4 fragColor;

void main() {
    fragColor = vs_color * texture(spriteSheetTexture, vs_texCoord);
}
