#include "../util/logger.h"
#include <d3d11.h>
#include <dxgi.h>
#include <cstring>

typedef HRESULT(WINAPI* pD3DCompile)(
    const void*, SIZE_T, const char*, void*, void*,
    const char*, const char*, UINT, UINT, void**, void**);

static const GUID IID_Tex2D_NM = {0x6f15aaf2,0xd208,0x4e89,{0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c}};

namespace NightMode {

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static ID3D11PixelShader* g_ps = nullptr;
static ID3D11VertexShader* g_vs = nullptr;
static ID3D11BlendState* g_blend = nullptr;
static ID3D11DepthStencilState* g_dss = nullptr;
static ID3D11RasterizerState* g_rast = nullptr;
static ID3D11Buffer* g_cb = nullptr;
static ID3D11RenderTargetView* g_cachedRTV = nullptr; // CACHED - not recreated each frame
static UINT g_cachedW = 0, g_cachedH = 0;
static bool g_enabled = false;
static float g_intensity = 0.3f;
static bool g_initialized = false;

// Vertex shader: fullscreen triangle from SV_VertexID (no vertex buffer)
static const char* VS_SRC =
    "struct O{float4 p:SV_Position;float2 u:TEXCOORD0;};\n"
    "O VS(uint id:SV_VertexID){O o;o.u=float2((id<<1)&2,id&2);\n"
    "o.p=float4(o.u*float2(2,-2)+float2(-1,1),0,1);return o;}\n";

// Pixel shader: UV-based sky detection (NO backbuffer read, NO CopyResource!)
// Multiply blend does: final = output * backbuffer
// So we output tint values: sky area gets dark tint, ground gets 1.0 (unchanged)
static const char* PS_SRC =
    "cbuffer CB:register(b0){float4 tint;};\n"
    "struct I{float4 p:SV_Position;float2 u:TEXCOORD0;};\n"
    "float4 PS(I i):SV_Target{\n"
    "  float top=saturate(1.0-i.u.y*3.0);\n"
    "  float sky=top*top;\n" // quadratic falloff for smoother transition
    "  float3 m=lerp(float3(1,1,1),tint.xyz,sky);\n"
    "  return float4(m,1);\n"
    "}\n";

void Init(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (g_initialized) return;
    g_dev = device; g_ctx = context;

    HMODULE hC = LoadLibraryA("d3dcompiler_47.dll");
    if (!hC) return;
    auto compile = (pD3DCompile)GetProcAddress(hC, "D3DCompile");
    if (!compile) return;

    ID3D10Blob *b=nullptr, *e=nullptr;
    if (FAILED(compile(VS_SRC,strlen(VS_SRC),0,0,0,"VS","vs_4_0",0,0,(void**)&b,(void**)&e))) {
        if(e) e->Release(); return;
    }
    device->CreateVertexShader(b->GetBufferPointer(),b->GetBufferSize(),0,&g_vs);
    b->Release();

    if (FAILED(compile(PS_SRC,strlen(PS_SRC),0,0,0,"PS","ps_4_0",0,0,(void**)&b,(void**)&e))) {
        if(e) e->Release(); return;
    }
    device->CreatePixelShader(b->GetBufferPointer(),b->GetBufferSize(),0,&g_ps);
    b->Release();

    D3D11_BLEND_DESC bd={};
    bd.RenderTarget[0].BlendEnable=TRUE;
    bd.RenderTarget[0].SrcBlend=D3D11_BLEND_DEST_COLOR;
    bd.RenderTarget[0].DestBlend=D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&bd,&g_blend);

    D3D11_DEPTH_STENCIL_DESC dd={}; dd.DepthEnable=FALSE;
    device->CreateDepthStencilState(&dd,&g_dss);

    D3D11_RASTERIZER_DESC rd={}; rd.FillMode=D3D11_FILL_SOLID; rd.CullMode=D3D11_CULL_NONE;
    device->CreateRasterizerState(&rd,&g_rast);

    D3D11_BUFFER_DESC cb={}; cb.ByteWidth=16; cb.Usage=D3D11_USAGE_DYNAMIC;
    cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER; cb.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cb,0,&g_cb);

    g_initialized = true;
    LOG_INFO("[Night] Optimized shaders ready (no CopyResource)");
}

void Render(IDXGISwapChain* swapChain) {
    if (!g_enabled || !g_initialized) return;

    // Get backbuffer ONLY to check size and get/cache RTV
    ID3D11Texture2D* bb=nullptr;
    swapChain->GetBuffer(0,IID_Tex2D_NM,(void**)&bb);
    if (!bb) return;

    // Cache RTV - only recreate on resolution change
    D3D11_TEXTURE2D_DESC bbDesc;
    bb->GetDesc(&bbDesc);
    if (!g_cachedRTV || bbDesc.Width != g_cachedW || bbDesc.Height != g_cachedH) {
        if (g_cachedRTV) g_cachedRTV->Release();
        g_dev->CreateRenderTargetView(bb,0,&g_cachedRTV);
        g_cachedW = bbDesc.Width;
        g_cachedH = bbDesc.Height;
    }
    bb->Release();
    if (!g_cachedRTV) return;

    // Update tint constant buffer
    D3D11_MAPPED_SUBRESOURCE m;
    if(SUCCEEDED(g_ctx->Map(g_cb,0,D3D11_MAP_WRITE_DISCARD,0,&m))) {
        float t[4]={g_intensity*0.15f, g_intensity*0.18f, g_intensity*0.4f, 1.0f};
        memcpy(m.pData,t,16);
        g_ctx->Unmap(g_cb,0);
    }

    // Save/restore minimal state
    ID3D11RenderTargetView* oldRTV=nullptr;
    ID3D11DepthStencilView* oldDSV=nullptr;
    g_ctx->OMGetRenderTargets(1,&oldRTV,&oldDSV);

    // Render fullscreen triangle with multiply blend (NO CopyResource!)
    g_ctx->OMSetRenderTargets(1,&g_cachedRTV,nullptr);
    g_ctx->OMSetBlendState(g_blend,0,0xFFFFFFFF);
    g_ctx->OMSetDepthStencilState(g_dss,0);
    g_ctx->RSSetState(g_rast);
    g_ctx->VSSetShader(g_vs,0,0);
    g_ctx->PSSetShader(g_ps,0,0);
    g_ctx->PSSetConstantBuffers(0,1,&g_cb);
    g_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_ctx->IASetInputLayout(nullptr);
    g_ctx->Draw(3,0);

    // Restore
    g_ctx->OMSetRenderTargets(1,&oldRTV,oldDSV);
    if(oldRTV) oldRTV->Release();
    if(oldDSV) oldDSV->Release();
}

void SetEnabled(bool e) { g_enabled=e; }
void SetIntensity(float i) { g_intensity=i<0.05f?0.05f:i>1.0f?1.0f:i; }

void Shutdown() {
    if(g_cachedRTV){g_cachedRTV->Release();g_cachedRTV=nullptr;}
    if(g_cb){g_cb->Release();g_cb=nullptr;}
    if(g_blend){g_blend->Release();g_blend=nullptr;}
    if(g_dss){g_dss->Release();g_dss=nullptr;}
    if(g_rast){g_rast->Release();g_rast=nullptr;}
    if(g_vs){g_vs->Release();g_vs=nullptr;}
    if(g_ps){g_ps->Release();g_ps=nullptr;}
    g_initialized=false;
}

} // namespace NightMode
