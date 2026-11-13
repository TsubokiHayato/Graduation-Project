#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Object3d.h"
#include "DebugCamera.h"
#include <memory>
#include "BossWarning.h"
#include "BossWarningRenderer.h"

// Satou 個人用の作業シーン
class SatouScene : public IScene {
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

	std::unique_ptr<TuboEngine::DebugCamera> debugCamera_;

	// 座標軸（動作確認用。不要なら消してOK）
	std::unique_ptr<TuboEngine::Object3d> axis_;

	// BossHit テスト用
	float playerX_ = 5.0f, playerZ_ = 0.0f, playerRadius_ = 0.5f;

	bool  showCircle_ = true;  float circleRadius_ = 4.0f;
	bool  showBeam_ = true;  float beamAngle_ = 0.0f, beamLength_ = 12.0f, beamWidth_ = 2.0f;
	bool  showFan_ = true;  float bossYaw_ = 0.0f, fanRadius_ = 6.0f;
	bool  showBullet_ = true;  float bulletX_ = 3.0f, bulletZ_ = 3.0f, bulletRadius_ = 0.5f;

	bool hitCircle_ = false, hitBeam_ = false, hitFan_ = false, hitBullet_ = false;

	// 警告表示のテスト用
	BossHit::Warning warning_;
	int   warnType_ = 0;            // 0:円 1:帯 2:扇
	float warnTelegraph_ = 2.0f;    // 広がりきるまでの秒数
	float warnHitSec_ = 0.15f;      // 判定が出ている秒数
	float warnTimeScale_ = 1.0f;    // 速度倍率
	bool  warnStart_ = false;
	bool  warnHitResult_ = false;

	BossHit::WarningRenderer warningRenderer_;

	int warnFanIdx_ = 3; // 初期は 90°
};
