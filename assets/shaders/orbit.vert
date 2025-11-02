layout(location=0) in float meanAnomaly;

uniform mat4 mvp;
uniform float semiMajorAxis;
uniform float eccentricity;

void main() {
    float semiMinorAxis = semiMajorAxis * sqrt(1.0 - eccentricity * eccentricity);
    float focus = sqrt(semiMajorAxis * semiMajorAxis - semiMinorAxis * semiMinorAxis);

    float x = semiMajorAxis * cos(meanAnomaly) - focus;
    float y = semiMinorAxis * sin(meanAnomaly);

    gl_Position = mvp * vec4(x, y, 0.0, 1.0);
}
