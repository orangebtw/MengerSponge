#version 330 core

in vec2 v_uv;

out vec4 fragColor;

uniform mat4 uCameraToWorld;
uniform mat4 uCameraInverseProjection;
uniform int uIterations;

const int MAX_STEPS = 256;
const float MAX_DIST = 100.0;
const float EPSILON  = 1e-5;
const float SURF_DIST = 0.001;
const float SHADOW_BIAS = EPSILON * 50;

const float AO_SAMPLES = 10.0;
const float AO_FACTOR = 0.25;

const vec3 LIGHT_POS = vec3(2.0, 2.0, 0.0);
const float AMBIENT_LIGHT = 0.2;

const float PI = 3.14159265359;

float GetSphere(vec3 p, float r) {
    return length(p) - r;
}

float GetBox(vec3 p, float size) {
    p = abs(p) - size;
    return max(p.x, max(p.y, p.z));
}

float GetCross(vec3 p, float size) {
    p = abs(p) - size / 3.0;
    float bx = max(p.y, p.z);
    float by = max(p.x, p.z);
    float bz = max(p.x, p.y);
    return min(min(bx, by), bz);
}

float GetInnerMenger(vec3 p, float size) {
    float d = EPSILON;
    float scale = 1.0;
    
    for (int i = 0; i < uIterations; ++i) {
        float r = size / scale;
        vec3 q = mod(p + r, 2.0 * r) - r;
        d = min(d, GetCross(q, r));
        scale *= 3.0;
    }
    return d;
}

vec4 SceneInfo(vec3 p) {
    float d1 = max(GetBox(p, 0.5), -GetInnerMenger(p, 0.5));
    float d2 = GetSphere(p - LIGHT_POS + vec3(0.5, 0.5, 0.0), 0.1);
    if (d1 < d2)
        return vec4(1.0, 1.0, 1.0, d1);
    else
        return vec4(1.0, 1.0, 0.0, d2);

    // float d = 0.0;
    // float cr = GetCross(p, size);
    // d = max(d, cr);

    // float scale = 1.0;
    // for (int i = 0; i < uIterations; ++i) {
    //     float r = size / scale;
    //     vec3 q = mod(p + r, 2.0 * r) - r;
    //     d = max(d, GetCross(q, r));
    //     scale *= 3.0;
    // }

    // return vec4(vec3(1.0), d);
}

vec3 GetNormal(vec3 p) {
    vec2 e = vec2(EPSILON, 0.0);
    vec3 n = SceneInfo(p).w - vec3(SceneInfo(p - e.xyy).w, SceneInfo(p - e.yxy).w, SceneInfo(p - e.yyx).w);
    return normalize(n);
}

vec4 RayMarch(vec3 ro, vec3 rd, int steps) {
    float progress = 0.0;
    vec3 color = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < steps; ++i) {
        vec3 samplePoint = ro + rd * progress;
        vec4 res = SceneInfo(samplePoint);
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
    vec3 n = GetNormal(p);
    
    // float dif = clamp(dot(n, l) * 0.5 + 0.5, 0., 1.);
    float dif = max(dot(n, l), 0.0);
    // shadow
    float d = RayMarch(p + n*SURF_DIST, l, MAX_STEPS).w;
    if (d < length(lightPos - p))
        dif *= 0.5;
    
    return dif;
}

vec3 randomSphereDir(vec2 rnd)
{
    float s = rnd.x*PI*2.;
    float t = rnd.y*2.-1.;
    return vec3(sin(s), cos(s), t) / sqrt(1.0 + t * t);
}

#define HASHSCALE1 .1231
float hash(float p)
{
	vec3 p3  = fract(vec3(p, p, p) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 randomHemisphereDir(vec3 dir, float i)
{
    vec3 v = randomSphereDir( vec2(hash(i+1.), hash(i+2.)) );
    return v * sign(dot(v, dir));
}

float GetAO( in vec3 p, in vec3 n, in float maxDist, in float falloff )
{
    const int nbIte = 32;
    const float nbIteInv = 1./float(nbIte);
    const float rad = 1.-1.*nbIteInv; //Hemispherical factor (self occlusion correction)

    float ao = 0.0;

    for( int i=0; i<nbIte; i++ )
    {
        float l = hash(float(i))*maxDist;
        vec3 rd = normalize(n+randomHemisphereDir(n, l )*rad)*l; // mix direction with the normal for self occlusion problems!

        ao += (l - max(SceneInfo( p + rd ).w,0.)) / maxDist * falloff;
    }

    return clamp( 1.-ao*nbIteInv, 0., 1.);
}

// float GetAO(vec3 pos, vec3 norm) {
//     float result = 1.0;
//     float s = -AO_SAMPLES;
//     float unit = 1.0 / AO_SAMPLES;
//     for (float i = unit; i < 1.0; i += unit) {
//         result -= pow(1.6, i * s) * (i - SceneInfo(pos + i * norm).w);
//     }
//     return result;
// }

void main() {
    vec3 col = vec3(0.0, 0.0, 0.0);
    vec2 uv = v_uv * 2.0 - 1.0;

    vec3 origin = (uCameraToWorld * vec4(0, 0, 0, 1)).xyz;
    vec3 dir = (uCameraInverseProjection * vec4(uv, 0, 1)).xyz;
    dir = (uCameraToWorld * vec4(dir, 0)).xyz;
    dir = normalize(dir);

    vec4 res = RayMarch(origin, dir, MAX_STEPS);
    if (res.w < MAX_DIST) {
        vec3 p = origin + dir * res.w;

        float direct_light = GetLight(p, LIGHT_POS);
        float ambient = GetAO(p, GetNormal(p), 4.0, 2.0) * 0.5;

        col = min(ambient + direct_light, 1.0) * res.rgb;
    }

    col = pow(col, vec3(0.4545, 0.4545, 0.4545));
    fragColor = vec4(col, 1.0);
}