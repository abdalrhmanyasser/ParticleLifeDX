#include "Renderer.h"
#include <d3dcompiler.h>
#include <random>
#include <time.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <string> // For dynamic color labels
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

Renderer::Renderer(HWND hwnd) : m_hwnd(hwnd), m_ruleBuffer(nullptr), m_ruleSRV(nullptr)
{
    // 1. Create Device and Swap Chain
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                  D3D11_SDK_VERSION, &scd, &m_swapChain, &m_device, nullptr, &m_context);

    ID3D11Texture2D *backBuffer;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&backBuffer);
    m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();

    // 2. Compile Shaders
    ID3DBlob *csBlob, *vsBlob, *psBlob;
    D3DCompileFromFile(L"shaders/Compute.hlsl", nullptr, nullptr, "main", "cs_5_0", 0, 0, &csBlob, nullptr);
    m_device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &m_computeShader);

    D3DCompileFromFile(L"shaders/Vertex.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);

    D3DCompileFromFile(L"shaders/Pixel.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, nullptr);
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

    csBlob->Release();
    vsBlob->Release();
    psBlob->Release();
    // Compile Matrix Shaders
    ID3DBlob *matVSBlob, *matPSBlob;
    D3DCompileFromFile(L"shaders/Matrix.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &matVSBlob, nullptr);
    m_device->CreateVertexShader(matVSBlob->GetBufferPointer(), matVSBlob->GetBufferSize(), nullptr, &m_matrixVS);

    D3DCompileFromFile(L"shaders/Matrix.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &matPSBlob, nullptr);
    m_device->CreatePixelShader(matPSBlob->GetBufferPointer(), matPSBlob->GetBufferSize(), nullptr, &m_matrixPS);

    matVSBlob->Release();
    matPSBlob->Release();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark(); // Use the built-in dark theme
    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_device, m_context);
}

Renderer::~Renderer()
{
    // --- NEW: Clean up ImGui ---
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    // Release all DirectX COM objects to prevent memory leaks
    if (m_constantBuffer)
        m_constantBuffer->Release();
    if (m_ruleSRV)
        m_ruleSRV->Release();
    if (m_ruleBuffer)
        m_ruleBuffer->Release();

    if (m_particleSRV)
        m_particleSRV->Release();
    if (m_particleUAV)
        m_particleUAV->Release();
    if (m_particleBuffer)
        m_particleBuffer->Release();

    if (m_pixelShader)
        m_pixelShader->Release();
    if (m_vertexShader)
        m_vertexShader->Release();
    if (m_computeShader)
        m_computeShader->Release();

    if (m_rtv)
        m_rtv->Release();
    if (m_swapChain)
        m_swapChain->Release();
    if (m_context)
        m_context->Release();
    if (m_device)
        m_device->Release();
    if (m_matrixVS)
        m_matrixVS->Release();
    if (m_matrixPS)
        m_matrixPS->Release();
}
// ADD THIS NEW FUNCTION anywhere in Renderer.cpp
void Renderer::Resize(UINT width, UINT height)
{
    if (!m_swapChain || width == 0 || height == 0)
        return;

    m_width = width;
    m_height = height;

    // 1. Release the old render target
    if (m_rtv)
    {
        m_rtv->Release();
        m_rtv = nullptr;
    }

    // 2. Resize the internal GPU buffers to match the new window size
    m_swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

    // 3. Recreate the render target
    ID3D11Texture2D *backBuffer = nullptr;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer);
    m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();
}
void Renderer::UpdateAndRender()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();
    float sidebarWidth = 300.0f;
    float simWidth = (float)m_width - sidebarWidth;
    if (simWidth < 100.0f)
        simWidth = 100.0f;

    // ==========================================
    // --- MOUSE LOGIC & VISUAL RING ---
    // ==========================================
    m_constants.mouseForce = 0.0f; // Default to off

    // Only apply mouse physics if the cursor is hovering over the simulation area (not the sidebar)
    if (io.MousePos.x < simWidth && io.MousePos.y < m_height && !io.WantCaptureMouse)
    {

        // Map pixel coordinates to DirectX Normalized Device Coordinates (-1 to 1)
        m_constants.mousePos.x = (io.MousePos.x / simWidth) * 2.0f - 1.0f;
        m_constants.mousePos.y = 1.0f - (io.MousePos.y / m_height) * 2.0f;
        m_constants.mouseRadius = m_mouseRadius;

        if (ImGui::IsMouseDown(0))
            m_constants.mouseForce = m_mouseStrength; // Left Click = Attract
        if (ImGui::IsMouseDown(1))
            m_constants.mouseForce = -m_mouseStrength; // Right Click = Repel

        // Draw the visual feedback ring if a button is held
        if (m_constants.mouseForce != 0.0f)
        {
            // Determine ring color (Green for attract, Red for repulse)
            ImU32 ringColor = (m_constants.mouseForce > 0.0f) ? IM_COL32(50, 255, 50, 200) : IM_COL32(255, 50, 50, 200);

            // Convert the radius back to screen pixels for ImGui to draw
            float pixelRadius = m_mouseRadius * (simWidth / 2.0f);

            // Draw a circle directly over the DirectX render!
            ImGui::GetForegroundDrawList()->AddCircle(io.MousePos, pixelRadius, ringColor, 64, 3.0f);
        }
    }

    // Upload live changes (including mouse data) to the GPU immediately
    m_context->UpdateSubresource(m_constantBuffer, 0, nullptr, &m_constants, 0, 0);

    // ==========================================
    // 1. COMPUTE PASS (Physics)
    // ==========================================
    m_context->CSSetShader(m_computeShader, nullptr, 0);
    m_context->CSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->CSSetShaderResources(0, 1, &m_ruleSRV);
    m_context->CSSetUnorderedAccessViews(0, 1, &m_particleUAV, nullptr);
    m_context->Dispatch((m_numParticles + 255) / 256, 1, 1);

    ID3D11UnorderedAccessView *nullUAV = nullptr;
    m_context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

    // ==========================================
    // 2. RENDER PASS (Graphics)
    // ==========================================
    float clearColor[4] = {0.05f, 0.05f, 0.05f, 1.0f};
    m_context->ClearRenderTargetView(m_rtv, clearColor);
    m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // -- Viewport 1: The Simulation --
    D3D11_VIEWPORT vpSim = {0.0f, 0.0f, simWidth, (float)m_height, 0.0f, 1.0f};
    m_context->RSSetViewports(1, &vpSim);
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->VSSetShaderResources(0, 1, &m_particleSRV);
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->DrawInstanced(4, m_numParticles, 0, 0);

    // -- Viewport 2: The Sidebar --
    D3D11_VIEWPORT vpSidebar = {simWidth, 0.0f, sidebarWidth, (float)m_height, 0.0f, 1.0f};
    m_context->RSSetViewports(1, &vpSidebar);
    m_context->VSSetShader(m_matrixVS, nullptr, 0);
    m_context->PSSetShader(m_matrixPS, nullptr, 0);
    m_context->VSSetShaderResources(0, 1, &m_ruleSRV);
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->DrawInstanced(4, m_constants.numTypes * m_constants.numTypes, 0, 0);

    ID3D11ShaderResourceView *nullSRV = nullptr;
    m_context->VSSetShaderResources(0, 1, &nullSRV);

    // ==========================================
    // 3. BUILD AND RENDER UI
    // ==========================================
    ImGui::SetNextWindowPos(ImVec2(simWidth, 0));
    ImGui::SetNextWindowSize(ImVec2(sidebarWidth, (float)m_height));
    ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Simulation Info");
    ImGui::Text("Particles: %d", m_numParticles);
    ImGui::Text("Active Types: %d", m_constants.numTypes);
    ImGui::Text("FPS: %.1f", io.Framerate);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Mouse Tools (LMB/RMB)");
    ImGui::SliderFloat("Tool Power", &m_mouseStrength, 1.0f, 20.0f);
    ImGui::SliderFloat("Tool Radius", &m_mouseRadius, 0.1f, 1.0f);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Physics Limits");
    ImGui::SliderFloat("Max Radius", &m_constants.rMax, 0.01f, 0.8f);
    ImGui::SliderFloat("Min Radius", &m_constants.rMin, 0.001f, 0.2f);
    ImGui::SliderFloat("Sim Speed", &m_constants.dt, 0.001f, 0.05f);
    ImGui::SliderFloat("Friction", &m_constants.friction, 0.1f, 1.0f);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Universe Controls");
    ImGui::SliderInt("Target Types", &m_targetTypes, 2, 16);

    if (ImGui::Button("Mutate & Apply Target Types", ImVec2(-1, 35)))
    {
        RandomizeRules(m_targetTypes);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Type Colors");
    for (uint32_t i = 0; i < m_constants.numTypes; ++i)
    {
        std::string label = "Type " + std::to_string(i);
        ImGui::ColorEdit3(label.c_str(), (float *)&m_constants.typeColors[i]);
    }

    ImGui::Dummy(ImVec2(0.0f, 200.0f));
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_swapChain->Present(1, 0);
}
void Renderer::InitParticles(uint32_t count)
{
    m_numParticles = count;

    // Generate random base particles
    std::vector<Particle> particles(count);
    for (auto &p : particles)
    {
        p.pos = {(rand() % 200 - 100) / 100.0f, (rand() % 200 - 100) / 100.0f};
        p.vel = {0.0f, 0.0f};
        p.type = rand() % 16; // Randomize base particles across a max of 16 types
    }

    // Create Particle Buffer (Bind flags: UAV for compute, SRV for vertex)
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.Usage = D3D11_USAGE_DEFAULT;
    bufDesc.ByteWidth = sizeof(Particle) * count;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufDesc.StructureByteStride = sizeof(Particle);

    D3D11_SUBRESOURCE_DATA initData = {particles.data(), 0, 0};
    m_device->CreateBuffer(&bufDesc, &initData, &m_particleBuffer);

    m_device->CreateUnorderedAccessView(m_particleBuffer, nullptr, &m_particleUAV);
    m_device->CreateShaderResourceView(m_particleBuffer, nullptr, &m_particleSRV);

    // Initialize Constants
    m_constants = {count, 0, 0.2f, 0.05f, 0.016f, 0.5f, {0, 0}};
    bufDesc.ByteWidth = sizeof(Constants);
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.MiscFlags = 0;
    bufDesc.StructureByteStride = 0;
    m_device->CreateBuffer(&bufDesc, nullptr, &m_constantBuffer);

    // Trigger the first set of rules (starts with 6 types)
    RandomizeRules(6);
}
void Renderer::RandomizeRules(uint32_t newNumTypes)
{
    // 1. Release the old buffer if it exists
    if (m_ruleSRV)
    {
        m_ruleSRV->Release();
        m_ruleSRV = nullptr;
    }
    if (m_ruleBuffer)
    {
        m_ruleBuffer->Release();
        m_ruleBuffer = nullptr;
    }

    // 2. True Randomness Setup
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    // 3. Generate New Rules
    std::vector<float> rules(newNumTypes * newNumTypes);
    for (auto &r : rules)
    {
        r = dist(gen);
    }

    // 4. Create the new Rule Buffer
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.Usage = D3D11_USAGE_DEFAULT;
    bufDesc.ByteWidth = sizeof(float) * rules.size();
    bufDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufDesc.StructureByteStride = sizeof(float);
    bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA initData = {rules.data(), 0, 0};
    m_device->CreateBuffer(&bufDesc, &initData, &m_ruleBuffer);
    m_device->CreateShaderResourceView(m_ruleBuffer, nullptr, &m_ruleSRV);

    // 5. Randomize custom colors for the new types
    for (uint32_t i = 0; i < newNumTypes; ++i)
    {
        m_constants.typeColors[i] = {
            dist(gen) * 0.5f + 0.5f, // Keep colors bright (0.5 to 1.0)
            dist(gen) * 0.5f + 0.5f,
            dist(gen) * 0.5f + 0.5f,
            1.0f};
    }

    // 6. Update the GPU Constants with the new type count
    m_constants.numTypes = newNumTypes;
    m_context->UpdateSubresource(m_constantBuffer, 0, nullptr, &m_constants, 0, 0);

    // --- NEW: RESET PARTICLES TO FIX "STATIC" BUG ---
    // Generate a fresh batch of particles to scatter the old formations
    std::vector<Particle> particles(m_numParticles);
    for (auto &p : particles)
    {
        p.pos = {(rand() % 200 - 100) / 100.0f, (rand() % 200 - 100) / 100.0f}; // Random position
        p.vel = {0.0f, 0.0f};                                                   // Reset momentum

        // CRITICAL: Force the particle type to be within the new universe bounds
        p.type = rand() % newNumTypes;
    }

    // Upload the fresh particles to the GPU, overwriting the frozen ones
    m_context->UpdateSubresource(m_particleBuffer, 0, nullptr, particles.data(), 0, 0);
    // ------------------------------------------------
}