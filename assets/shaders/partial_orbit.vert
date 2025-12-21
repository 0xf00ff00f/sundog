layout(location=0) in float angleOffset;
layout(location=1) in float normalDirection;

uniform mat4 mvp;
uniform float semiMajorAxis;
uniform float eccentricity;
uniform float aspectRatio;
uniform float thickness;
uniform float startAngle;
uniform float endAngle;
uniform float currentAngle;

out float vs_angle;
out float vs_currentAngle;
out float vs_endAngle;

vec2 positionAt(float angle)
{
    // conic section with focus at origin
    // works fine for hyperbolae because semiMajorAxis is negative!
    float l = semiMajorAxis * (1.0 - eccentricity * eccentricity); // semi-latus rectum
    float r = l / (1.0 + eccentricity * cos(angle));
    return vec2(r * cos(angle), r * sin(angle));
}

void main() {
    float angle = startAngle + angleOffset;

    // point on the curve
    vec2 current = positionAt(angle);

    vec2 next = positionAt(angle + radians(0.1));

    vec4 currentClip = mvp * vec4(current, 0.0, 1.0);
    vec2 currentScreen = currentClip.xy / currentClip.w;
    currentScreen.x *= aspectRatio;

    // TODO: project direction directly?
    vec4 nextClip = mvp * vec4(next, 0.0, 1.0);
    vec2 nextScreen = nextClip.xy / nextClip.w;
    nextScreen.x *= aspectRatio;

    vec2 directionScreen = normalize(nextScreen - currentScreen);
    vec2 normalScreen = vec2(-directionScreen.y, directionScreen.x);
    normalScreen *= thickness;

    vec2 normalClip = normalScreen;
    normalClip.x /= aspectRatio;

    vs_angle = angle;
    vs_currentAngle = currentAngle;
    vs_endAngle = endAngle;
    gl_Position = currentClip + vec4(normalDirection * normalClip * currentClip.w, 0.0, 0.0);
}
