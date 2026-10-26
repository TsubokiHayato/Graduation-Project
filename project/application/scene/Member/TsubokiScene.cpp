#include "TsubokiScene.h"
#include "Input.h"
#include "Matrix.h"
#include "Object3dCommon.h"
#include "TextManager.h"
#include "LineManager.h"
#include "EditorObjects.h"
#include "GameTime.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
constexpr float kPi = std::numbers::pi_v<float>;

// human モデル（Mixamo のボーン名）
const char* const kHipsJoint = "mixamorig:Hips";
const char* const kLeftFootJoint = "mixamorig:LeftFoot";
const char* const kRightFootJoint = "mixamorig:RightFoot";
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
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate(cameraOffset_);
	camera_->setRotation({cameraPitch_, 0.0f, 0.0f});
	camera_->setScale({1.0f, 1.0f, 1.0f});
	camera_->Update();

	axis_ = std::make_unique<Object3d>();
	axis_->Initialize("axis/axis.obj");
	axis_->SetCamera(camera_.get());
	axis_->SetPosition({0.0f, 0.0f, 0.0f});

	ground_ = std::make_unique<Object3d>();
	ground_->Initialize("tile/tile30x30.obj");
	ground_->SetCamera(camera_.get());

	// 人型キャラクター。walk.gltf と sneakWalk.gltf はアニメーション名が同じなので名前を付けて登録する
	human_ = std::make_unique<Animator>();
	human_->Initialize("human/walk.gltf");
	human_->SetCamera(camera_.get());
	human_->SetScale({modelScale_, modelScale_, modelScale_});
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
	UpdateCharacter();
	UpdateCamera();

	camera_->Update();
	axis_->Update();
	ground_->Update();
	human_->Update(TuboEngine::Time::DeltaTime());

	// ボーンのワールド座標の確認用（ImGui でオン）
	if (showHandMarker_) {
		LineManager::GetInstance()->DrawSphere(human_->GetJointWorldPosition("mixamorig:RightHand"), 0.08f, {1.0f, 0.3f, 0.3f, 1.0f}, 12);
	}

	TextManager::GetInstance()->UpdateAll();
}

void TsubokiScene::UpdateCharacter() {
	Input* input = Input::GetInstance();

	// 入力（WASD / 十字キー / 左スティック）
	Math::Vector3 move = {0.0f, 0.0f, 0.0f};
	if (input->PushKey(DIK_W) || input->PushKey(DIK_UP)) { move.z += 1.0f; }
	if (input->PushKey(DIK_S) || input->PushKey(DIK_DOWN)) { move.z -= 1.0f; }
	if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) { move.x += 1.0f; }
	if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) { move.x -= 1.0f; }
	// Shift / パッドの A ボタンでしのび足
	bool sneak = input->PushKey(DIK_LSHIFT) || input->PushKey(DIK_RSHIFT);

	XINPUT_STATE pad{};
	if (input->GetJoystickState(0, pad)) {
		float lx = pad.Gamepad.sThumbLX / 32767.0f;
		float ly = pad.Gamepad.sThumbLY / 32767.0f;
		if (std::sqrt(lx * lx + ly * ly) > 0.25f) { // デッドゾーン
			move.x += lx;
			move.z += ly;
		}
		sneak = sneak || (pad.Gamepad.wButtons & XINPUT_GAMEPAD_A);
	}

	float length = std::sqrt(move.x * move.x + move.z * move.z);
	bool isMoving = length > 0.0f;

	if (isMoving) {
		// 斜め移動が速くならないよう正規化
		move.x /= (std::max)(length, 1.0f);
		move.z /= (std::max)(length, 1.0f);
		float speed = sneak ? sneakSpeed_ : walkSpeed_;
		position_.x += move.x * speed * TuboEngine::Time::DeltaTime();
		position_.z += move.z * speed * TuboEngine::Time::DeltaTime();

		// 進行方向へなめらかに向く（最短方向に回る）
		float targetYaw = std::atan2(move.x, move.z);
		float diff = std::remainder(targetYaw - yaw_, 2.0f * kPi);
		yaw_ += diff * (std::min)(turnSpeed_ * TuboEngine::Time::DeltaTime(), 1.0f);
	}

	// 状態の切り替えは AnimatorController に任せる（条件は Initialize で登録）
	animController_.SetBool("moving", isMoving);
	animController_.SetBool("sneak", sneak);
	animController_.SetDefaultBlendTime(blendTime_);
	animController_.Update(*human_);

	human_->SetPosition(position_);
	human_->SetRotation({0.0f, yaw_, 0.0f});
	human_->SetScale({modelScale_, modelScale_, modelScale_});
}

void TsubokiScene::UpdateCamera() {
	camera_->SetTranslate({position_.x + cameraOffset_.x, position_.y + cameraOffset_.y, position_.z + cameraOffset_.z});
	camera_->setRotation({cameraPitch_, 0.0f, 0.0f});
}

// =============================================================================
//  描画フェーズ
// =============================================================================
void TsubokiScene::Object3DDraw() {
	ground_->Draw();
	axis_->Draw();

	// スキニングするモデルは専用PSOに切り替えてから描画する（以降の3D描画もスキニング用になるので最後に描く）
	TuboEngine::Object3dCommon::GetInstance()->DrawSettingsSkinning();
	human_->Draw();
}

void TsubokiScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
}

void TsubokiScene::ParticleDraw() {}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void TsubokiScene::ImGuiDraw() {
#ifdef USE_IMGUI
	// Hierarchy / Inspector / Timeline に出す（クリックで選択 → Gizmo で移動・キーフレーム作成）
	TuboEngine::Editor::Inspect("human", human_.get());
	TuboEngine::Editor::Inspect("axis", axis_.get());
	TuboEngine::Editor::Inspect("ground", ground_.get());

	axis_->DrawImGui("3D Object : axis");
	human_->DrawImGui("Human");

	ImGui::Begin("Human");
	ImGui::Separator();
	ImGui::Text("Controller  (WASD: 移動 / Shift: しのび足)");
	ImGui::Text("State: %s", animController_.GetCurrentState().c_str());
	ImGui::Checkbox("右手の位置を表示", &showHandMarker_);
	ImGui::DragFloat("Model Scale", &modelScale_, 0.01f, 0.1f, 10.0f);
	ImGui::DragFloat("Walk Speed", &walkSpeed_, 0.05f, 0.0f, 10.0f);
	ImGui::DragFloat("Sneak Speed", &sneakSpeed_, 0.05f, 0.0f, 10.0f);
	ImGui::DragFloat("Turn Speed", &turnSpeed_, 0.1f, 0.0f, 30.0f);
	ImGui::DragFloat("Blend Time", &blendTime_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat3("Camera Offset", &cameraOffset_.x, 0.1f);
	ImGui::DragFloat("Camera Pitch", &cameraPitch_, 0.01f);
	ImGui::End();
	animController_.DrawImGui("Human");
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
