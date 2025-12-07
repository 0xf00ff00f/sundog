out vec4 fragColor;

in float vs_angle;
in float vs_endAngle;

uniform vec4 color;

void main() {
    if (vs_angle > vs_endAngle)
        discard;
    fragColor = color;
}
