#include "OnoderaScene.h"
#include "Input.h"
#include "TextManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

// =============================================================================
//  Initialize
// =============================================================================
void OnoderaScene::Initialize() {
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->setRotation({ 0.1f, 0.0f, 0.0f });
	camera_->setScale({ 1.0f, 1.0f, 1.0f });
	camera_->Update();

	// プレイヤ―の生成、初期化処理
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_.get());
}

// =============================================================================
//  Update
// =============================================================================
void OnoderaScene::Update() {
	camera_->Update();

	// プレイヤ―更新処理
	player_->Update();

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void OnoderaScene::Object3DDraw() {
	// プレイヤ―描画処理
	player_->Draw();
}

void OnoderaScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void OnoderaScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void OnoderaScene::ImGuiDraw() {
#ifdef USE_IMGUI
	player_->DrawImGui();

#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void OnoderaScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
