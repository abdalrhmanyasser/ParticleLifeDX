#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <random>

struct alignas(16) Particle
{
    DirectX::XMFLOAT2 pos;
    DirectX::XMFLOAT2 vel;
    int type;
    float pad[3];
};
struct Constants
{
    uint32_t numParticles;
    uint32_t numTypes;
    float rMax;
    float rMin;
    float dt;
    float friction;

    // --- NEW: Mouse Interaction Variables ---
    DirectX::XMFLOAT2 mousePos;
    float mouseForce;      // Positive = Attract, Negative = Repulse, 0 = Off
    float mouseRadius;     // How far the mouse force reaches
    DirectX::XMFLOAT2 pad; // Padding to maintain 16-byte alignment
    // ----------------------------------------

    DirectX::XMFLOAT4 typeColors[16];
};
class Renderer
{
public:
    Renderer(HWND hwnd);
    ~Renderer();

    void InitParticles(uint32_t count);
    void RandomizeRules(uint32_t newNumTypes);
    void UpdateAndRender();

    void Resize(UINT width, UINT height); // NEW

private:
    int m_targetTypes = 6;
    float m_mouseStrength = 5.0f;
    float m_mouseRadius = 0.4f;
    HWND m_hwnd;
    UINT m_width = 800;  // NEW
    UINT m_height = 600; // NEW
    ID3D11Device *m_device;
    ID3D11DeviceContext *m_context;
    IDXGISwapChain *m_swapChain;
    ID3D11RenderTargetView *m_rtv;

    ID3D11Buffer *m_particleBuffer;
    ID3D11UnorderedAccessView *m_particleUAV;
    ID3D11ShaderResourceView *m_particleSRV;

    ID3D11Buffer *m_ruleBuffer;
    ID3D11ShaderResourceView *m_ruleSRV;

    ID3D11Buffer *m_constantBuffer;
    Constants m_constants;

    ID3D11ComputeShader *m_computeShader;
    ID3D11VertexShader *m_vertexShader;
    ID3D11PixelShader *m_pixelShader;

    ID3D11VertexShader *m_matrixVS;
    ID3D11PixelShader *m_matrixPS;

    uint32_t m_numParticles;
};