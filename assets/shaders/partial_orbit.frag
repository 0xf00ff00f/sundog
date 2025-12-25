out vec4 fragColor;

uniform float startAngle;
uniform float endAngle;

in float vs_angle;
in float vs_currentAngle;

uniform vec4 color;

void main() {
    if (vs_angle > endAngle)
        discard;
    float alpha;
    if (vs_angle > vs_currentAngle)
    {
        const float dashInterval = 0.0625; // TODO make this a uniform
        if (mod(vs_angle, dashInterval) < 0.5 * dashInterval)
            discard;
        alpha = 0.5;
    }
    else
    {
        float interval = endAngle - startAngle;
        alpha = mix(0.2, 1.0, (vs_angle - (vs_currentAngle - interval)) / interval);
    }
    fragColor = alpha * color;
}
