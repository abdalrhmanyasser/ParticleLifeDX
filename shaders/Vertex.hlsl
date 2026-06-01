struct Particle {
    float2 pos;
    float2 vel;
    int type;
    float3 pad;
};

StructuredBuffer<Particle> Particles : register(t0);

// NEW: Receive the Constants from C++ (Now with matching memory alignment!)
cbuffer Constants : register(b0) {
    uint numParticles;
    uint numTypes;
    float rMax;
    float rMin;
    float dt;
    float friction;
    
    // The missing mouse variables that caused the overlap!
    float2 mousePos;
    float mouseForce;
    float mouseRadius;
    float2 pad;
    
    float4 typeColors[16]; 
};
struct VS_INPUT {
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    Particle p = Particles[input.instanceID];
    
    float2 quadCoords[4] = {
        float2(-1.0,  1.0), float2( 1.0,  1.0),
        float2(-1.0, -1.0), float2( 1.0, -1.0)
    };
    
    float2 localPos = quadCoords[input.vertexID];
    float2 worldPos = (localPos * 0.005) + p.pos; 
    
    output.pos = float4(worldPos, 0.0, 1.0);
    
    // Read the exact color from our C++ constant buffer
    output.color = typeColors[p.type].rgb; 
    
    return output;
}