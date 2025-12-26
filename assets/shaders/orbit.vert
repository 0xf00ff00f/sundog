uniform mat4 mvp;
uniform float semiMajorAxis;
uniform float eccentricity;
uniform float aspectRatio;
uniform float thickness;
uniform float startAngle;
uniform float endAngle;
uniform float vertexCount;
uniform vec4 color;

out vec4 vs_color;

void main() {
    int vertexIndex = gl_VertexID / 2;
    float t = float(vertexIndex) / (float(vertexCount) - 1);
    float angle = startAngle + t * (endAngle - startAngle);
    float normalDirection = 2.0 * float(gl_VertexID % 2) - 1.0;

    float semiMinorAxis = semiMajorAxis * sqrt(1.0 - eccentricity * eccentricity);
    float focus = sqrt(semiMajorAxis * semiMajorAxis - semiMinorAxis * semiMinorAxis);

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

    vs_color = mix(0.2, 1.0, t) * color;
    gl_Position = currentClip + vec4(normalDirection * normalClip * currentClip.w, 0.0, 0.0);
}
