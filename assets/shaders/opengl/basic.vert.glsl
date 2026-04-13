#version 330 core

uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform int uIterations;

in int a_position_x;
in int a_position_y;
in int a_position_z;
in uint a_data;

out vec3 vPosition;
out vec3 vNormal;
out float vAo;

const float AO_VALUES[4] = float[4](0.1, 0.25, 0.5, 1.0);

void main() {
    vec3 p = vec3(a_position_x, a_position_y, a_position_z);
    vec3 pos = p / pow(3, uIterations);

    int normal_x = int((a_data >> 6) & uint(0x3)) - 1;
    int normal_y = int((a_data >> 4) & uint(0x3)) - 1;
    int normal_z = int((a_data >> 2) & uint(0x3)) - 1;
    uint ao_id = a_data & uint(0x3);

    vPosition = pos;
    vNormal = vec3(normal_x, normal_y, normal_z);
    vAo = AO_VALUES[ao_id];
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(pos, 1.0);
}