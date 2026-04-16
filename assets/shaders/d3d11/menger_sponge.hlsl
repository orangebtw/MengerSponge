uniform float4x4 uCameraToWorld;
uniform float4x4 uCameraInverseProjection;
uniform int uIterations;

struct VSInput {
    float2 position : Position;
    float2 uv : UV;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
};

VSOutput VS(VSInput inp) {
    VSOutput outp;
    outp.uv = inp.uv;
    outp.position = float4(inp.position, 0.0, 1.0);
    return outp;
}

static const int MAX_STEPS = 256;
static const float MAX_DIST = 100.0;
static const float EPSILON  = 1e-5;
static const float SURF_DIST = 0.001;
static const float SHADOW_BIAS = EPSILON * 50;

static const float AO_SAMPLES = 10.0;
static const float AO_FACTOR = 0.25;

static const float3 LIGHT_POS = float3(2.0, 2.0, 0.0);
static const float AMBIENT_LIGHT = 0.2;

static const float PI = 3.14159265359;

float GetSphere(float3 p, float r) {
    return length(p) - r;
}

float GetBox(float3 p, float size) {
    p = abs(p) - size;
    return max(p.x, max(p.y, p.z));
}

float GetCross(float3 p, float size) {
    p = abs(p) - size / 3.0;
    float bx = max(p.y, p.z);
    float by = max(p.x, p.z);
    float bz = max(p.x, p.y);
    return min(min(bx, by), bz);
}

float3 mod(float3 x, float3 y) {
    return x - y * floor(x / y);
}

float GetInnerMenger(float3 p, float size) {
    float d = EPSILON;
    float scale = 1.0;
    
    for (int i = 0; i < uIterations; ++i) {
        float r = size / scale;
        float3 q = mod(p + r, 2.0 * r) - r;
        d = min(d, GetCross(q, r));
        scale *= 3.0;
    }
    return d;
}

float4 SceneInfo(float3 p) {
    float d1 = max(GetBox(p, 0.5), -GetInnerMenger(p, 0.5));
    float d2 = GetSphere(p - LIGHT_POS + float3(0.5, 0.5, 0.0), 0.1);
    if (d1 < d2)
        return float4(1.0, 1.0, 1.0, d1);
    else
        return float4(1.0, 1.0, 0.0, d2);

    // float d = 0.0;
    // float cr = GetCross(p, size);
    // d = max(d, cr);

    // float scale = 1.0;
    // for (int i = 0; i < uIterations; ++i) {
    //     float r = size / scale;
    //     float3 q = mod(p + r, 2.0 * r) - r;
    //     d = max(d, GetCross(q, r));
    //     scale *= 3.0;
    // }

    // return float4(float3(1.0), d);
}

float3 GetNormal(float3 p) {
    float2 e = float2(EPSILON, 0.0);
    float3 n = SceneInfo(p).w - float3(SceneInfo(p - e.xyy).w, SceneInfo(p - e.yxy).w, SceneInfo(p - e.yyx).w);
    return normalize(n);
}

float4 RayMarch(float3 ro, float3 rd, int steps) {
    float progress = 0.0;
    float3 color = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < steps; ++i) {
        float3 samplePoint = ro + rd * progress;
        float4 res = SceneInfo(samplePoint);
        if (res.w < EPSILON) {
            color = res.rgb;
            break;
        }
        if (progress > MAX_DIST)
            break;
        progress += res.w;
    }

    return float4(color, progress);
}

float GetLight(float3 p, float3 lightPos) {
    float3 l = normalize(lightPos-p);
    float3 n = GetNormal(p);
    
    // float dif = clamp(dot(n, l) * 0.5 + 0.5, 0., 1.);
    float dif = max(dot(n, l), 0.0);
    // shadow
    float d = RayMarch(p + n*SURF_DIST, l, MAX_STEPS).w;
    if (d < length(lightPos - p))
        dif *= 0.5;
    
    return dif;
}

float3 randomSphereDir(float2 rnd)
{
    float s = rnd.x*PI*2.;
    float t = rnd.y*2.-1.;
    return float3(sin(s), cos(s), t) / sqrt(1.0 + t * t);
}

#define HASHSCALE1 .1231
float hash(float p)
{
	float3 p3  = frac(float3(p, p, p) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return frac((p3.x + p3.y) * p3.z);
}

float3 randomHemisphereDir(float3 dir, float i)
{
    float3 v = randomSphereDir( float2(hash(i+1.), hash(i+2.)) );
    return v * sign(dot(v, dir));
}

float GetAO( in float3 p, in float3 n, in float maxDist, in float falloff )
{
    const int nbIte = 32;
    const float nbIteInv = 1./float(nbIte);
    const float rad = 1.-1.*nbIteInv; //Hemispherical factor (self occlusion correction)

    float ao = 0.0;

    for( int i=0; i<nbIte; i++ )
    {
        float l = hash(float(i))*maxDist;
        float3 rd = normalize(n+randomHemisphereDir(n, l )*rad)*l; // mix direction with the normal for self occlusion problems!

        ao += (l - max(SceneInfo( p + rd ).w,0.)) / maxDist * falloff;
    }

    return clamp( 1.-ao*nbIteInv, 0., 1.);
}

// float GetAO(float3 pos, float3 norm) {
//     float result = 1.0;
//     float s = -AO_SAMPLES;
//     float unit = 1.0 / AO_SAMPLES;
//     for (float i = unit; i < 1.0; i += unit) {
//         result -= pow(1.6, i * s) * (i - SceneInfo(pos + i * norm).w);
//     }
//     return result;
// }

float4 PS(VSOutput inp) : SV_Target {
    float3 col = float3(0.0, 0.0, 0.0);
    float2 uv = inp.uv * 2.0 - 1.0;

    float3 origin = mul(uCameraToWorld, float4(0, 0, 0, 1)).xyz;
    float3 dir = mul(uCameraInverseProjection, float4(uv, 0, 1)).xyz;
    dir = mul(uCameraToWorld, float4(dir, 0)).xyz;
    dir = normalize(dir);

    float4 res = RayMarch(origin, dir, MAX_STEPS);
    if (res.w < MAX_DIST) {
        float3 p = origin + dir * res.w;

        float direct_light = GetLight(p, LIGHT_POS);
        float ambient = GetAO(p, GetNormal(p), 4.0, 2.0) * 0.5;

        col = min(ambient + direct_light, 1.0) * res.rgb;
    }

    col = pow(col, float3(0.4545, 0.4545, 0.4545));
    return float4(col, 1.0);
}