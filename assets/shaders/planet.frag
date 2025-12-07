uniform sampler2D diffuseTexture;

uniform mat4 viewMatrix;

uniform vec3 lightPosition;
uniform vec3 lightIntensity;
uniform vec3 ka;
uniform vec3 ks;
uniform float shininess;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texCoord;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vs_normal);
    vec3 s = normalize(vec3(viewMatrix * vec4(lightPosition, 1.0)) - vs_position);
    vec3 v = normalize(-vs_position);
    vec3 r = reflect(-s, n);
    float diffuse = max(dot(s, n), 0.0);
    float specular = 0.0;
    if (dot(s, n) > 0.0)
        specular = pow(max(dot(r, v), 0.0), shininess);
    vec3 diffuseColor = texture(diffuseTexture, vs_texCoord).rgb;
    vec3 color = lightIntensity * ((ka + diffuse) * diffuseColor + specular * ks);
    float gamma = 2.2;
    fragColor = vec4(pow(color, vec3(1.0/gamma)), 1.0);
}
