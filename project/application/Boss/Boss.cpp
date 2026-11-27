#include "Boss.h"
#include "BossAttackLockOnBeam.h"
#include "BossAttackOmniBomb.h"
#include "BossAttackPattern.h"
#include "BossAttackRegistry.h"
#include "BossAttackState.h"
#include "BossAttackSweep.h"
#include "Camera.h"
#include "LineManager.h"
#include "Model.h"
#include "ModelManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kDegToRad = kPi / 180.0f;

// 角度の差を -π〜π に収める
float WrapAngle(float a) { return std::remainder(a, 2.0f * kPi); }

Math::Vector4 LerpColor(const Math::Vector4& a, const Math::Vector4& b, float t) {
	return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

const Math::Vector4 kColorNormal = {1.0f, 1.0f, 1.0f, 1.0f};
const Math::Vector4 kColorDefeated = {0.2f, 0.2f, 0.2f, 1.0f};   // 撃破後
const Math::Vector4 kColorCore = {0.2f, 1.0f, 1.0f, 1.0f};       // コアの向きの表示


// 保存する小数を小数点以下4桁に丸める（float の誤差で 0.800000011920929 のようにならないように）
void RoundFloats(JsonManager::Json& json) {
	if (json.is_number_float()) {
		json = std::round(json.get<double>() * 10000.0) / 10000.0;
	} else if (json.is_object() || json.is_array()) {
		for (JsonManager::Json& child : json) {
			RoundFloats(child);
		}
	}
}
} // namespace

Boss::Boss() = default;
Boss::~Boss() = default;

// =============================================================================
//  Initialize
// =============================================================================
void Boss::Initialize(TuboEngine::Camera* camera, const Math::Vector3& center) {
	// 見た目（モデルは形態の見た目で差し替える）
	object_ = std::make_unique<Object3d>();
	object_->Initialize("boss_block/boss_block.obj");
	object_->SetCamera(camera);
	object_->SetScale({1.0f, 1.0f, 1.0f});
	appliedModel_ = "boss_block/boss_block.obj";
	appliedTexture_.clear();

	// 背中のコア（モデルは形態の見た目で差し替える）
	core_ = std::make_unique<Object3d>();
	core_->Initialize("boss_core/boss_core.obj");
	core_->SetCamera(camera);
	appliedCoreModel_ = "boss_core/boss_core.obj";

	center_ = center;
	yaw_ = 0.0f;

	// 攻撃・形態のデータを読み込んで戦闘を始める（ファイルが無ければコードの初期値）
	LoadData();
}

// =============================================================================
//  戦闘のやり直し
// =============================================================================
void Boss::Restart() {
	CancelPattern();
	requestedPatterns_.clear();

	phaseIndex_ = 0;
	hp_ = data_.common.maxHp;
	height_ = 0.0f;
	color_ = kColorNormal;
	bodyControlEnabled_ = true;

	ResetPhaseRuntime();
	ApplyLook(GetPhase().look);
	EnterIdle(data_.common.firstDelay);
	animationKeyState_ = -1;
	animationKeyAttack_ = nullptr;
	animationKeyStep_ = -1;
	ApplyObjectTransform();
}

// =============================================================================
//  Update
// =============================================================================
void Boss::Update(float deltaTime) {
	const float dt = dePaused_ ? 0.0f : deltaTime * speedScale_;

	// 移行演出中でなければ、形態の見た目は ImGui での調整もすぐ反映する
	if (state_ != State::Transition) {
		ApplyLook(GetPhase().look);
	}

	Simulate(dt);

	// HP 以外のきっかけ（今後足す部位破壊など）もここで毎フレーム調べる
	CheckPhaseTriggers();

	// 状態に合ったアニメーションの指示
	UpdateAnimationRequest();

	if (deDrawLines_) {
		// 攻撃範囲のライン描画
		if (currentPattern_) {
			currentPattern_->DrawLines(*this);
		}
		if (state_ != State::Defeated && look_.core.showArcLines) {
			DrawCoreLines();
		}
	}

	// 移動、色の適用
	ApplyObjectTransform();

	// オブジェクトの更新
	object_->Update();
	core_->Update();
}

void Boss::Draw() {

	object_->Draw();

	if (look_.core.visible) {
		core_->Draw();
	}
}

// =============================================================================
//  行動サイクル
// =============================================================================
void Boss::Simulate(float deltaTime) {
	switch (state_) {
	case State::Idle:       UpdateIdle(deltaTime);       break;
	case State::Acting:     UpdateActing(deltaTime);     break;
	case State::Transition: UpdateTransition(deltaTime); break;
	case State::Defeated:   UpdateDefeated(deltaTime);   break;
	}
}

void Boss::EnterIdle(float waitTime) {
	state_ = State::Idle;
	stateTimer_ = 0.0f;
	idleWait_ = waitTime;
	currentPattern_ = nullptr;
}

// -----------------------------------------------------------------------------
//  待機：自機の方へ向き直り、待機時間が過ぎて向きがそろったら（または待ちすぎたら）次の攻撃へ
// -----------------------------------------------------------------------------
void Boss::UpdateIdle(float deltaTime) {
	stateTimer_ += deltaTime;
	TurnTowardTarget(deltaTime);

	if (stateTimer_ < idleWait_) {
		return;
	}

	const float targetAngle = std::atan2(targetPosition_.z - center_.z, targetPosition_.x - center_.x);
	const bool facing = std::abs(WrapAngle(targetAngle - GetFacingAngle())) <= data_.common.faceTolDeg * kDegToRad;
	const bool waitedTooLong = stateTimer_ >= idleWait_ + data_.common.faceMaxWait;
	if (!facing && !waitedTooLong) {
		return;
	}

	if (BossAttackPattern* next = SelectNextPattern()) {
		StartPattern(next);
	}
}

void Boss::UpdateActing(float deltaTime) {
	if (!currentPattern_) {
		EnterIdle(GetPhase().idleTime);
		return;
	}
	currentPattern_->Update(*this, deltaTime);
	if (currentPattern_->IsFinished()) {
		height_ = 0.0f;
		color_ = kColorNormal;
		EnterIdle(GetPhase().idleTime);
	}
}

// -----------------------------------------------------------------------------
//  移行演出：浮き上がって光り、途中で見た目を切り替え、降りてから次の形態の待機へ
// -----------------------------------------------------------------------------
void Boss::UpdateTransition(float deltaTime) {
	stateTimer_ += deltaTime;
	const BossTransitionData& tr = GetPhase().transition;
	const float duration = GetPhase().transitionTime;
	const float t = duration > 0.0f ? std::clamp(stateTimer_ / duration, 0.0f, 1.0f) : 1.0f;

	// 決めた時点を過ぎたら新しい形態の見た目に切り替える
	if (!transitionLookApplied_ && t >= tr.lookSwitchRatio) {
		transitionLookApplied_ = true;
		ApplyLook(GetPhase().look);
	}

	// 浮き上がって戻る。点滅はだんだん速くなる
	height_ = tr.riseHeight * std::sin(kPi * t);
	const float hz = tr.blinkStartHz + (tr.blinkEndHz - tr.blinkStartHz) * t;
	const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * hz * stateTimer_);
	color_ = LerpColor(kColorNormal, tr.glowColor, blink * std::sin(kPi * t));

	if (t >= 1.0f) {
		height_ = 0.0f;
		color_ = kColorNormal;
		ApplyLook(GetPhase().look);
		EmitCue(Cue::Type::TransitionEnd, tr.endCue, phaseIndex_);
		EnterIdle(GetPhase().idleTime);
	}
}

void Boss::UpdateDefeated(float deltaTime) {
	stateTimer_ += deltaTime;
	// 暗くなって少し沈む
	const float t = std::clamp(stateTimer_ / 1.0f, 0.0f, 1.0f);
	height_ = -0.5f * t;
	color_ = LerpColor(kColorNormal, kColorDefeated, t);
}

// =============================================================================
//  攻撃の選択
// =============================================================================
BossAttackPattern* Boss::SelectNextPattern() {
	// 1. 外から予約された攻撃（予約順）。見つからない名前は飛ばす
	while (!requestedPatterns_.empty()) {
		const std::string name = requestedPatterns_.front();
		requestedPatterns_.pop_front();
		BossAttackPattern* pattern = FindPattern(name);
		if (pattern && pattern->IsValid()) {
			return pattern;
		}
	}

	// 2. デバッグで固定した攻撃
	if (!deForcedPattern_.empty()) {
		BossAttackPattern* pattern = FindPattern(deForcedPattern_);
		if (pattern && pattern->IsValid()) {
			return pattern;
		}
	}

	// 3. 自分では選ばない設定なら、予約が来るまで待機を続ける
	if (!autoAttack_) {
		return nullptr;
	}

	return SelectFromPhase();
}

BossAttackPattern* Boss::SelectFromPhase() {
	const std::vector<BossPatternEntry>& entries = GetPhase().patterns;
	if (cooldowns_.size() != entries.size()) {
		cooldowns_.assign(entries.size(), 0);
	}

	// 使える候補（パターンがあり、重みが0より大きい）
	std::vector<size_t> usable;
	for (size_t i = 0; i < entries.size(); ++i) {
		BossAttackPattern* pattern = FindPattern(entries[i].pattern);
		if (pattern && pattern->IsValid() && entries[i].weight > 0.0f) {
			usable.push_back(i);
		}
	}
	if (usable.empty()) {
		return nullptr;
	}

	// 再使用待ちでない候補。すべて再使用待ちなら、残り回数がいちばん少ないものを候補にする
	std::vector<size_t> candidates;
	for (size_t i : usable) {
		if (cooldowns_[i] <= 0) {
			candidates.push_back(i);
		}
	}
	if (candidates.empty()) {
		int minRemain = cooldowns_[usable.front()];
		for (size_t i : usable) {
			minRemain = (std::min)(minRemain, cooldowns_[i]);
		}
		for (size_t i : usable) {
			if (cooldowns_[i] == minRemain) {
				candidates.push_back(i);
			}
		}
	}

	// 重み付きランダム
	float total = 0.0f;
	for (size_t i : candidates) {
		total += entries[i].weight;
	}
	std::uniform_real_distribution<float> dist(0.0f, total);
	float pick = dist(random_);
	size_t chosen = candidates.back();
	for (size_t i : candidates) {
		pick -= entries[i].weight;
		if (pick <= 0.0f) {
			chosen = i;
			break;
		}
	}

	// 再使用までの回数：選ぶたびに全員1つ減らし、選ばれたものは設定した回数に戻す
	for (int& remain : cooldowns_) {
		remain = (std::max)(remain - 1, 0);
	}
	cooldowns_[chosen] = (std::max)(entries[chosen].cooldown, 0);

	return FindPattern(entries[chosen].pattern);
}

void Boss::StartPattern(BossAttackPattern* pattern) {
	currentPattern_ = pattern;
	state_ = State::Acting;
	stateTimer_ = 0.0f;
	currentPattern_->Enter(*this);
	if (onAttackStarted_) {
		onAttackStarted_(currentPattern_->GetName());
	}
}

void Boss::CancelPattern() {
	if (currentPattern_) {
		currentPattern_->Cancel(*this);
		currentPattern_ = nullptr;
	}
	bodyControlEnabled_ = true;
}

// =============================================================================
//  形態
// =============================================================================
void Boss::CheckPhaseTriggers() {
	if (state_ == State::Transition || state_ == State::Defeated) {
		return;
	}
	const int next = phaseIndex_ + 1;
	if (next >= static_cast<int>(data_.phases.size())) {
		return;
	}
	if (IsTriggerSatisfied(data_.phases[next].trigger)) {
		StartTransition(next);
	}
}

bool Boss::IsTriggerSatisfied(const BossPhaseTrigger& trigger) const {
	if (trigger.type == "HpRatio") {
		return GetHpRatio() <= trigger.value;
	}
	// 部位破壊など、新しいきっかけはここに足す
	return false;
}

float Boss::GetNextPhaseHpFloor() const {
	const int next = phaseIndex_ + 1;
	if (next >= static_cast<int>(data_.phases.size())) {
		return 0.0f;
	}
	const BossPhaseTrigger& trigger = data_.phases[next].trigger;
	if (trigger.type != "HpRatio") {
		return 0.0f;
	}
	return (std::max)(data_.common.maxHp * trigger.value, 0.0f);
}

void Boss::StartTransition(int nextPhase) {
	// 攻撃をすぐに打ち切り、残った弾・攻撃範囲も消す
	CancelPattern();

	phaseIndex_ = nextPhase;
	state_ = State::Transition;
	stateTimer_ = 0.0f;
	transitionLookApplied_ = false;
	ResetPhaseRuntime();

	if (onPhaseChanged_) {
		onPhaseChanged_(phaseIndex_, GetPhase().name);
	}
	EmitCue(Cue::Type::TransitionStart, GetPhase().transition.startCue, phaseIndex_);
}

void Boss::Defeat() {
	CancelPattern();
	state_ = State::Defeated;
	stateTimer_ = 0.0f;
	hp_ = 0.0f;
	if (onDefeated_) {
		onDefeated_();
	}
	EmitCue(Cue::Type::Defeated, data_.common.defeatCue, phaseIndex_);
}

void Boss::ResetPhaseRuntime() {
	cooldowns_.assign(GetPhase().patterns.size(), 0);
}

void Boss::ApplyLook(const BossLookData& look) {
	look_ = look;
	ModelManager* models = ModelManager::GetInstance();

	// モデルの差し替え（変わったときだけ。読み込めなければ今のまま）
	if (!look_.model.empty() && look_.model != appliedModel_) {
		models->LoadModel(look_.model);
		if (models->FindModel(look_.model)) {
			object_->SetModel(look_.model);
			appliedModel_ = look_.model;
		}
	}
	// テクスチャの差し替え（ボスだけに効く。空ならモデルのテクスチャ。読み込めなければ今のまま）
	if (look_.texture != appliedTexture_) {
		if (object_->SetTexture(look_.texture)) {
			appliedTexture_ = look_.texture;
		}
	}
	// 材質（ボス専用のモデルなので、ほかのオブジェクトには影響しない）
	if (Model* model = object_->GetModel()) {
		model->SetModelShininess(look_.shininess);
		model->SetModelEnvironmentCoefficient(look_.environmentCoefficient);
	}

	// コアのモデル
	if (!look_.core.model.empty() && look_.core.model != appliedCoreModel_) {
		models->LoadModel(look_.core.model);
		if (models->FindModel(look_.core.model)) {
			core_->SetModel(look_.core.model);
			appliedCoreModel_ = look_.core.model;
		}
	}
}

// =============================================================================
//  演出の合図・アニメーションの指示
// =============================================================================
void Boss::EmitCue(Cue::Type type, const std::string& name, int phaseIndex) {
	if (onCue_) {
		onCue_({type, name, phaseIndex});
	}
}

void Boss::UpdateAnimationRequest() {
	// いまの状態・ボスを動かしている攻撃・その段階
	const BossAttackState* attack = nullptr;
	int step = -1;
	if (state_ == State::Acting && currentPattern_) {
		attack = currentPattern_->GetCurrentAttack();
		if (attack) {
			step = static_cast<int>(attack->GetStep());
		}
	}
	if (static_cast<int>(state_) == animationKeyState_ && attack == animationKeyAttack_ && step == animationKeyStep_) {
		return;
	}
	animationKeyState_ = static_cast<int>(state_);
	animationKeyAttack_ = attack;
	animationKeyStep_ = step;

	// 形態のアニメーション名の対応から選ぶ
	const BossAnimationMap& map = GetPhase().animations;
	std::string name;
	float duration = 0.0f;
	switch (state_) {
	case State::Idle:
		name = map.idle;
		break;
	case State::Transition:
		name = map.transition;
		duration = GetPhase().transitionTime;
		break;
	case State::Defeated:
		name = map.defeated;
		break;
	case State::Acting:
		if (!attack) {
			name = map.idle; // 攻撃と攻撃の間
			break;
		}
		if (auto it = map.attacks.find(attack->GetId()); it != map.attacks.end()) {
			switch (attack->GetStep()) {
			case BossAttackState::Step::Telegraph:
				name = it->second.telegraph;
				duration = attack->GetTelegraphLength();
				break;
			case BossAttackState::Step::Attack:
				name = it->second.attack;
				duration = attack->GetAttackLength();
				break;
			case BossAttackState::Step::Recovery:
				name = it->second.recovery;
				duration = attack->GetRecoveryLength();
				break;
			default:
				break;
			}
		}
		break;
	}

	currentAnimation_ = name;
	currentAnimationDuration_ = duration;
	if (!name.empty() && onAnimation_) {
		onAnimation_(name, duration);
	}
}

// =============================================================================
//  見た目の確認（BossEdit）
// =============================================================================
void Boss::ForcePhase(int phaseIndex) {
	if (phaseIndex < 0 || phaseIndex >= static_cast<int>(data_.phases.size())) {
		return;
	}
	CancelPattern();
	phaseIndex_ = phaseIndex;
	height_ = 0.0f;
	color_ = kColorNormal;
	ResetPhaseRuntime();
	ApplyLook(GetPhase().look);
	EnterIdle(GetPhase().idleTime);
}

void Boss::PlayTransitionPreview(int phaseIndex) {
	if (phaseIndex < 0 || phaseIndex >= static_cast<int>(data_.phases.size())) {
		return;
	}
	// ひとつ前の形態の見た目から始める（最初の形態なら自分の見た目のまま）
	ForcePhase((std::max)(phaseIndex - 1, 0));
	StartTransition(phaseIndex);
}

// =============================================================================
//  ダメージ
// =============================================================================
Boss::DamageResult Boss::TakeDamage(float damage, const Math::Vector3& attackerPosition) {
	if (IsInvincible() || damage <= 0.0f) {
		return DamageResult::Ignored;
	}
	// 背中のコアの向きからでなければ弾く
	if (!IsInCoreArc(attackerPosition)) {
		return DamageResult::Blocked;
	}

	// 次の形態の閾値より下には一度に削れない（超えた分は持ち越さない）
	const float floorHp = (std::min)(GetNextPhaseHpFloor(), hp_);
	hp_ = (std::max)(hp_ - damage, floorHp);
	hp_ = (std::max)(hp_, 0.0f);

	CheckPhaseTriggers();
	if (hp_ <= 0.0f && state_ != State::Transition) {
		Defeat();
	}
	return DamageResult::Hit;
}

bool Boss::IsInCoreArc(const Math::Vector3& position) const {
	const float angle = std::atan2(position.z - center_.z, position.x - center_.x);
	const float backAngle = GetFacingAngle() + kPi;
	return std::abs(WrapAngle(angle - backAngle)) <= data_.common.coreArcDeg * 0.5f * kDegToRad;
}

// =============================================================================
//  向き
//  角度 θ の方向は (cosθ, 0, sinθ)。Object3d の Y 回転 yaw で前方(+Z)は (sin yaw, 0, cos yaw) を向くので
//  yaw = π/2 − θ で対応させる
// =============================================================================
void Boss::SetFacingAngle(float angle) {
	if (!bodyControlEnabled_) {
		return;
	}
	yaw_ = kPi * 0.5f - angle;
}

float Boss::GetFacingAngle() const {

	return kPi * 0.5f - yaw_;
}

void Boss::TurnTowardTarget(float deltaTime) {
	if (!bodyControlEnabled_) {
		return;
	}

	const float targetAngle = std::atan2(targetPosition_.z - center_.z, targetPosition_.x - center_.x);

	// -π〜π に収めて近い方へ回る
	const float diff = WrapAngle(targetAngle - GetFacingAngle());

	const float maxStep = GetPhase().turnSpeedDeg * kDegToRad * deltaTime;

	SetFacingAngle(GetFacingAngle() + std::clamp(diff, -maxStep, maxStep));

	// 角度が大きくなり続けないように yaw を -π〜π に戻す
	yaw_ = WrapAngle(yaw_);
}

// =============================================================================
//  見た目の反映
// =============================================================================
Math::Vector3 Boss::LocalToWorldOffset(const Math::Vector3& offset) const {
	// Object3d の Y 回転 yaw で、前(+Z) は (sin yaw, 0, cos yaw)、右(+X) は (cos yaw, 0, -sin yaw) を向く
	const Math::Vector3 scaled = {offset.x * look_.scale.x, offset.y * look_.scale.y, offset.z * look_.scale.z};
	const float s = std::sin(yaw_);
	const float c = std::cos(yaw_);
	return {scaled.x * c + scaled.z * s, scaled.y, -scaled.x * s + scaled.z * c};
}

void Boss::ApplyObjectTransform() {

	// 体：位置のずれ（モデルの補正）を足し、向きの補正を加える
	const Math::Vector3 bodyOffset = LocalToWorldOffset(look_.offset);
	object_->SetScale(look_.scale);
	object_->SetPosition({center_.x + bodyOffset.x, center_.y + bodyOffset.y + height_, center_.z + bodyOffset.z});
	object_->SetRotation({look_.rotationDeg.x * kDegToRad, yaw_ + look_.rotationDeg.y * kDegToRad, look_.rotationDeg.z * kDegToRad});

	// 攻撃の演出の色 × 形態の色
	object_->SetModelColor({color_.x * look_.color.x, color_.y * look_.color.y, color_.z * look_.color.z, color_.w * look_.color.w});

	// コア：背中に付いて回る。硬直中は点滅、撃破後は暗くする
	const BossCoreLook& core = look_.core;
	const Math::Vector3 coreOffset = LocalToWorldOffset(core.offset);
	core_->SetScale({core.scale, core.scale, core.scale});
	core_->SetPosition({center_.x + coreOffset.x, center_.y + coreOffset.y + height_, center_.z + coreOffset.z});
	core_->SetRotation({0.0f, yaw_, 0.0f});
	Math::Vector4 coreColor = core.color;
	const BossAttackState* attack = (state_ == State::Acting && currentPattern_) ? currentPattern_->GetCurrentAttack() : nullptr;
	if (attack && attack->GetStep() == BossAttackState::Step::Recovery) {
		const float blink = 0.5f + 0.5f * std::sin(2.0f * kPi * core.blinkHz * attack->GetStepTimer());
		coreColor = LerpColor(core.color, core.recoveryColor, blink);
	} else if (state_ == State::Defeated) {
		coreColor = LerpColor(core.color, kColorDefeated, 0.8f);
	}
	core_->SetModelColor(coreColor);
}

void Boss::DrawCoreLines() const {
	// 背中のコアを狙える範囲の両端を、ボスの後ろに短い線で示す
	const float back = GetFacingAngle() + kPi;
	const float half = data_.common.coreArcDeg * 0.5f * kDegToRad;
	const float r0 = 1.2f * look_.scale.x;
	const float r1 = r0 + 2.5f;
	const float y = center_.y + 0.1f;
	LineManager* lm = LineManager::GetInstance();
	for (float a : {back - half, back + half}) {
		lm->DrawLine({center_.x + std::cos(a) * r0, y, center_.z + std::sin(a) * r0}, {center_.x + std::cos(a) * r1, y, center_.z + std::sin(a) * r1}, kColorCore);
	}
	// 真後ろ（コアの位置）
	lm->DrawLine({center_.x + std::cos(back) * r0, y, center_.z + std::sin(back) * r0}, {center_.x + std::cos(back) * (r0 + 1.0f), y, center_.z + std::sin(back) * (r0 + 1.0f)}, kColorCore);
}

// =============================================================================
//  データ（Resources/Boss.json）
//  { "default": {...}, "current": {...} }。読み込みは current を優先し、無ければ default
// =============================================================================
bool Boss::LoadData() {
	JsonManager::Json file;
	const bool loaded = JsonManager::Load(kBossJsonPath, file, /*logIfMissing*/ false) && file.is_object();

	if (loaded && file.contains("default") && file["default"].is_object()) {
		defaultJson_ = file["default"];
	} else {
		// ファイルが無い・default が無いときは、コードの初期値を default にする
		ApplyJson(JsonManager::Json::object());
		defaultJson_ = BuildJson();
	}

	if (loaded && file.contains("current") && file["current"].is_object()) {
		ApplyJson(file["current"]);
	} else {
		ApplyJson(defaultJson_);
	}

	deDataMessage_ = loaded ? "読み込みました" : "ファイルが無いので初期値を使っています（Save で作成）";
	return loaded;
}

bool Boss::SaveData() {
	JsonManager::Json file;
	file["default"] = defaultJson_;
	file["current"] = BuildJson();
	RoundFloats(file);
	const bool saved = JsonManager::Save(kBossJsonPath, file);
	deDataMessage_ = saved ? "保存しました" : "保存に失敗しました";
	return saved;
}

bool Boss::ResetDataToDefault() {
	ApplyJson(defaultJson_);
	const bool saved = SaveData();
	deDataMessage_ = saved ? "初期値に戻して保存しました" : "保存に失敗しました";
	return saved;
}

void Boss::ApplyJson(const JsonManager::Json& json) {
	// 攻撃中のものを片付けてから、攻撃を作り直す（数値をコードの初期値に戻してから JSON を反映するため）
	CancelPattern();
	previewPattern_.reset();
	attacks_ = BossAttackRegistry::CreateAll();

	BossData data;
	JsonManager::Read(json, "common", data.common);
	JsonManager::Read(json, "patterns", data.patterns);
	JsonManager::Read(json, "phases", data.phases);
	data_ = std::move(data);

	if (json.is_object() && json.contains("attacks") && json["attacks"].is_object()) {
		const JsonManager::Json& attacksJson = json["attacks"];
		for (const std::unique_ptr<BossAttackState>& attack : attacks_) {
			if (attacksJson.contains(attack->GetId())) {
				attack->LoadParams(attacksJson[attack->GetId()]);
			}
		}
	}

	ConvertLegacyPatterns();
	EnsurePhases();
	RebuildPatterns();
	Restart();
}

JsonManager::Json Boss::BuildJson() const {
	JsonManager::Json json;
	json["common"] = data_.common;
	json["patterns"] = data_.patterns;
	json["phases"] = data_.phases;
	JsonManager::Json attacksJson = JsonManager::Json::object();
	for (const std::unique_ptr<BossAttackState>& attack : attacks_) {
		JsonManager::Json params;
		attack->SaveParams(params);
		attacksJson[attack->GetId()] = params;
	}
	json["attacks"] = attacksJson;
	return json;
}

void Boss::EnsurePhases() {
	if (!data_.phases.empty()) {
		return;
	}
	BossPhaseData phase;
	phase.name = "Phase1";
	for (const std::unique_ptr<BossAttackState>& attack : attacks_) {
		phase.patterns.push_back({attack->GetId(), 1.0f, 1});
	}
	data_.phases.push_back(phase);
}

void Boss::RebuildPatterns() {
	patterns_.clear();
	if (!IsPreviewing()) {
		currentPattern_ = nullptr;
	}

	// 攻撃名はそのまま単発のパターンとして使える（硬直はその攻撃の標準の硬直時間）
	for (const std::unique_ptr<BossAttackState>& attack : attacks_) {
		BossPatternData single;
		single.name = attack->GetId();
		single.clips.push_back({attack->GetId(), 0.0f, 0, true, JsonManager::Json::object()});
		patterns_.push_back(std::make_unique<BossAttackPattern>(single, /*useAttackRecovery*/ true));
	}

	// BossEdit シーンで作ったタイムライン
	for (const BossPatternData& data : data_.patterns) {
		const bool duplicated = FindPattern(data.name) != nullptr;
		patterns_.push_back(std::make_unique<BossAttackPattern>(data));
		if (duplicated) {
			patterns_.back()->SetError("同じ名前のパターン・攻撃がすでにあります: " + data.name);
		} else if (data.name.empty()) {
			patterns_.back()->SetError("名前がありません");
		}
	}
}

void Boss::ConvertLegacyPatterns() {
	for (BossPatternData& data : data_.patterns) {
		if (!data.clips.empty() || data.legacyAttacks.empty()) {
			continue;
		}
		// 連続技：前の攻撃の終わり（予兆＋攻撃）に次を置く／同時攻撃：全部 0 秒に段を分けて置き、先頭だけボスを動かす
		const bool parallel = data.legacyType == "Parallel";
		float time = 0.0f;
		float lastRecovery = data.recoveryTime;
		for (size_t i = 0; i < data.legacyAttacks.size(); ++i) {
			BossTimelineClip clip;
			clip.attack = data.legacyAttacks[i];
			clip.start = parallel ? 0.0f : time;
			clip.track = parallel ? static_cast<int>(i) : 0;
			clip.controlsBody = !parallel || i == 0;
			if (BossAttackState* attack = FindAttack(clip.attack)) {
				attack->SetupContext(*this);
				time += attack->GetTelegraphLength() + attack->GetAttackLength();
				if (!parallel || i == 0) {
					lastRecovery = attack->GetRecoveryLength();
				}
			}
			data.clips.push_back(clip);
		}
		data.recoveryTime = lastRecovery;
		data.legacyAttacks.clear();
		data.legacyType.clear();
	}
}

JsonManager::Json Boss::GetAttackBaseParams(const std::string& attackId) const {
	JsonManager::Json params = JsonManager::Json::object();
	if (const BossAttackState* attack = FindAttack(attackId)) {
		attack->SaveParams(params);
	}
	return params;
}

void Boss::ApplyDataChanges() {
	if (!IsPreviewing()) {
		CancelPattern();
		if (state_ == State::Acting) {
			EnterIdle(GetPhase().idleTime);
		}
	}
	RebuildPatterns();
}

// =============================================================================
//  プレビュー
// =============================================================================
void Boss::StartPreview(const BossPatternData& pattern) {
	CancelPattern();
	requestedPatterns_.clear();
	autoAttack_ = false;

	// 移行演出・撃破から戻し、自機の方を向いた状態から始める（毎回同じ結果になるように）
	if (state_ == State::Transition || state_ == State::Defeated) {
		ApplyLook(GetPhase().look);
	}
	height_ = 0.0f;
	color_ = kColorNormal;
	bodyControlEnabled_ = true;
	SetFacingAngle(std::atan2(targetPosition_.z - center_.z, targetPosition_.x - center_.x));

	previewPattern_ = std::make_unique<BossAttackPattern>(pattern);
	StartPattern(previewPattern_.get());
}

void Boss::SeekPreview(const BossPatternData& pattern, float time) {
	StartPreview(pattern);
	constexpr float kStep = 1.0f / 60.0f;
	const int steps = static_cast<int>(std::floor(time / kStep + 0.5f));
	for (int i = 0; i < steps && IsPreviewing(); ++i) {
		Simulate(kStep);
	}
}

void Boss::StopPreview() {
	if (IsPreviewing()) {
		CancelPattern();
		height_ = 0.0f;
		color_ = kColorNormal;
		EnterIdle(0.0f);
	}
}

bool Boss::IsPreviewing() const {
	return previewPattern_ && currentPattern_ == previewPattern_.get();
}

float Boss::GetPreviewTime() const {
	return previewPattern_ ? previewPattern_->GetTime() : 0.0f;
}

BossAttackState* Boss::FindAttack(const std::string& id) const {
	for (const std::unique_ptr<BossAttackState>& attack : attacks_) {
		if (id == attack->GetId()) {
			return attack.get();
		}
	}
	return nullptr;
}

BossAttackPattern* Boss::FindPattern(const std::string& name) const {
	// 同じ名前があれば先に登録したもの（攻撃名の単発）を使う
	for (const std::unique_ptr<BossAttackPattern>& pattern : patterns_) {
		if (pattern->GetName() == name) {
			return pattern.get();
		}
	}
	return nullptr;
}

// =============================================================================
//  ImGui
// =============================================================================
void Boss::DrawImGui() {

#ifdef USE_IMGUI

	object_->DrawImGui("3D Object : Boss");

	if (!ImGui::Begin("Boss")) {

		ImGui::End();

		return;
	}

	DrawImGuiStatus();

	ImGui::Separator();

	DrawImGuiData();

	ImGui::End();

#endif
}

void Boss::DrawImGuiStatus() {
#ifdef USE_IMGUI
	static const char* kStateNames[] = {"Idle (待機)", "Acting (攻撃中)", "Transition (移行演出・無敵)", "Defeated (撃破)"};
	static const char* kStepNames[] = {"Telegraph (予兆)", "Attack (攻撃)", "Recovery (硬直)", "Finished"};

	// ---- 状態 ----
	ImGui::Text("Phase  : %d / %d  %s", phaseIndex_ + 1, static_cast<int>(data_.phases.size()), GetPhase().name.c_str());
	ImGui::Text("State  : %s", kStateNames[static_cast<int>(state_)]);

	const float ratio = GetHpRatio();
	char hpText[64];
	snprintf(hpText, sizeof(hpText), "HP %.0f / %.0f", hp_, data_.common.maxHp);
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.3f, 0.1f, 1.0f));
	ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f), hpText);
	ImGui::PopStyleColor();
	// 次の形態の閾値
	if (phaseIndex_ + 1 >= static_cast<int>(data_.phases.size())) {
		ImGui::Text("次の形態まで : - (最後の形態)");
	} else if (const float floorHp = GetNextPhaseHpFloor(); floorHp > 0.0f) {
		ImGui::Text("次の形態まで : HP %.0f 以下", floorHp);
	} else {
		ImGui::Text("次の形態まで : - (HP 以外のきっかけ)");
	}

	// 攻撃していないときも同じ行数で表示する（行が消えると下の項目の位置がずれて触りにくいため）
	const BossAttackState* attack = currentPattern_ ? currentPattern_->GetCurrentAttack() : nullptr;
	if (currentPattern_) {
		ImGui::Text("Pattern: %s  %.2f", currentPattern_->GetName().c_str(), currentPattern_->GetTime());
	} else {
		ImGui::Text("Pattern: -");
	}
	if (attack) {
		ImGui::Text("Attack : %s", attack->GetName());
		ImGui::Text("Step   : %s  %.2f", kStepNames[static_cast<int>(attack->GetStep())], attack->GetStepTimer());
	} else {
		ImGui::Text("Attack : -");
		ImGui::Text("Step   : -");
	}

	// ---- デバッグ ----
	ImGui::Checkbox("Pause", &dePaused_);
	ImGui::SameLine();
	ImGui::Checkbox("Draw lines", &deDrawLines_);
	ImGui::SameLine();
	ImGui::Checkbox("Auto attack", &autoAttack_);
	ImGui::DragFloat("Speed scale", &speedScale_, 0.01f, 0.1f, 3.0f);

	if (ImGui::Button("Restart battle")) {
		Restart();
	}
	ImGui::SameLine();
	const bool canAdvance = phaseIndex_ + 1 < static_cast<int>(data_.phases.size()) && !IsInvincible();
	if (ImGui::Button("Next phase") && canAdvance) {
		const float floorHp = GetNextPhaseHpFloor();
		if (floorHp > 0.0f) {
			hp_ = (std::min)(hp_, floorHp); // HP を閾値まで下げて、きっかけと同じ流れで移行する
		}
		StartTransition(phaseIndex_ + 1);
	}

	// ダメージのテスト（自機の攻撃ができるまでの確認用）
	ImGui::DragFloat("Test damage", &deTestDamage_, 1.0f, 0.0f, 10000.0f);
	auto testHit = [this](float angleOffset, const char* label) {
		const float a = GetFacingAngle() + angleOffset;
		const Math::Vector3 from = {center_.x + std::cos(a) * 10.0f, center_.y, center_.z + std::sin(a) * 10.0f};
		static const char* kResultNames[] = {"Hit (コアに命中)", "Blocked (弾かれた)", "Ignored (無敵など)"};
		deLastDamage_ = std::string(label) + " → " + kResultNames[static_cast<int>(TakeDamage(deTestDamage_, from))];
	};
	if (ImGui::Button("Hit from back (core)")) {
		testHit(kPi, "背中から");
	}
	ImGui::SameLine();
	if (ImGui::Button("Hit from front")) {
		testHit(0.0f, "正面から");
	}
	ImGui::Text("結果 : %s", deLastDamage_.empty() ? "-" : deLastDamage_.c_str());
	ImGui::Text("自機の位置 : %s", IsInCoreArc(targetPosition_) ? "コアを狙える" : "正面（弾かれる）");

	// 出す攻撃を固定する（形態の抽選の代わり）
	std::vector<const char*> items = {"Phase (形態の抽選)"};
	int selected = 0;
	for (const std::unique_ptr<BossAttackPattern>& pattern : patterns_) {
		items.push_back(pattern->GetName().c_str());
		if (pattern->GetName() == deForcedPattern_) {
			selected = static_cast<int>(items.size()) - 1;
		}
	}
	if (ImGui::Combo("Force pattern", &selected, items.data(), static_cast<int>(items.size()))) {
		deForcedPattern_ = selected == 0 ? std::string() : patterns_[selected - 1]->GetName();
	}
#endif
}

void Boss::DrawImGuiData() {
#ifdef USE_IMGUI
	// ---- 保存・読み込み ----
	if (ImGui::Button("Save")) {
		SaveData();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload")) {
		LoadData();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset to default")) {
		ResetDataToDefault();
	}
	if (!deDataMessage_.empty()) {
		ImGui::Text("%s", deDataMessage_.c_str());
	}
	ImGui::TextDisabled("攻撃パターンは BossEdit シーンで作る。形態の追加や削除は %s を書き換えて Reload", kBossJsonPath);

	// ---- ボス共通 ----
	if (ImGui::CollapsingHeader("Common (ボス共通)")) {
		ImGui::DragFloat("Max HP", &data_.common.maxHp, 10.0f, 1.0f, 100000.0f);
		ImGui::DragFloat("Core arc (deg)", &data_.common.coreArcDeg, 1.0f, 0.0f, 360.0f);
		ImGui::DragFloat("Face tolerance (deg)", &data_.common.faceTolDeg, 0.5f, 0.0f, 180.0f);
		ImGui::DragFloat("Face max wait", &data_.common.faceMaxWait, 0.05f, 0.0f, 10.0f);
		ImGui::DragFloat("First delay", &data_.common.firstDelay, 0.05f, 0.0f, 10.0f);
		hp_ = (std::min)(hp_, data_.common.maxHp);
	}

	// ---- 形態 ----
	if (ImGui::CollapsingHeader("Phases (形態)", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (size_t p = 0; p < data_.phases.size(); ++p) {
			BossPhaseData& phase = data_.phases[p];
			ImGui::PushID(static_cast<int>(p));
			const bool current = static_cast<int>(p) == phaseIndex_;
			if (ImGui::TreeNodeEx("phase", current ? ImGuiTreeNodeFlags_DefaultOpen : 0, "%d: %s%s", static_cast<int>(p) + 1, phase.name.c_str(), current ? "  ← いま" : "")) {
				if (p > 0) {
					ImGui::Text("Trigger : %s", phase.trigger.type.c_str());
					if (phase.trigger.type == "HpRatio") {
						ImGui::SliderFloat("HP ratio", &phase.trigger.value, 0.0f, 1.0f, "%.2f");
					} else {
						ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "知らないきっかけです（移行しません）");
					}
					ImGui::DragFloat("Transition time", &phase.transitionTime, 0.05f, 0.0f, 10.0f);
				}
				ImGui::DragFloat("Turn speed (deg/s)", &phase.turnSpeedDeg, 1.0f, 0.0f, 720.0f);
				ImGui::DragFloat("Idle time", &phase.idleTime, 0.01f, 0.0f, 10.0f);
				ImGui::Text("Model : %s", phase.look.model.c_str());
				ImGui::ColorEdit4("Color", &phase.look.color.x);
				ImGui::DragFloat3("Scale", &phase.look.scale.x, 0.01f, 0.1f, 20.0f);

				ImGui::Text("Patterns (重み / 再使用までの回数)");
				for (size_t e = 0; e < phase.patterns.size(); ++e) {
					BossPatternEntry& entry = phase.patterns[e];
					ImGui::PushID(static_cast<int>(e));
					const BossAttackPattern* pattern = FindPattern(entry.pattern);
					if (!pattern || !pattern->IsValid()) {
						ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "%s : %s", entry.pattern.c_str(), pattern ? pattern->GetError().c_str() : "見つかりません");
					} else {
						const int remain = current && e < cooldowns_.size() ? cooldowns_[e] : 0;
						ImGui::Text("%s%s", entry.pattern.c_str(), remain > 0 ? "  (再使用待ち)" : "");
					}
					ImGui::SetNextItemWidth(120.0f);
					ImGui::DragFloat("Weight", &entry.weight, 0.05f, 0.0f, 100.0f);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(120.0f);
					ImGui::DragInt("Cooldown", &entry.cooldown, 0.1f, 0, 20);
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	// ---- 攻撃パターン（タイムライン） ----
	if (ImGui::CollapsingHeader("Patterns (攻撃パターン)")) {
		for (const std::unique_ptr<BossAttackPattern>& pattern : patterns_) {
			const BossPatternData& data = pattern->GetData();
			if (pattern->IsValid() && data.clips.size() == 1 && data.name == data.clips[0].attack) {
				continue; // 攻撃名の単発は省略
			}
			std::string clips;
			for (const BossTimelineClip& clip : data.clips) {
				char text[96];
				snprintf(text, sizeof(text), "%s%s@%.2f", clips.empty() ? "" : ", ", clip.attack.c_str(), clip.start);
				clips += text;
			}
			ImGui::Text("%s : %s / 硬直 %.2f", data.name.c_str(), clips.c_str(), data.recoveryTime);
			if (!pattern->IsValid()) {
				ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "  %s", pattern->GetError().c_str());
			}
		}
	}

	// ---- 攻撃ごとの数値 ----
	if (ImGui::CollapsingHeader("Attacks (攻撃ごとの数値)")) {
		for (const std::unique_ptr<BossAttackState>& attack : attacks_) {
			attack->DrawImGui();
		}

		// 仕様：爆撃の半径 ≥ ステージ半径（避けられない）
		if (auto* bomb = dynamic_cast<BossAttackOmniBomb*>(FindAttack("OmniBomb")); bomb && bomb->GetParams().radius < stageRadius_) {
			ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "全方位爆撃の半径がステージ半径(%.1f)未満です", stageRadius_);
		}
		// 仕様：扇の半径 ＜ ステージ半径（離れれば避けられる）
		if (auto* sweep = dynamic_cast<BossAttackSweep*>(FindAttack("Sweep")); sweep && sweep->GetParams().radius >= stageRadius_) {
			ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "薙ぎ払いの半径がステージ半径(%.1f)以上です", stageRadius_);
		}
	}
#endif
}
