#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Object3d.h"
#include "../../Boss/Boss.h"
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
	TuboEngine::Camera* GetMainCamera() const override { return camera_.get(); }

private:
	std::unique_ptr<TuboEngine::Camera> camera_;

	// 座標軸（動作確認用。不要なら消してOK）
	std::unique_ptr<TuboEngine::Object3d> axis_;

	// ---- ボス ----
	std::unique_ptr<Boss> boss_;
	float stageRadius_ = 20.0f; // ステージ半径（仮）

	// ---- 動作確認用の仮の自機（距離と角度で位置を持つ） ----
	std::unique_ptr<TuboEngine::Object3d> dummyPlayer_;
	float playerDistance_ = 12.0f;    // ボスからの距離
	float playerAngle_ = -1.5707963f; // ボスの周りの角度（ラジアン）。-π/2 でカメラ手前
	float playerRadius_ = 0.5f;       // 自機の大きさ（当たり判定の半径）
	float playerMaxHp_ = 100.0f;
	float playerHp_ = 100.0f;
	float shieldRatio_ = 0.0f;     // シールドの配分（0〜1）
	float maxReduction_ = 0.8f;    // シールド100％時の軽減率（1未満）
	float lastDamage_ = 0.0f;      // 直近に受けたダメージ（軽減後）
};
