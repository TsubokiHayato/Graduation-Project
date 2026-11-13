#include "BoosterParticlePSO.h"
#include <cassert>

// =============================================================================
//  Initialize
// =============================================================================
void BoosterParticlePSO::Initialize() {
    CreateRootSignature();
    // 通常のアルファ合成：色 = 粒子の色 × α + 元の色 × (1 - α)
    alphaBlendPipeline_ = CreatePipeline(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA);
    // 加算合成：色 = 元の色 + 粒子の色 × α（重なるほど明るくなる）
    additivePipeline_ = CreatePipeline(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_ONE);
}

void BoosterParticlePSO::SetAlphaBlend(ID3D12GraphicsCommandList* cmd) { Set(cmd, alphaBlendPipeline_.Get()); }
void BoosterParticlePSO::SetAdditive(ID3D12GraphicsCommandList* cmd) { Set(cmd, additivePipeline_.Get()); }

void BoosterParticlePSO::Set(ID3D12GraphicsCommandList* cmd, ID3D12PipelineState* pipeline) {
    if (!cmd || !pipeline) {
        return;
    }
    cmd->SetGraphicsRootSignature(rootSignature_.Get());
    cmd->SetPipelineState(pipeline);
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// =============================================================================
//  ルートシグネチャ（エンジンの ParticlePSO と同じ構成）
//  0: Material(CBV, PS) / 1: インスタンシング(SRV t0, VS) / 2: テクスチャ(SRV t0, PS)
// =============================================================================
void BoosterParticlePSO::CreateRootSignature() {
    auto* dx = TuboEngine::DirectXCommon::GetInstance();

    D3D12_DESCRIPTOR_RANGE textureRange[1] = {};
    textureRange[0].BaseShaderRegister = 0;
    textureRange[0].NumDescriptors = 1;
    textureRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    textureRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE instancingRange[1] = {};
    instancingRange[0] = textureRange[0];

    D3D12_ROOT_PARAMETER params[3] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[0].Descriptor.ShaderRegister = 0;

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    params[1].DescriptorTable.pDescriptorRanges = instancingRange;
    params[1].DescriptorTable.NumDescriptorRanges = 1;

    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[2].DescriptorTable.pDescriptorRanges = textureRange;
    params[2].DescriptorTable.NumDescriptorRanges = 1;

    D3D12_STATIC_SAMPLER_DESC sampler[1] = {};
    sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler[0].MaxLOD = D3D12_FLOAT32_MAX;
    sampler[0].ShaderRegister = 0;
    sampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    desc.pParameters = params;
    desc.NumParameters = _countof(params);
    desc.pStaticSamplers = sampler;
    desc.NumStaticSamplers = _countof(sampler);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    assert(SUCCEEDED(hr));
    hr = dx->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

// =============================================================================
//  パイプライン（ブレンド以外はエンジンの ParticlePSO と同じ）
// =============================================================================
Microsoft::WRL::ComPtr<ID3D12PipelineState> BoosterParticlePSO::CreatePipeline(D3D12_BLEND srcBlend, D3D12_BLEND destBlend) {
    auto* dx = TuboEngine::DirectXCommon::GetInstance();

    D3D12_INPUT_ELEMENT_DESC inputElements[4] = {};
    inputElements[0].SemanticName = "POSITION";
    inputElements[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElements[1].SemanticName = "TEXCOORD";
    inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElements[2].SemanticName = "NORMAL";
    inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElements[3].SemanticName = "COLOR";
    inputElements[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayout{};
    inputLayout.pInputElementDescs = inputElements;
    inputLayout.NumElements = _countof(inputElements);

    D3D12_BLEND_DESC blend{};
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = srcBlend;
    blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].DestBlend = destBlend;
    // 描画先のα（不透明=1）は変えない。粒子のαで上書きすると、Game ウィンドウで粒子の所が透けて黒く抜ける
    blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
    blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;

    D3D12_RASTERIZER_DESC rasterizer{};
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;

    // 深度は比較のみ（書き込まない）
    D3D12_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = true;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    Microsoft::WRL::ComPtr<IDxcBlob> vs = dx->CompileShader(L"Resources/Shaders/Particle.VS.hlsl", L"vs_6_0");
    Microsoft::WRL::ComPtr<IDxcBlob> ps = dx->CompileShader(L"Resources/Shaders/Particle.PS.hlsl", L"ps_6_0");
    assert(vs && ps);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature_.Get();
    desc.InputLayout = inputLayout;
    desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    desc.BlendState = blend;
    desc.RasterizerState = rasterizer;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.SampleDesc.Count = 1;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.DepthStencilState = depth;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    HRESULT hr = dx->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline));
    assert(SUCCEEDED(hr));
    return pipeline;
}
