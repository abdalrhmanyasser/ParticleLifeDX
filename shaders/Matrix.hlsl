StructuredBuffer<float> Rules : register(t0);

cbuffer Constants : register(b0) {
    uint numParticles;
    uint numTypes;
    float rMax;
    float rMin;
    float dt;
    float friction;
    float2 pad;
};

struct VS_INPUT {
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

PS_INPUT VSMain(VS_INPUT input) {
    PS_INPUT output;
    
    uint xIndex = input.instanceID % numTypes;
    uint yIndex = input.instanceID / numTypes;
    float ruleVal = Rules[input.instanceID];

    float2 quadCoords[4] = {
        float2(-1.0,  1.0), float2( 1.0,  1.0),
        float2(-1.0, -1.0), float2( 1.0, -1.0)
    };
    
    // Dynamically calculate the size of each square so the whole grid always fits in the sidebar
    // 1.8 is the maximum width of the grid (-0.9 to 0.9), leaving a small margin
    float gridWidth = 1.8; 
    float quadSize = gridWidth / (float)numTypes; 
    float padding = quadSize * 0.05; // 5% padding between squares
    
    // Center the grid in the top half of the sidebar
    float startX = -0.9 + (quadSize / 2.0); 
    float startY =  0.8 - (quadSize / 2.0); 
    
    // Scale the base quad to match the calculated size
    // We divide Y by 2.0 to account for the sidebar being roughly twice as tall as it is wide (300x600)
    // This keeps the squares from stretching into rectangles
    float2 localPos = quadCoords[input.vertexID] * float2((quadSize / 2.0) - padding, (quadSize / 4.0) - (padding/2.0));
    
    float2 screenPos = float2(
        startX + (xIndex * quadSize),
        startY - (yIndex * (quadSize / 2.0)) // Divide Y spacing by 2 to match the Y-scale fix above
    );

    output.pos = float4(screenPos + localPos, 0.0, 1.0);
    
    if (ruleVal < 0.0) {
        output.color = float3(1.0, 0.2, 0.2) * abs(ruleVal);
    } else {
        output.color = float3(0.2, 1.0, 0.2) * ruleVal; 
    }
    
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET {
    return float4(input.color, 1.0);
}