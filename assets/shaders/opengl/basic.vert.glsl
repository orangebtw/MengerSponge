#version 330 core

uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

in vec3 a_position;
in uint a_ao_id;
in int a_normal_x;
in int a_normal_y;
in int a_normal_z;

out vec3 vPosition;
out vec3 vNormal;
out float vAo;

const float AO_VALUES[4] = float[4](0.1, 0.25, 0.5, 1.0);

void main() {
    vPosition = a_position;
    vNormal = vec3(a_normal_x, a_normal_y, a_normal_z);
    vAo = AO_VALUES[a_ao_id];
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(a_position, 1.0);
}