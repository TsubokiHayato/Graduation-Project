#pragma once
#include "Boss/Boss.h"
#include "BossTimelineEditor.h"
#include "Camera.h"
#include "IScene.h"
#include "Object3d.h"
#include <memory>

// =============================================================================
//  BossEditScene
//  ボスの攻撃パターンをタイムラインで作るシーン。
//  中央にボス、的（自機の代わり）を置き、作ったパターンをその場で再生・好きな時刻へ移動して確かめる。
//  編集画面は BossTimelineEditor（Boss Edit / Boss Timeline / Clip Inspector の3つのウィンドウ）。
// =============================================================================
class BossEditScene : public IScene {
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
	// 見下ろしのカメラの位置を反映
	void ApplyCamera();

	std::unique_ptr<TuboEngine::Camera> camera_;
	float cameraDistance_ = 34.0f; // ステージ中央からの距離
	float cameraPitchDeg_ = 55.0f; // 見下ろす角度

	std::unique_ptr<Boss> boss_;
	float stageRadius_ = 20.0f;

	// 的（自機の代わり）の見た目
	std::unique_ptr<TuboEngine::Object3d> target_;

	BossTimelineEditor editor_;
};
