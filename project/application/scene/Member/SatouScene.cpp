#include "SatouScene.h"
#include "Input.h"
#include "TextManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// =============================================================================
//  Initialize
// =============================================================================
void SatouScene::Initialize() {
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({0.0f, 2.0f, -15.0f});
	camera_->setRotation({0.1f, 0.0f, 0.0f});
	camera_->setScale({1.0f, 1.0f, 1.0f});
	camera_->Update();

	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj");
	axis_->SetCamera(camera_.get());
	axis_->SetPosition({0.0f, 0.0f, 0.0f});
}

// =============================================================================
//  Update
// =============================================================================
void SatouScene::Update() {
	camera_->Update();
	axis_->Update();

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void SatouScene::Object3DDraw() {
	axis_->Draw();
}

void SatouScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void SatouScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void SatouScene::ImGuiDraw() {
#ifdef USE_IMGUI
	axis_->DrawImGui("3D Object : axis");
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void SatouScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
