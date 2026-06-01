struct Particle {
    float2 pos;
    float2 vel;
    int type;
    float3 pad;
};

StructuredBuffer<float> Rules : register(t0);
RWStructuredBuffer<Particle> Particles : register(u0);

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

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint index = id.x;
    if (index >= numParticles) return;

    Particle p1 = Particles[index];
    float2 totalForce = float2(0.0, 0.0);

    // --- PARTICLE INTERACTIONS ---
    for (uint i = 0; i < numParticles; i++) {
        if (i == index) continue;

        Particle p2 = Particles[i];
        float2 dir = p2.pos - p1.pos;
        float dist = length(dir);

        if (dist > 0.0 && dist < rMax) {
            float2 norm = dir / dist;
            float force = 0.0;
            float rule = Rules[p1.type * numTypes + p2.type];

            if (dist < rMin) {
                force = (dist / rMin) - 1.0;
                totalForce += norm * force * 3.0; // Strong short-range push
            } else {
                force = rule * (1.0 - abs(2.0 * dist - rMax - rMin) / (rMax - rMin));
                totalForce += norm * force;
            }
        }
    }

    // --- MOUSE INTERACTION ---
    if (mouseForce != 0.0) {
        float2 mDir = mousePos - p1.pos;
        float mDist = length(mDir);

        if (mDist > 0.0 && mDist < mouseRadius) {
            float2 mNorm = mDir / mDist;
            // Scale force smoothly to 0 at the edge of the radius
            float mMag = mouseForce * (1.0 - (mDist / mouseRadius)) * 100.0;
            totalForce += mNorm * mMag;
        }
    }

    // --- DYNAMIC WALL LOGIC ---
    totalForce -= p1.pos * (0.2 / worldSize); // Gentle center gravity

    float softEdge = worldSize * 0.85;
    float wallForce = 150.0;

    // Soft spring walls
    if (p1.pos.x > softEdge) totalForce.x -= (p1.pos.x - softEdge) * wallForce;
    else if (p1.pos.x < -softEdge) totalForce.x += (-softEdge - p1.pos.x) * wallForce;

    if (p1.pos.y > softEdge) totalForce.y -= (p1.pos.y - softEdge) * wallForce;
    else if (p1.pos.y < -softEdge) totalForce.y += (-softEdge - p1.pos.y) * wallForce;

    // Apply kinematics
    p1.vel = (p1.vel + totalForce * dt) * friction;
    p1.pos += p1.vel * dt;

    // Hard velocity bounce based on the dynamic Map Size
    if (p1.pos.x > worldSize) { p1.pos.x = worldSize; p1.vel.x *= -0.5; }
    else if (p1.pos.x < -worldSize) { p1.pos.x = -worldSize; p1.vel.x *= -0.5; }

    if (p1.pos.y > worldSize) { p1.pos.y = worldSize; p1.vel.y *= -0.5; }
    else if (p1.pos.y < -worldSize) { p1.pos.y = -worldSize; p1.vel.y *= -0.5; }

    Particles[index] = p1;
}