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

    DirectX::XMFLOAT2 mousePos;
    float mouseForce;
    float mouseRadius;

    DirectX::XMFLOAT2 pan;
    float zoom;

    // --- NEW: Map Size ---
    float worldSize;

    // CHANGED: Shrunk padding from 3 floats to 2 floats (8 bytes)
    // zoom(4) + worldSize(4) + pad2(8) = exactly 16 bytes!
    float pad2[2];
    // ---------------------

    DirectX::XMFLOAT4 typeColors[16];
};
class Renderer
{
public:
    Renderer(HWND hwnd);
    ~Renderer();

    void InitParticles(uint32_t count);
    void RandomizeRules(uint32_t newNumTypes);
    void RebuildParticles(uint32_t newCount); // NEW
    void UpdateAndRender();
    void Resize(UINT width, UINT height);

private:
    HWND m_hwnd;
    UINT m_width = 800;
    UINT m_height = 600;
    float m_worldSize = 2.0f; // Default map size
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
    int m_targetTypes = 6;
    int m_targetParticles = 15000; // NEW

    float m_mouseStrength = 5.0f;
    float m_mouseRadius = 0.4f;

    // NEW: Camera State
    DirectX::XMFLOAT2 m_pan = {0.0f, 0.0f};
    float m_zoom = 1.0f;
};