#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Object3d.h"
#include <memory>
#include "Player/Player.h"

// Onodera 個人用の作業シーン
class OnoderaScene : public IScene {
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

	// プレイヤ―
	std::unique_ptr<Player> player_;

};