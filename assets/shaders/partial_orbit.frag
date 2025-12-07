out vec4 fragColor;

in float vs_angle;
in float vs_currentAngle;
in float vs_endAngle;

uniform vec4 color;

void main() {
    if (vs_angle > vs_endAngle)
        discard;
    if (vs_angle > vs_currentAngle)
    {
        const float dashInterval = 0.0625; // TODO make this a uniform
        if (mod(vs_angle, dashInterval) < 0.5 * dashInterval)
            discard;
    }
    fragColor = color;
}
