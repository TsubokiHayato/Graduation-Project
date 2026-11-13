#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Object3d.h"
#include "DebugCamera.h"
#include <memory>

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
	bool  showFan_ = true;  float bossYaw_ = 0.0f, fanRadius_ = 6.0f, fanAngle_ = 90.0f; // 度
	bool  showBullet_ = true;  float bulletX_ = 3.0f, bulletZ_ = 3.0f, bulletRadius_ = 0.5f;

	bool hitCircle_ = false, hitBeam_ = false, hitFan_ = false, hitBullet_ = false;
};
