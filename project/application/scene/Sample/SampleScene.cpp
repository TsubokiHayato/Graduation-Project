#include "SampleScene.h"
#include "GameScenes.h"
#include "SceneManager.h"     // シーン遷移（ChangeScene）

#include "Input.h"            // キーボード / マウス / ゲームパッド入力（エンジンが毎フレーム自動 Update）
#include "IParticleEmitter.h" // ParticlePreset 構造体
#include "LineManager.h"      // デバッグ用ライン / グリッド / 球（シングルトン。Update/Draw はエンジンが自動）
#include "ParticleManager.h"  // パーティクル（シングルトン。Update/Draw はシーンが駆動する）
#include "TextManager.h"      // テキスト管理（シングルトン。Update/Draw はシーンが駆動する）

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// このシーンで使うエンジンの型は基本 TuboEngine 名前空間にある。
using namespace TuboEngine;

// =============================================================================
//  Initialize ── 生成・読み込み（最初に1回だけ呼ばれる）
// =============================================================================
void SampleScene::Initialize() {
	// ───────────────────────────────────────────────────────────
	//  ① カメラ ── 3D 描画には最低1台必要。
	//     投影パラメータ(fovY/aspect/near/far)も scale も、コンストラクタで
	//     既定設定済み（scale は {1,1,1}）。下では分かりやすさのため明示している。
	//     ※ 別シーンへ移るときは SceneManager::GetInstance()->ChangeScene(番号) を呼ぶ。
	// ───────────────────────────────────────────────────────────
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({0.0f, 2.0f, -15.0f}); // 少し上・後ろから
	camera_->setRotation({0.1f, 0.0f, 0.0f});    // 少し見下ろす
	camera_->setScale({1.0f, 1.0f, 1.0f});
	camera_->Update();

	// デバッグカメラ：F2 で ON/OFF。ON の間は主カメラを「乗っ取って」自由に動かせる。
	debugCamera_ = std::make_unique<DebugCamera>();

	// ───────────────────────────────────────────────────────────
	//  ② 3D オブジェクト ── Object3d::Initialize(モデルパス) だけで
	//     ModelManager によるモデル読み込みまで内部で行われる（事前ロード不要）。
	//     パスは "Resources/Models/" からの相対。
	//     生成後に SetCamera() で「どのカメラで見るか」を必ず渡す。
	// ───────────────────────────────────────────────────────────
	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj"); // 座標軸モデル：向き(XYZ)が一目で分かる
	axis_->SetCamera(camera_.get());
	axis_->SetPosition({0.0f, 0.0f, 0.0f});

	sphere_ = std::make_unique<Object3d>();
	sphere_->Initialize("sphere/sphere.obj"); // ライティングの効果を見るための球
	sphere_->SetCamera(camera_.get());
	sphere_->SetPosition({3.5f, 0.0f, 0.0f});
	sphere_->SetLightType(lightType_); // ライト計算方式（後で ImGui から切り替え）

	// ───────────────────────────────────────────────────────────
	//  ③ 2D スプライト ── Sprite::Initialize(テクスチャパス)。
	//     パスは "Resources/Textures/" からの相対。座標は画面ピクセル(左上原点)。
	// ───────────────────────────────────────────────────────────
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("uvChecker.png");
	sprite_->SetPosition({40.0f, 40.0f});
	sprite_->SetSize({160.0f, 160.0f});

	// ───────────────────────────────────────────────────────────
	//  ④ 音声 ── Audio::Initialize(ファイル名)。"Resources/Audio/" から読む。
	//     Play(loop, volume) で再生（再生操作は ImGui ボタンから行う）。
	// ───────────────────────────────────────────────────────────
	bgm_ = std::make_unique<Audio>();
	bgm_->Initialize("test.wav");

	// ───────────────────────────────────────────────────────────
	//  ⑤ パーティクル ── ParticleManager（シングルトン）に「プリセット」を渡して
	//     エミッターを生成する。autoEmit=true で毎フレーム自動放出。
	//     登録済みの型名: "Default" "Primitive" "Ring" "Cylinder" "Original" "OrbitTrail"
	// ───────────────────────────────────────────────────────────
	ParticlePreset preset;
	preset.name = "SampleParticles";
	preset.texture = "particle.png"; // "Resources/Textures/" から
	preset.maxInstances = 256;
	preset.billboard = true;         // 常にカメラを向く板ポリ
	preset.autoEmit = true;          // 自動で出し続ける
	preset.emitRate = 40.0f;         // 1秒あたりの放出数
	preset.center = {-3.5f, 0.0f, 0.0f};
	preset.posMin = {-0.3f, 0.0f, -0.3f};
	preset.posMax = {0.3f, 0.0f, 0.3f};
	preset.velMin = {-0.3f, 1.2f, -0.3f};
	preset.velMax = {0.3f, 2.2f, 0.3f};
	preset.lifeMin = 0.6f;
	preset.lifeMax = 1.4f;
	preset.gravity = {0.0f, 0.2f, 0.0f};
	preset.scaleStart = {0.4f, 0.4f, 0.4f};
	preset.scaleEnd = {0.05f, 0.05f, 0.05f};
	preset.colorStart = {0.4f, 0.8f, 1.0f, 1.0f};
	preset.colorEnd = {0.2f, 0.3f, 1.0f, 0.0f}; // だんだん透明に
	if (IParticleEmitter* e = ParticleManager::GetInstance()->CreateEmitterByType("Default", preset)) {
		particleName_ = e->GetName(); // 片付け用に名前を覚えておく
	}

	// ───────────────────────────────────────────────────────────
	//  ⑥ テキスト ── TextManager（シングルトン）。
	//     CreateText で生成したテキストはエンジン終了まで TextManager 側が所有する。
	//     Update/Draw はシーンが駆動する（ParticleManager と同じ作法）。
	// ───────────────────────────────────────────────────────────
	TextManager::GetInstance()->GetOrCreateFontSized(TextManager::PresetFontNames::Best10, 32.0f);
	TextManager::GetInstance()->CreateText(
		TextManager::PresetFontNames::Best10 + "_32",
		"TextManager Sample",
		{40.0f, 220.0f}
	);
}

// =============================================================================
//  Update ── ロジック更新（毎フレーム）
// =============================================================================
void SampleScene::Update() {
	// (1) カメラ更新 → そのあとデバッグカメラを適用する。
	//     DebugCamera は「主カメラを更新した後・描画前」に毎フレーム呼ぶのが約束。
	//     F2 が押されている間だけ主カメラを乗っ取る。
	camera_->Update();
	debugCamera_->Update(camera_.get());

	// (2) 入力でオブジェクトを動かす。
	HandleInput();

	// (3) ライト方式を反映（ImGui で変えた値を毎フレーム適用）。
	sphere_->SetLightType(lightType_);

	// (4) 各オブジェクトの行列を更新（描画前に必須）。
	axis_->Update();
	sphere_->Update();
	sprite_->Update();

	// (5) デバッグライン：原点グリッド・ワールド座標軸・球のワイヤー。
	//     LineManager はシングルトンで、ここで積んだ線をエンジンがまとめて描く。
	if (showGrid_) {
		// DrawGrid(サイズ, 分割数, 回転, 色)
		LineManager::GetInstance()->DrawGrid(20.0f, 20, {0.0f, 0.0f, 0.0f}, {0.35f, 0.35f, 0.35f, 1.0f});
	}
	if (showHelpers_) {
		// DrawLine(始点, 終点, 色) ── ワールドの XYZ 軸を色分け表示
		LineManager::GetInstance()->DrawLine({0, 0, 0}, {5, 0, 0}, {1, 0, 0, 1}); // X:赤
		LineManager::GetInstance()->DrawLine({0, 0, 0}, {0, 5, 0}, {0, 1, 0, 1}); // Y:緑
		LineManager::GetInstance()->DrawLine({0, 0, 0}, {0, 0, 5}, {0, 0, 1, 1}); // Z:青
		// DrawSphere(中心, 半径, 色, 分割数) ── 球の当たり判定っぽい可視化
		LineManager::GetInstance()->DrawSphere(sphere_->GetPosition(), 1.2f, {1.0f, 1.0f, 0.0f, 1.0f}, 16);
	}

	// (6) パーティクル更新。dt（経過時間）とカメラを渡す。
	//     ※ Particle は Line/Input と違い、エンジンが自動更新しない＝シーンが駆動する。
	ParticleManager::GetInstance()->Update(1.0f / 60.0f, camera_.get());

	// (7) TextManager 更新。Particle と同様にシーンが駆動する。
	TextManager::GetInstance()->UpdateAll();
}

// 入力処理：WASD/QE で操作対象(axis_)を動かす。
void SampleScene::HandleInput() {
	Input* input = Input::GetInstance();
	Math::Vector3 pos = axis_->GetPosition();

	// PushKey(DIK_*) は「押されている間 true」。DIK_* は dinput.h のキーコード。
	if (input->PushKey(DIK_D)) pos.x += moveSpeed_;
	if (input->PushKey(DIK_A)) pos.x -= moveSpeed_;
	if (input->PushKey(DIK_W)) pos.z += moveSpeed_; // 奥(+Z)へ
	if (input->PushKey(DIK_S)) pos.z -= moveSpeed_;
	if (input->PushKey(DIK_E)) pos.y -= moveSpeed_;
	if (input->PushKey(DIK_Q)) pos.y += moveSpeed_;

	axis_->SetPosition(pos);
}

// =============================================================================
//  描画フェーズ ── エンジンが種類ごとに適切な順序で呼ぶ。各 Draw() を呼ぶだけ。
// =============================================================================
void SampleScene::Object3DDraw() {
	axis_->Draw();
	sphere_->Draw();
}

void SampleScene::SpriteDraw() {
	sprite_->Draw();

	// TextManager が持つテキストの描画。Sprite と同じパイプライン状態で描く。
	TextManager::GetInstance()->DrawAll();
}

void SampleScene::ParticleDraw() {
	// パーティクルの実描画。Update と対になっている。
	ParticleManager::GetInstance()->Draw();
}

// =============================================================================
//  ImGuiDraw ── デバッグ UI。Debug ビルド(USE_IMGUI)でのみエンジンから呼ばれる。
// =============================================================================
void SampleScene::ImGuiDraw() {
#ifdef USE_IMGUI
	// 学習ガイド＆このシーン専用の操作パネル
	DrawGuideImGui();

	// 各オブジェクトは「自分専用の操作ウィンドウ」を持っている（位置・色・ライト等）。
	// これらを呼ぶだけで Transform やマテリアルを GUI から触れる。
	axis_->DrawImGui("3D Object : axis");
	sphere_->DrawImGui("3D Object : sphere");
	sprite_->DrawImGui("Sprite : uvChecker");

	// デバッグカメラ・ライン・パーティクルにも統合 GUI がある。
	debugCamera_->DrawImGui();
	LineManager::GetInstance()->DrawImGui();
	ParticleManager::GetInstance()->DrawImGui(); // パーティクルの本格エディタ（生成/保存/プレビュー）
	TextManager::GetInstance()->DrawImGui();     // テキストの本格エディタ（生成/JSONレイアウト保存読込）
#endif
}

// このシーン用のガイド＋操作 UI 本体。
void SampleScene::DrawGuideImGui() {
#ifdef USE_IMGUI
	if (ImGuiManager::GetInstance()->BeginPanel("エンジンガイド")) {

		ImGui::TextWrapped("このシーンは TuboEngine の使い方を一通り体験できる教材です。");
		ImGui::Separator();

		// --- シーン遷移テスト ---------------------------------------------------
		//  遷移APIは SceneManager::GetInstance()->ChangeScene(番号) の一本だけ。
		//  押すと「次のフレーム」で切り替わる（その場では差し替わらないので安全）。
		if (ImGui::CollapsingHeader("シーン遷移テスト（ChangeScene）", ImGuiTreeNodeFlags_DefaultOpen)) {
			SceneManager* sm = SceneManager::GetInstance();
			if (ImGui::Button("Title へ")) {
				sm->ChangeScene(TITLE);
			}
			ImGui::SameLine();
			if (ImGui::Button("Option へ")) {
				sm->ChangeScene(OPTION);
			}

			ImGui::Spacing();
			// 同じ番号を渡すとリロード（Finalize → Initialize）。生成し直しの確認に。
			if (ImGui::Button("このシーンをリロード")) {
				sm->ChangeScene(SAMPLE);
			}
		}

		if (ImGui::CollapsingHeader("操作方法", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BulletText("WASD / Q,E : 座標軸オブジェクトを移動");
			ImGui::BulletText("F2 : デバッグカメラ ON/OFF");
			ImGui::BulletText("デバッグカメラ ON 中 → 右ドラッグ回転 / 中ドラッグ平行移動 / ホイールでズーム");
		}

		if (ImGui::CollapsingHeader("ライブ操作", ImGuiTreeNodeFlags_DefaultOpen)) {
			// --- 3D オブジェクトの移動速度 ---
			ImGui::SliderFloat("Move Speed", &moveSpeed_, 0.01f, 0.5f);

			// --- 球のライト計算方式 ---
			const char* lightNames[] = {"0:Directional(平行光源)", "1:Lambert", "2:Blinn-Phong", "3:PointLight", "4:SpotLight"};
			ImGui::Combo("Sphere Light", &lightType_, lightNames, IM_ARRAYSIZE(lightNames));

			// --- デバッグ表示トグル ---
			ImGui::Checkbox("Grid", &showGrid_);
			ImGui::SameLine();
			ImGui::Checkbox("Axis/Sphere Helpers", &showHelpers_);

			// --- 音声操作 ---
			ImGui::Separator();
			ImGui::Text("Audio (test.wav)");
			if (ImGui::Button(bgmPlaying_ ? "Stop" : "Play(loop)")) {
				if (bgmPlaying_) {
					bgm_->Stop();
				} else {
					bgm_->Play(true, bgmVolume_); // loop=true
				}
				bgmPlaying_ = !bgmPlaying_;
			}
			if (ImGui::SliderFloat("Volume", &bgmVolume_, 0.0f, 1.0f)) {
				bgm_->SetVolume(bgmVolume_);
			}
		}

		if (ImGui::CollapsingHeader("入力の状態 (Input)")) {
			Input* input = Input::GetInstance();
			const Math::Vector2& mp = input->GetMousePosition();
			ImGui::Text("Mouse Pos : (%.0f, %.0f)", mp.x, mp.y);
			ImGui::Text("Mouse L/R/M : %d / %d / %d", input->IsPressMouse(0), input->IsPressMouse(1), input->IsPressMouse(2));
			ImGui::Text("Wheel : %d", input->GetWheel());
			ImGui::Text("Keys W/A/S/D : %d/%d/%d/%d", input->PushKey(DIK_W), input->PushKey(DIK_A), input->PushKey(DIK_S), input->PushKey(DIK_D));
		}

		if (ImGui::CollapsingHeader("このエンジンの主な機能（学習メモ）")) {
			ImGui::TextWrapped("[このシーンで実演中]");
			ImGui::BulletText("Camera / DebugCamera : 3Dカメラと F2 フリーカメラ");
			ImGui::BulletText("Object3d + ModelManager : .obj/.gltf モデル描画とライティング");
			ImGui::BulletText("Sprite + TextureManager : 2D 画像描画");
			ImGui::BulletText("Audio : WAV の再生/停止/音量");
			ImGui::BulletText("LineManager : グリッド・線・球のデバッグ描画");
			ImGui::BulletText("ParticleManager : プリセット駆動のパーティクル");
			ImGui::BulletText("Input : キー/マウス/パッド");
			ImGui::Spacing();
			ImGui::TextWrapped("[さらに用意されている機能（各クラス参照）]");
			ImGui::BulletText("PostEffectManager : Bloom/Outline/Sepia/VHS 等の画面効果");
			ImGui::BulletText("SkyBox : キューブマップ天球");
			ImGui::BulletText("Animation / Animator : gltf スケルトンアニメ（AnimatedCube.gltf）");
			ImGui::BulletText("SphSimulator : GPU パーティクル流体シミュ");
			ImGui::BulletText("Collider / CollisionManager : 当たり判定");
			ImGui::BulletText("BehaviorTree : AI のビヘイビアツリー");
			ImGui::BulletText("Framework::SetBlendMode : 加算/減算などの合成切替");
		}
		
	}
ImGuiManager::GetInstance()->EndPanel();
#endif
}


// =============================================================================
//  Finalize ── 後始末。再入場時に二重生成しないようパーティクルも片付ける。
// =============================================================================
void SampleScene::Finalize() {
	if (bgm_ && bgmPlaying_) {
		bgm_->Stop();
	}
	if (!particleName_.empty()) {
		ParticleManager::GetInstance()->Remove(particleName_);
	}

	// TextManager はシングルトンでシーンを越えて生存するため、
	// このシーンで作ったテキストは退場時に必ず片付ける。
	TextManager::GetInstance()->ClearAllTexts();
}
