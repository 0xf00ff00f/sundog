layout(location=0) in float angleOffset;
layout(location=1) in float normalDirection;

uniform mat4 mvp;
uniform float semiMajorAxis;
uniform float eccentricity;
uniform float aspectRatio;
uniform float thickness;
uniform float startAngle;
uniform float endAngle;

out float vs_angle;
out float vs_endAngle;

void main() {
    float semiMinorAxis = semiMajorAxis * sqrt(1.0 - eccentricity * eccentricity);
    float focus = sqrt(semiMajorAxis * semiMajorAxis - semiMinorAxis * semiMinorAxis);

    float angle = startAngle + angleOffset;

    // point on the curve
    vec2 current = vec2(semiMajorAxis * cos(angle) - focus, semiMinorAxis * sin(angle));

    // direction (tangent vector) of the curve
    vec2 direction = vec2(-semiMajorAxis * sin(angle), semiMinorAxis * cos(angle));

    vec4 currentClip = mvp * vec4(current, 0.0, 1.0);
    vec2 currentScreen = currentClip.xy / currentClip.w;
    currentScreen.x *= aspectRatio;

    // TODO: project direction directly?
    vec4 nextClip = mvp * vec4(current + direction, 0.0, 1.0);
    vec2 nextScreen = nextClip.xy / nextClip.w;
    nextScreen.x *= aspectRatio;

    vec2 directionScreen = normalize(nextScreen - currentScreen);
    vec2 normalScreen = vec2(-directionScreen.y, directionScreen.x);
    normalScreen *= thickness;

    vec2 normalClip = normalScreen;
    normalClip.x /= aspectRatio;

    vs_angle = angle;
    vs_endAngle = endAngle;
    gl_Position = currentClip + vec4(normalDirection * normalClip * currentClip.w, 0.0, 0.0);
}
