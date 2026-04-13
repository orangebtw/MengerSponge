#version 330 core

out vec4 fragColor;

in vec3 vPosition;
in vec3 vNormal;
in float vAo;

const vec3 LIGHT_POS = vec3(3.0, 3.0, 3.0);

void main() {
    float ambient = 0.2;

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(LIGHT_POS - vPosition);

    float diffuse = max(dot(normal, lightDir), 0.0);

    vec3 color = vec3(1.0, 1.0, 1.0) * (diffuse + ambient) * vAo;
    color = pow(color, vec3(0.454545));

    fragColor = vec4(color, 1.0);
}