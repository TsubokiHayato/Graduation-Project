#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Object3d.h"
#include <memory>

// Miyamoto 個人用の作業シーン
class MiyamotoScene : public IScene {
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

	std::unique_ptr<TuboEngine::Object3d> boss_;

	std::unique_ptr<TuboEngine::Object3d> terrain_;



	TuboEngine::Math::Vector3 stageSize_ = { 10.0f, 0.1f, 10.0f };

	// player変数
	float angle_ = 0.0f;
	float radius_ = 5.0f;
	TuboEngine::Math::Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	float minRadius_ = 1.0f;
	float maxRadius_ = 10.0f;
};
