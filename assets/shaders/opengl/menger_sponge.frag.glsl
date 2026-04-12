#version 330 core

in vec2 v_uv;

out vec4 fragColor;

uniform mat4 uCameraToWorld;
uniform mat4 uCameraInverseProjection;
uniform int uIterations;

#define MAX_STEPS 300
#define MAX_DIST 100.0
#define EPSILON 1e-5
#define SURF_DIST 0.001
#define SHADOW_BIAS EPSILON * 50;

#define AO_SAMPLES 10.0
#define AO_FACTOR 1.0

const vec3 LIGHT_POS = vec3(3.0, 3.0, 0.0);

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

vec4 Blend(float a, float b, vec3 colA, vec3 colB, float k) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    float blendDst = mix( b, a, h ) - k*h*(1.0-h);
    vec3 blendCol = mix(colB,colA,h);
    return vec4(blendCol, blendDst);
}

vec4 map(vec3 p) {
    float size = 0.5;
    float d1 = max(box(p, 0.5), -getInnerMenger(p, 0.5));
    float d2 = sphere(p - LIGHT_POS + vec3(0.25), 0.1);
    return Blend(d1, d2, vec3(1.0), vec3(1.0, 1.0, 0.0), 1.0);

    // float d = 0.0;
    // float cr = getCross(p, size);
    // d = max(d, cr);

    // float scale = 1.0;
    // for (int i = 0; i < uIterations; ++i) {
    //     float r = size / scale;
    //     vec3 q = mod(p + r, 2.0 * r) - r;
    //     d = max(d, getCross(q, r));
    //     scale *= 3.0;
    // }

    // return vec4(vec3(1.0), d);
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

float GetLight(vec3 p, vec3 lightPos) {
    vec3 l = normalize(lightPos-p);
    vec3 n = getNormal(p);
    
    float dif = clamp(dot(n, l) * 0.5 + 0.5, 0., 1.);
    // shadow
    float d = rayMarch(p+n*SURF_DIST*2., l, MAX_STEPS).w;
    if (d < length(lightPos - p)) dif *= .5;
    
    return dif;
}

void main() {
    vec3 col = vec3(0.0);
    vec2 uv = v_uv * 2.0 - 1.0;

    vec3 origin = (uCameraToWorld * vec4(0, 0, 0, 1)).xyz;
    vec3 dir = (uCameraInverseProjection * vec4(uv, 0, 1)).xyz;
    dir = (uCameraToWorld * vec4(dir, 0)).xyz;
    dir = normalize(dir);

    vec4 res = rayMarch(origin, dir, MAX_STEPS);
    if (res.w < MAX_DIST) {
        vec3 p = origin + dir * res.w;
        float ao = GetLight(p, LIGHT_POS);
        col = ao * res.rgb;
    }

    col = pow(col, vec3(.4545));
    fragColor = vec4(col, 1.0);
}