// =========================================================================
// Cascaded Shadow Maps (CSM) Depth Pass
// =========================================================================

struct ShadowConstants {
    float4x4 lightViewProj;
};

ConstantBuffer<ShadowConstants> ShadowCB : register(b0);

struct VSInput {
    float3 position : POSITION;
};

struct VSOutput {
    float4 positionCS : SV_Position;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.positionCS = mul(float4(input.position, 1.0f), ShadowCB.lightViewProj);
    return output;
}

void PSMain(VSOutput input) {
    // Hardware depth writes automatically to depth buffer
}
