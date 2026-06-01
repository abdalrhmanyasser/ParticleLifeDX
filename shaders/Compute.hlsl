struct Particle {
    float2 pos;
    float2 vel;
    int type;
    float3 pad; // 16-byte alignment
};

cbuffer Constants : register(b0) {
    uint numParticles;
    uint numTypes;
    float rMax;
    float rMin;
    float dt;
    float friction;
    
    // NEW MOUSE CONSTANTS
    float2 mousePos;
    float mouseForce;
    float mouseRadius;
    float2 pad;
    
    float4 typeColors[16];
};

// Read/Write buffer for particles
RWStructuredBuffer<Particle> Particles : register(u0);

// Read-only buffer for the rule matrix
StructuredBuffer<float> Rules : register(t0);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint index = DTid.x;
    if (index >= numParticles) return;

    Particle p1 = Particles[index];
    float2 totalForce = float2(0.0, 0.0);

    for (uint i = 0; i < numParticles; i++) {
        if (i == index) continue;

        Particle p2 = Particles[i];
        float2 dir = p2.pos - p1.pos;
        float dist = length(dir);

        if (dist > 0.0 && dist < rMax) {
            float ruleValue = Rules[p1.type * numTypes + p2.type];

            // YOUR RULE: Skip if too close AND attractive
            if (dist < rMin && ruleValue > 0.0) {
                continue; 
            }

            float2 normDir = dir / dist;
            float forceMag = ruleValue * (1.0 - (dist / rMax));
            
            totalForce += normDir * forceMag;
        }
    }
    // --- UPGRADED WALL REPULSION LOGIC ---
    
    // 1. Center Gravity: A very subtle pull towards (0,0) to discourage corner clustering
    totalForce -= p1.pos * 0.5; 

    // 2. Stronger Springs: Push back hard when crossing the 0.85 threshold
    float edge = 0.85; 
    float wallForce = 150.0; 

    if (p1.pos.x > edge) {
        totalForce.x -= (p1.pos.x - edge) * wallForce; 
    } else if (p1.pos.x < -edge) {
        totalForce.x += (-edge - p1.pos.x) * wallForce;
    }

    if (p1.pos.y > edge) {
        totalForce.y -= (p1.pos.y - edge) * wallForce;
    } else if (p1.pos.y < -edge) {
        totalForce.y += (-edge - p1.pos.y) * wallForce;
    }
    // --- NEW: MOUSE INTERACTION LOGIC ---
    // Only calculate if the mouse button is actually being pressed
    if (mouseForce != 0.0) {
        float2 mDir = mousePos - p1.pos;
        float mDist = length(mDir);
        
        // If the particle is inside the mouse radius
        if (mDist > 0.0 && mDist < mouseRadius) {
            float2 mNorm = mDir / mDist;
            
            // The force gets exponentially stronger the closer the particle is to the center of the cursor
            float mMag = mouseForce * (1.0 - (mDist / mouseRadius)) * 250.0;
            totalForce += mNorm * mMag;
        }
    }
    // ------------------------------------

    // Update velocity and apply friction
    p1.vel = (p1.vel + totalForce * dt) * friction;
    
    // Update position
    p1.pos += p1.vel * dt;

    // 3. Hard Velocity Bounce: If the swarm overpowers the spring, bounce them!
    // We check against 0.99 to keep them strictly visible on screen
    if (p1.pos.x > 0.99) { 
        p1.pos.x = 0.99; 
        p1.vel.x *= -0.8; // Reverse momentum and dampen it slightly
    } else if (p1.pos.x < -0.99) { 
        p1.pos.x = -0.99; 
        p1.vel.x *= -0.8; 
    }

    if (p1.pos.y > 0.99) { 
        p1.pos.y = 0.99; 
        p1.vel.y *= -0.8; 
    } else if (p1.pos.y < -0.99) { 
        p1.pos.y = -0.99; 
        p1.vel.y *= -0.8; 
    }
    // ------------------------------------

    Particles[index] = p1;
}