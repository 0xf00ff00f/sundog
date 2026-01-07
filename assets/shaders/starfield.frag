out vec4 fragColor;

in vec2 vs_texCoord;
in vec4 vs_color;

uniform vec4 color;

float superEllipse(vec2 p, float n)
{
    // |x/a|^n + |y/b|^n
    return pow(abs(p.x / .5), n) + pow(abs(p.y / .5), n);
}

float star(vec2 p)
{
    float d = length(p);
    float alpha = 0.01 / d;
    float rays = superEllipse(p, 0.5);
    rays = 1.0 - smoothstep(0.1, 0.55, rays);
    alpha += rays * 0.4;
    alpha *= 1.0 - smoothstep(0.01, 0.3, d);
    return alpha;
}

void main() {
    fragColor = star(0.3 * vs_texCoord) * vs_color;
}
