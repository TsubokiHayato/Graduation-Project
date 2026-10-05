#pragma once
#include "IScene.h"

#include "Audio.h"        // 音声（BGM / SE）
#include "Camera.h"       // 3D カメラ
#include "DebugCamera.h"  // F2 で切り替わるデバッグ用フリーカメラ
#include "Object3d.h"     // 3D モデル描画
#include "Sprite.h"       // 2D スプライト描画
#include <memory>
#include <string>

// =============================================================================
//  SampleScene ── このエンジン（TuboEngine / EngineCore）の使い方を一通り学ぶ
//                 ための「教材シーン」。
//
//  ▼ シーンの基本構造（IScene を継承）
//    Initialize()    : 生成・読み込み（最初に1回）
//    Update()        : ロジック更新（毎フレーム）
//    Object3DDraw()  : 3D オブジェクトの描画フェーズ
//    SpriteDraw()    : 2D スプライトの描画フェーズ
//    ParticleDraw()  : パーティクルの描画フェーズ
//    ImGuiDraw()     : デバッグ UI（Debug ビルドのみ呼ばれる）
//    Finalize()      : 後始末
//    GetMainCamera() : このシーンの「主カメラ」をエンジンへ渡す（ライン描画等が参照）
//
//  描画関数が用途ごとに分かれているのは、エンジンが
//    3D → 2D(Sprite) → Line → Particle
//  の順に正しいパイプライン状態を作ってから各フェーズを呼ぶため。
//  「何をどのフェーズで描くか」を意識するのがこのエンジンの作法。
//
//  ▼ 実行中の操作（Debug ビルド）
//    WASD / QE : 操作対象の3Dオブジェクトを移動（Q=上 E=下）
//    F2        : デバッグカメラ ON/OFF（ONの間は右ドラッグ回転・中ドラッグ平行移動・ホイールでズーム）
//    画面右の ImGui ウィンドウから各機能を操作・確認できる。
// =============================================================================
class SampleScene : public IScene {
public:
	void Initialize() override;
	void Update() override;
	void Finalize() override;
	void Object3DDraw() override;
	void SpriteDraw() override;
	void ImGuiDraw() override;
	void ParticleDraw() override;

	// このシーンの主カメラ。エンジン側（LineManager 等）がここから取得する。
	TuboEngine::Camera* GetMainCamera() const override { return camera_.get(); }

private:
	// --- 入力ヘルパ（WASDでオブジェクトを動かす等） ---
	void HandleInput();
	// --- ImGui（学習ガイド＋各機能の操作パネル） ---
	void DrawGuideImGui();

private:
	// ───────── カメラ ─────────
	std::unique_ptr<TuboEngine::Camera> camera_;           // 主カメラ
	std::unique_ptr<TuboEngine::DebugCamera> debugCamera_; // F2 で乗っ取るフリーカメラ

	// ───────── 3D オブジェクト ─────────
	std::unique_ptr<TuboEngine::Object3d> axis_;   // 操作対象（座標軸モデル。向きが分かりやすい）
	std::unique_ptr<TuboEngine::Object3d> sphere_; // ライティング確認用の球

	// ───────── 2D スプライト ─────────
	std::unique_ptr<TuboEngine::Sprite> sprite_; // UV チェッカー画像

	// ───────── 音声 ─────────
	std::unique_ptr<TuboEngine::Audio> bgm_; // BGM（ループ再生のサンプル）

	// ───────── パーティクル ─────────
	// 生成したエミッターの名前（Finalize で片付けるため保持）。
	std::string particleName_;

	// ───────── 操作用の状態 ─────────
	int lightType_ = 1;          // 球のライト計算方式（0:平行 1:Lambert 2:Blinn-Phong 3:Point 4:Spot）
	float moveSpeed_ = 0.1f;     // WASD 移動速度
	bool bgmPlaying_ = false;    // BGM 再生中フラグ
	float bgmVolume_ = 0.5f;     // BGM 音量
	bool showGrid_ = true;       // 地面グリッド表示
	bool showHelpers_ = true;    // 座標軸ライン・球ワイヤーの表示
};
