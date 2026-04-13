struct VSInput {
    int position_x : PositionX;
    int position_y : PositionY;
    int position_z : PositionZ;
    uint data : Data;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 pos : Position;
    float3 normal : Normal;
    float ao : AO;
};

uniform float4x4 uViewMatrix;
uniform float4x4 uProjectionMatrix;
uniform int uIterations;

static const float AO_VALUES[4] = { 0.1, 0.25, 0.5, 1.0 };

VSOutput VS(VSInput inp) {
    float3 p = float3(inp.position_x, inp.position_y, inp.position_z);
    float3 pos = p / pow(3, uIterations);

    int normal_x = ((inp.data >> 6) & 0x3) - 1;
    int normal_y = ((inp.data >> 4) & 0x3) - 1;
    int normal_z = ((inp.data >> 2) & 0x3) - 1;
    int ao_id = inp.data & 0x3;

    VSOutput outp;
    outp.normal = float3(normal_x, normal_y, normal_z);
    outp.position = mul(mul(uProjectionMatrix, uViewMatrix), float4(pos, 1.0));
    outp.pos = pos;
    outp.ao = AO_VALUES[ao_id];
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