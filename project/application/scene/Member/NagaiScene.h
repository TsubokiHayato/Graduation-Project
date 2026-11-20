#pragma once
#include "IScene.h"
#include "Camera.h"
#include "UniversalCamera.h"
#include "Object3d.h"
#include "Boss/Boss.h"
#include "Player/Player.h"
#include <memory>

// Nagai 個人用の作業シーン
class NagaiScene : public IScene {
public:
	void Initialize() override;
	void Update() override;
	void Finalize() override;
	void Object3DDraw() override;
	void SpriteDraw() override;
	void ImGuiDraw() override;
	void ParticleDraw() override;
	TuboEngine::Camera* GetMainCamera() const override { return camera_->GetCamera(); }

private:
	// カメラのパラメータ設定
	void SetupCamera();

	// ボス → 自機 → カメラ が一直線に並ぶように自機を追従するカメラ
	std::unique_ptr<TuboEngine::UniversalCamera> camera_;

	// 座標軸（動作確認用。不要なら消してOK）
	std::unique_ptr<TuboEngine::Object3d> axis_;

	// ---- ボス ----
	std::unique_ptr<Boss> boss_;
	float stageRadius_ = 20.0f; // ステージ半径（仮）
	float minDistance_ = 2.0f;  // 自機がボスに近づける最小距離（仮）

	// ---- 自機 ----
	std::unique_ptr<Player> player_;
};
