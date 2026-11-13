#include "TsubokiScene.h"
#include "Input.h"
#include "Matrix.h"
#include "Object3dCommon.h"
#include "TextManager.h"
#include "LineManager.h"
#include "EditorObjects.h"
#include "SceneSaveData.h"
#include "GameTime.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {

// human モデル（Mixamo のボーン名）
const char* const kHipsJoint = "mixamorig:Hips";
const char* const kLeftFootJoint = "mixamorig:LeftFoot";
const char* const kRightFootJoint = "mixamorig:RightFoot";
// 地面（ground）の高さ
constexpr float kGroundHeight = 0.0f;
// プレイヤー（block.obj は 2x2x2 の箱）の中心から足元までの高さ
constexpr float kPlayerHalfHeight = 1.0f;
// 待機ポーズを作るときに使う walk の位置（周期に対する割合）。ここと半周期後で足が前後対称になる
constexpr float kIdlePhase = 0.27f;

// ジョイントのスケルトン空間での位置
Math::Vector3 JointPosition(const Skeleton& skeleton, const char* name) {
	const Math::Matrix4x4& m = skeleton.joints[skeleton.jointMap.at(name)].skeletonSpaceMatrix;
	return {m.m[3][0], m.m[3][1], m.m[3][2]};
}
float LowerFootHeight(const Skeleton& skeleton) {
	return (std::min)(JointPosition(skeleton, kLeftFootJoint).y, JointPosition(skeleton, kRightFootJoint).y);
}
} // namespace

// =============================================================================
//  Initialize
// =============================================================================
void TsubokiScene::Initialize() {
	// ※ Camera はグローバルにも前方宣言があるため TuboEngine:: を明示する。
	camera_ = std::make_unique<TuboEngine::UniversalCamera>();
	// ImGui で調整して Save したプリセットがあればそれを使う
	if (!camera_->LoadPresets("Resources/Camera/presets.json", /*logIfMissing*/ false)) {
		// 保存が無いときの初期値。エンジンの初期値は身長約2mの人形を肩越しに映す近さなので、
		// 2x2x2 の箱のプレイヤーだと画面が箱で埋まり、背中のノズルと噴射が見えない。引いて広めに映す
		auto& behind = camera_->GetPreset(TuboEngine::UniversalCamera::ViewMode::Behind);
		behind.distance = 16.0f;
		behind.pitchDeg = 22.0f;
		behind.lookHeight = 1.0f;
		behind.fovDeg = 35.0f;
	}

	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj");
	axis_->SetCamera(camera_->GetCamera());
	axis_->SetPosition({0.0f, 0.0f, 0.0f});

	ground_ = std::make_unique<Object3d>();
	ground_->Initialize("tile/tile30x30.obj");
	ground_->SetCamera(camera_->GetCamera());

	// プレイヤー（移動とカメラ追従の対象）
	player_ = std::make_unique<Player>();
	player_->Initialize(camera_->GetCamera());
	// Player.json の初期位置は y=-2 で、このシーンの地面(y=0)に埋まるため、箱(2x2x2)の底が地面に付く高さに置く
	{
		TuboEngine::Object3d* playerObject = player_->GetPlayerObject();
		TuboEngine::Math::Vector3 pos = playerObject->GetPosition();
		pos.y = kGroundHeight + kPlayerHalfHeight;
		playerObject->SetPosition(pos);
	}

	// 人形（アニメーション確認用）。walk.gltf と sneakWalk.gltf はアニメーション名が同じなので名前を付けて登録する
	human_ = std::make_unique<Animator>();
	human_->Initialize("human/walk.gltf");
	human_->SetCamera(camera_->GetCamera());
	human_->SetScale({modelScale_, modelScale_, modelScale_});
	human_->SetPosition({4.0f, 0.0f, 0.0f}); // プレイヤーと重ならない位置（エディタで保存した位置があればそちらを使う）
	human_->AddAnimation("Walk", "human/walk.gltf");
	human_->AddAnimation("SneakWalk", "human/sneakWalk.gltf");
	human_->AddAnimation("Idle", CreateIdlePose());

	// 状態遷移を登録する（どの状態からでも、条件に合う状態へ切り替える）
	using Op = TuboEngine::AnimatorController::Op;
	animController_.AddState("Idle", "Idle");
	animController_.AddState("Walk", "Walk");
	animController_.AddState("SneakWalk", "SneakWalk");
	animController_.AddTransition("*", "Idle", {{"moving", Op::False}});
	animController_.AddTransition("*", "SneakWalk", {{"moving", Op::True}, {"sneak", Op::True}});
	animController_.AddTransition("*", "Walk", {{"moving", Op::True}, {"sneak", Op::False}});
	animController_.SetDefaultState("Idle");

	// エディタで保存した位置・テクスチャなどを反映する（保存が無ければ何もしない。名前は Editor::Inspect と同じ）
	SceneSaveData::Apply("human", human_.get());
	SceneSaveData::Apply("axis", axis_.get());
	SceneSaveData::Apply("ground", ground_.get());

	// プレイヤー背中のバーニアノズル（block は 2x2x2 の箱なので、背面 z=-1 にノズル先端を付ける）
	booster_ = std::make_unique<PlayerBooster>();
	booster_->SetMountOffset(0, { 0.6f, 0.3f, -1.0f}); // 右
	booster_->SetMountOffset(1, {-0.6f, 0.3f, -1.0f}); // 左
	booster_->Initialize(player_->GetPlayerObject(), camera_->GetCamera());
	booster_->SetGroundHeight(kGroundHeight);

	// カメラの追従対象（名前で登録し、ImGui で切り替えられる）
	// player は向きを渡さない：WASD はワールド軸で動くので、カメラが回ると操作方向と画面がずれるため
	// human はアニメーション確認時に寄って見る用、axis はロックオンの練習用
	// カメラのプリセットは human（足元基準・身長約2m）と共用なので、player も足元の位置を渡す。
	// 箱の中心を渡すと注視点が高くなりすぎ、背中のノズルと噴射が画面の下に切れてしまう
	camera_->AddTarget("player", [this] {
		TuboEngine::Math::Vector3 pos = player_->GetPlayerObject()->GetPosition();
		pos.y -= kPlayerHalfHeight;
		return pos;
	});
	camera_->AddTarget("human", [this] { return human_->GetPosition(); }, [this] { return human_->GetRotation().y; });
	camera_->AddTarget("axis", [this] { return axis_->GetPosition(); });
	camera_->SetTarget("player", 0.0f);
	camera_->SetViewMode(TuboEngine::UniversalCamera::ViewMode::Behind, 0.0f);
	camera_->Update(0.0f);
}

TuboEngine::Animation TsubokiScene::CreateIdlePose() const {
	const Animation* walk = human_->FindAnimation("Walk");
	Skeleton skeleton = human_->GetSkeleton(); // 作業用のコピー

	// 歩行中に接地している足の高さ（これに待機ポーズの足の高さを揃える）
	float groundHeight = 1e9f;
	for (int i = 0; i < 30; ++i) {
		ApplyAnimation(skeleton, *walk, walk->duration * i / 30.0f);
		UpdateSkeleton(skeleton);
		groundHeight = (std::min)(groundHeight, LowerFootHeight(skeleton));
	}

	// 逆位相の2フレームを半分ずつ混ぜると、左右対称の立ちポーズになる
	float t0 = walk->duration * kIdlePhase;
	float t1 = std::fmod(t0 + walk->duration * 0.5f, walk->duration);
	ApplyAnimationBlend(skeleton, *walk, t0, *walk, t1, 0.5f);
	UpdateSkeleton(skeleton);

	// 膝が曲がった姿勢の平均なので足が沈む。その分だけ腰を持ち上げる
	float lift = groundHeight - LowerFootHeight(skeleton);
	Joint& hips = skeleton.joints[skeleton.jointMap.at(kHipsJoint)];
	Math::Matrix4x4 toParentLocal = Math::Inverse(skeleton.joints[*hips.parent].skeletonSpaceMatrix);
	hips.transform.translate.x += lift * toParentLocal.m[1][0];
	hips.transform.translate.y += lift * toParentLocal.m[1][1];
	hips.transform.translate.z += lift * toParentLocal.m[1][2];

	return CapturePose(skeleton, "Idle");
}

// =============================================================================
//  Update
// =============================================================================
void TsubokiScene::Update() {
	// プレイヤーの移動（WASD / Space でダッシュ）
	player_->Update();
	// ノズルはプレイヤーを動かした後に更新する
	// 出力はエネルギー配分、ダッシュ開始の瞬間にバーストを出す
	booster_->Update(player_->GetEnergyOutput()->GetBoosterOutputRate(), player_->GetMovement()->IsDashing());
	// プレイヤー本体を移動方向に向ける（ノズルが常に背中側に来るように）
	// Player::Update で行列は更新済みなので、向きを反映するためもう一度更新する
	TuboEngine::Object3d* playerObject = player_->GetPlayerObject();
	TuboEngine::Math::Vector3 playerRotate = playerObject->GetRotation();
	playerRotate.y = booster_->GetYaw();
	playerObject->SetRotation(playerRotate);
	playerObject->Update();

	UpdateHumanPreview();

	// Q でロックオン切り替え（相手は axis）
	if (Input::GetInstance()->TriggerKey(DIK_Q)) {
		if (camera_->IsLockedOn()) {
			camera_->ClearLockOn();
		} else {
			camera_->LockOn("axis");
		}
	}
	// カメラはキャラクターを動かした後、オブジェクトの Update の前に更新する
	camera_->Update(TuboEngine::Time::DeltaTime());
	axis_->Update();
	ground_->Update();
	human_->Update(TuboEngine::Time::DeltaTime());

	// ボーンのワールド座標の確認用（ImGui でオン）
	if (showHandMarker_) {
		LineManager::GetInstance()->DrawSphere(human_->GetJointWorldPosition("mixamorig:RightHand"), 0.08f, {1.0f, 0.3f, 0.3f, 1.0f}, 12);
	}

	TextManager::GetInstance()->UpdateAll();
}

void TsubokiScene::UpdateHumanPreview() {
	// 人形は移動させず、ImGui のチェックで状態を切り替えてアニメーションを確認する
	// 状態の切り替えは AnimatorController に任せる（条件は Initialize で登録）
	animController_.SetBool("moving", previewMoving_);
	animController_.SetBool("sneak", previewSneak_);
	animController_.SetDefaultBlendTime(blendTime_);
	animController_.Update(*human_);

	human_->SetScale({modelScale_, modelScale_, modelScale_});
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void TsubokiScene::Object3DDraw() {
	ground_->Draw();
	axis_->Draw();
	// プレイヤーとノズルは通常のPSOで描くので、スキニング用に切り替える前に描画する
	player_->Draw();
	booster_->Draw();

	// スキニングするモデルは専用PSOに切り替えてから描画する（以降の3D描画もスキニング用になるので最後に描く）
	TuboEngine::Object3dCommon::GetInstance()->DrawSettingsSkinning();
	human_->Draw();
}

void TsubokiScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void TsubokiScene::ParticleDraw() {
	// ブースターの噴射（描画設定はフレームワーク側で済んでいる）
	booster_->DrawParticles();
}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void TsubokiScene::ImGuiDraw() {
#ifdef USE_IMGUI
	// Hierarchy / Inspector / Timeline に出す（クリックで選択 → Gizmo で移動・キーフレーム作成）
	player_->DrawImGui();
	TuboEngine::Editor::Inspect("human", human_.get());
	camera_->DrawImGui(); // UniversalCamera の操作画面（Hierarchy にも "Camera" が出る）
	TuboEngine::Editor::Inspect("axis", axis_.get());
	TuboEngine::Editor::Inspect("ground", ground_.get());

	axis_->DrawImGui("3D Object : axis");
	human_->DrawImGui("Human");

	ImGui::Begin("Human");
	ImGui::Separator();
	ImGui::Text("Animation Preview  (プレイヤー操作: WASD 移動 / Space ダッシュ / Q ロックオン)");
	ImGui::Text("State: %s", animController_.GetCurrentState().c_str());
	ImGui::Checkbox("Moving", &previewMoving_);
	ImGui::SameLine();
	ImGui::Checkbox("Sneak", &previewSneak_);
	ImGui::Checkbox("右手の位置を表示", &showHandMarker_);
	ImGui::DragFloat("Model Scale", &modelScale_, 0.01f, 0.1f, 10.0f);
	ImGui::DragFloat("Blend Time", &blendTime_, 0.01f, 0.0f, 1.0f);
	ImGui::End();
	animController_.DrawImGui("Human");

	ImGui::Begin("Booster");
	booster_->DrawImGui();
	ImGui::End();
#endif
}

// =============================================================================
//  Finalize
// =============================================================================
void TsubokiScene::Finalize() {
	// TextManager はシングルトンでシーンを越えて生存するため、退場時に片付ける。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
}
