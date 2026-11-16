#include "OnoderaScene.h"
#include "Input.h"
#include "TextManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
#include "Player/Equipment/EquipmentManager.h"

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

	// 装備データ読み込み
	equipmentDataJson_ = std::make_unique<EquipmentDataJson>();
	equipmentDataJson_->Load(equipmentData_);
	// EquipmentManager初期化
	EquipmentManager::GetInstance()->Initialize(equipmentData_, equipmentDataJson_.get(), camera_.get());
}

// =============================================================================
//  Update
// =============================================================================
void OnoderaScene::Update() {
	camera_->Update();

	// 装備管理更新処理
	EquipmentManager::GetInstance()->Update(player_->GetEnergyOutput()->GetShieldOutputRate());

	// プレイヤ―更新処理
	player_->Update();

	TextManager::GetInstance()->UpdateAll();
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void OnoderaScene::Object3DDraw() {
	// 装備管理描画
	EquipmentManager::GetInstance()->Draw();

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
	//シールドオブジェクトの設定
	EquipmentManager::GetInstance()->GetShield()->GetShieldObject()->DrawImGui("Shield Object");

	if (ImGui::Begin("Equipment")) {
		EquipmentManager::GetInstance()->DrawImGui();
		// 動作確認用のダメージボタン
		if (ImGui::Button("Test Damage (50)")) {
			EquipmentManager::GetInstance()->TakeDamage(50.0f, *player_->GetHP(), player_->GetState());
		}
	}
	ImGui::End();

#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void OnoderaScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
	// EquipmentManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	EquipmentManager::GetInstance()->Finalize();
}
