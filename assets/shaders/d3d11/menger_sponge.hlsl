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

static const int MAX_STEPS = 300;
static const float MAX_DIST = 100.0;
static const float EPSILON  = 1e-5;
static const float SURF_DIST = 0.001;
static const float SHADOW_BIAS = EPSILON * 50;

static const float AO_SAMPLES = 10.0;
static const float AO_FACTOR = 1.0;

static const float3 LIGHT_POS = float3(3.0, 3.0, 0.0);

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

float GetInnerMenger(float3 p, float size) {
    float d = EPSILON;
    float scale = 1.0;
    
    for (int i = 0; i < uIterations; ++i) {
        float r = size / scale;
        float3 q = fmod(p + r, 2.0 * r) - r;
        d = min(d, GetCross(q, r));
        scale *= 3.0;
    }
    return d;
}

float4 Blend(float a, float b, float3 colA, float3 colB, float k) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    float blendDst = lerp( b, a, h ) - k*h*(1.0-h);
    float3 blendCol = lerp(colB,colA,h);
    return float4(blendCol, blendDst);
}

float4 SceneInfo(float3 p) {
    float size = 0.5;
    float d1 = max(GetBox(p, 0.5), -GetInnerMenger(p, 0.5));
    float d2 = GetSphere(p - LIGHT_POS + float3(0.25, 0.25, 0.25), 0.1);
    return Blend(d1, d2, float3(1.0, 1.0, 1.0), float3(1.0, 1.0, 0.0), 1.0);

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

float GetAO(float3 pos, float3 norm) {
    float result = 1.0;
    float s = -AO_SAMPLES;
    float unit = 1.0 / AO_SAMPLES;
    for (float i = unit; i < 1.0; i += unit) {
        result -= pow(1.6, i * s) * (i - SceneInfo(pos + i * norm).w);
    }
    return result * AO_FACTOR;
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
    
    float dif = clamp(dot(n, l) * 0.5 + 0.5, 0., 1.);
    // shadow
    float d = RayMarch(p+n*SURF_DIST*2., l, MAX_STEPS).w;
    if (d < length(lightPos - p)) dif *= .5;
    
    return dif;
}

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
        float ao = GetLight(p, LIGHT_POS);
        col = ao * res.rgb;
    }

    col = pow(col, float3(0.4545, 0.4545, 0.4545));
    return float4(col, 1.0);
}