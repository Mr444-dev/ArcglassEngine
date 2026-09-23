struct VSOutput {
    float4 position : SV_Position;
    float3 color : COLOR;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;

    float2 positions[3] = {
        float2( 0.0f,  0.5f),
        float2( 0.5f, -0.5f),
        float2(-0.5f, -0.5f)
    };

    float3 colors[3] = {
        float3(1.0f, 0.25f, 0.25f),
        float3(0.25f, 1.0f, 0.25f),
        float3(0.25f, 0.5f, 1.0f)
    };

    output.position = float4(positions[vertexID % 3], 0.0f, 1.0f);
    output.color = colors[vertexID % 3];
    return output;
}

float4 PSMain(VSOutput input) : SV_Target {
    return float4(input.color, 1.0f);
}
