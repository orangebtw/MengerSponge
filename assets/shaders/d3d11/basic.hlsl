struct VSInput {
    float3 position : Position;
    uint ao_id : AoID;
    int normal_x : NormalX;
    int normal_y : NormalY;
    int normal_z : NormalZ;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 pos : Position;
    float3 normal : Normal;
    float ao : AO;
};

uniform float4x4 uViewMatrix;
uniform float4x4 uProjectionMatrix;

static const float AO_VALUES[4] = { 0.1, 0.25, 0.5, 1.0 };

VSOutput VS(VSInput inp) {
    VSOutput outp;
    outp.normal = float3(inp.normal_x, inp.normal_y, inp.normal_z);
    outp.position = mul(mul(uProjectionMatrix, uViewMatrix), float4(inp.position, 1.0));
    outp.pos = inp.position;
    outp.ao = AO_VALUES[inp.ao_id];
    return outp;
}

static const float3 LIGHT_POS = float3(3.0, 3.0, 3.0);

float4 PS(VSOutput inp, uint primitiveId : SV_PrimitiveID) : SV_Target {
    float ambient = 0.2;

    float3 normal = normalize(inp.normal);
    float3 lightDir = normalize(LIGHT_POS - inp.pos);

    float diffuse = max(dot(normal, lightDir), 0.0);

    return float4(1.0, 1.0, 1.0, 1.0) * (diffuse + ambient) * inp.ao;
    // return float4(1.0, 1.0, 1.0, 1.0) * inp.ao;
}