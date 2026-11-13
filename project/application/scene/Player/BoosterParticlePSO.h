#pragma once
#include "DirectXCommon.h"

// ブースター噴射用のパーティクルPSO（通常のアルファ合成 / 加算合成）
//
// エンジンの ParticlePSO はアルファ値の合成が SrcBlendAlpha=ONE / DestBlendAlpha=ZERO で、
// 粒子を描いた場所の描画先のαが粒子のα（縁なら0.1など）で上書きされる。
// 全画面表示ではバックバッファのαは無視されるので問題ないが、エディタの Game ウィンドウは
// その結果を ImGui でα合成して表示するため、粒子の所だけ半透明になり黒く抜けて見える。
// ここではどちらのモードも「描画先のαはそのまま（SrcBlendAlpha=ZERO / DestBlendAlpha=ONE）」にする。
class BoosterParticlePSO {
public:
    // 初期化（シェーダーのコンパイルとPSO生成）
    void Initialize();
    // 通常のアルファ合成に切り替える（煙・炎・火花など、色で見せる層）
    void SetAlphaBlend(ID3D12GraphicsCommandList* cmd);
    // 加算合成に切り替える（芯・発光など、光らせる層）
    void SetAdditive(ID3D12GraphicsCommandList* cmd);

private:
    void CreateRootSignature();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipeline(D3D12_BLEND srcBlend, D3D12_BLEND destBlend);
    void Set(ID3D12GraphicsCommandList* cmd, ID3D12PipelineState* pipeline);

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> alphaBlendPipeline_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> additivePipeline_;
};
