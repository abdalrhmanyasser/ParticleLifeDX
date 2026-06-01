struct Particle {
    float2 pos;
    float2 vel;
    int type;
    float3 pad;
};

StructuredBuffer<Particle> Particles : register(t0);

cbuffer Constants : register(b0) {
    uint numParticles;
    uint numTypes;
    float rMax;
    float rMin;
    float dt;
    float friction;
    
    float2 mousePos;
    float mouseForce;
    float mouseRadius;
    
    float2 pan;
    float zoom;
    float worldSize;
    float2 pad2;
    
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
    
    float2 particleCenter = (p.pos * zoom) + pan;
    float2 worldPos = (localPos * 0.005 * zoom) + particleCenter; 
    
    output.pos = float4(worldPos, 0.0, 1.0);
    output.color = typeColors[p.type].rgb; 
    
    return output;
}