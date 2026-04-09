#version 330 core

in vec2 v_uv;

out vec4 fragColor;

uniform mat4 uCameraToWorld;
uniform mat4 uCameraInverseProjection;
uniform int uIterations;

#define MAX_STEPS 300
#define MAX_DIST 1000.0
#define EPSILON 0.00001

#define AO_SAMPLES 10.0
#define AO_FACTOR 1.0

float sphere(vec3 p, float r) {
    return length(p) - r;
}

float box(vec3 p, float size) {
    p = abs(p) - size;
    return max(p.x, max(p.y, p.z));
}

float getCross(vec3 p, float size) {
    p = abs(p) - size / 3.0;
    float bx = max(p.y, p.z);
    float by = max(p.x, p.z);
    float bz = max(p.x, p.y);
    return min(min(bx, by), bz);
}

float getInnerMenger(vec3 p, float size) {
    float d = EPSILON;
    float scale = 1.0;
    
    for (int i = 0; i < uIterations; ++i) {
        float r = size / scale;
        vec3 q = mod(p + r, 2.0 * r) - r;
        d = min(d, getCross(q, r));
        scale *= 3.0;
    }
    return d;
}

vec4 map(vec3 p) {
    float d = max(box(p, 0.5), -getInnerMenger(p, 0.5));
    return vec4(vec3(1.0, 1.0, 1.0), d * 0.9);
}

float getAO(vec3 pos, vec3 norm) {
    float result = 1.0;
    float s = -AO_SAMPLES;
    float unit = 1.0 / AO_SAMPLES;
    for (float i = unit; i < 1.0; i += unit) {
        result -= pow(1.6, i * s) * (i - map(pos + i * norm).w);
    }
    return result * AO_FACTOR;
}

vec3 getNormal(vec3 p) {
    vec2 e = vec2(EPSILON, 0.0);
    vec3 n = map(p).w - vec3(map(p - e.xyy).w, map(p - e.yxy).w, map(p - e.yyx).w);
    return normalize(n);
}

vec4 rayMarch(vec3 ro, vec3 rd, int steps) {
    float progress = 0.0;
    vec3 color = vec3(0.0);

    for (int i = 0; i < steps; ++i) {
        vec3 samplePoint = ro + rd * progress;
        vec4 res = map(samplePoint);
        if (res.w < EPSILON) {
            color = res.rgb;
            break;
        }
        if (progress > MAX_DIST)
            break;
        progress += res.w;
    }

    return vec4(color, progress);
}

void main() {
    vec3 light = vec3(0.0, 1000.0, 0.0);

    vec3 col = vec3(0.0);
    vec2 uv = v_uv * 2.0 - 1.0;

    vec3 origin = (uCameraToWorld * vec4(0, 0, 0, 1)).xyz;
    vec3 dir = (uCameraInverseProjection * vec4(uv, 0, 1)).xyz;
    dir = (uCameraToWorld * vec4(dir, 0)).xyz;
    dir = normalize(dir);
    
    vec4 res = rayMarch(origin, dir, MAX_STEPS);
    if (res.w < MAX_DIST) {
        vec3 p = origin + dir * res.w;
        vec3 normal = getNormal(p);
        float diff = 0.7 * max(0.0, dot(normal, -dir));
        float ao = getAO(p, normal);
        col += diff * ao * res.rgb;
    }

    fragColor = vec4(sqrt(col), 1.0);
}