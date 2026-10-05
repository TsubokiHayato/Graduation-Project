#include "OptionScene.h"
#include "GameScenes.h"
#include "SceneManager.h"
#include "TextManager.h"
#include "TextObject.h"
#include "settings/Settings.h"
#include "audio/AudioManager.h"
#include "Input.h"
#include <string>

using namespace TuboEngine;

namespace {
constexpr Math::Vector4 kColorNormal   = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr Math::Vector4 kColorSelected = {1.0f, 0.85f, 0.2f, 1.0f};

// 行レイアウト（画面 1280x720 基準。中央付近に左揃えで並べる）
constexpr float kLabelX   = 440.0f; // ラベル左端
constexpr float kValueX   = 760.0f; // 値の左端（列を揃える）
constexpr float kStartY   = 250.0f; // 1行目の中心Y
constexpr float kRowStep  = 64.0f;  // 行間
constexpr float kCursorGap = 44.0f; // ラベル左端からカーソルまでの距離
constexpr float kMenuFontSize = 36.0f;

// 0.0〜1.0 を "80%" 形式に。
std::string ToPercent(float v01) {
	int pct = static_cast<int>(v01 * 100.0f + 0.5f);
	return std::to_string(pct) + "%";
}

float Clamp01(float v) {
	if (v < 0.0f) return 0.0f;
	if (v > 1.0f) return 1.0f;
	return v;
}
} // namespace

void OptionScene::Initialize() {
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({0.0f, 0.0f, -15.0f});
	camera_->setRotation({0.0f, 0.0f, 0.0f});
	camera_->setScale({1.0f, 1.0f, 1.0f});
	camera_->Update();

	// 前シーンの UI が残っていても作り直せるようクリアしてから構築する。
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();

	BuildItems();
	CreateTexts();

	selected_ = 0;
	RefreshValues();
	ApplySelection();

	// タイトルから引き継いだ BGM を維持（直接開いても鳴るように）。
	AudioManager::GetInstance()->PlayBgm("title.wav");
	// 設定画面を開いた合図。
	AudioManager::GetInstance()->PlaySe("window_open.mp3");

	// シーン遷移フェード(入場で黒→クリア、退場で FadeOut)。
	fadeScreen_ = std::make_unique<FadeScreen>();
	fadeScreen_->Initialize();
}

// =============================================================================
//  ★設定項目の定義（増やすのはここだけ）
// =============================================================================
void OptionScene::BuildItems() {
	Settings* s = Settings::GetInstance();

	// 値を持つ項目：valueText（表示）と onAdjust（← → で変更）を書く。
	items_.push_back(Item{
		"BGM音量",
		[s] { return ToPercent(s->bgmVolume); },
		[s](int dir) {
			s->bgmVolume = Clamp01(s->bgmVolume + dir * 0.1f);
			AudioManager::GetInstance()->ApplyBgmVolume(); // 鳴っている BGM に即反映
		},
		nullptr});

	items_.push_back(Item{
		"SE音量",
		[s] { return ToPercent(s->seVolume); },
		[s](int dir) {
			s->seVolume = Clamp01(s->seVolume + dir * 0.1f);
			AudioManager::GetInstance()->PlaySe("cursor_move.mp3"); // 変更後の音量を試聴
		},
		nullptr});

	// アクション項目：onDecide（決定時の動作）を書く。
	items_.push_back(Item{
		"既定値に戻す", nullptr, nullptr,
		[this] {
			Settings::GetInstance()->ResetToDefault();
			AudioManager::GetInstance()->ApplyBgmVolume();
			RefreshValues();
		}});

	items_.push_back(Item{
		"タイトルへ戻る", nullptr, nullptr,
		[this] {
			Settings::GetInstance()->Save();
			pendingScene_ = TITLE; // フェードアウト完了後にタイトルへ
			fadeScreen_->FadeOut();
		}});
}

void OptionScene::CreateTexts() {
	auto* tm = TextManager::GetInstance();
	tm->GetOrCreateFontSized(TextManager::PresetFontNames::YasashisaGothicBold, kMenuFontSize);
	const std::string font =
		TextManager::PresetFontNames::YasashisaGothicBold + "_" + std::to_string(static_cast<int>(kMenuFontSize));

	// 見出しとヒント（参照は持たない）
	tm->GetOrCreateFontSized(TextManager::PresetFontNames::YasashisaGothicBold, 56.0f);
	if (TextObject* head = tm->CreateText(
			TextManager::PresetFontNames::YasashisaGothicBold + "_56", "設定", {640.0f, 120.0f})) {
		head->SetHorizontalAlign(1);
		head->SetVerticalAlign(1);
	}
	tm->GetOrCreateFontSized(TextManager::PresetFontNames::YasashisaGothicBold, 22.0f);
	if (TextObject* hint = tm->CreateText(
			TextManager::PresetFontNames::YasashisaGothicBold + "_22",
			"↑↓ : 項目　←→ : 変更　Enter : 決定　Esc : 戻る", {640.0f, 680.0f})) {
		hint->SetHorizontalAlign(1);
		hint->SetVerticalAlign(1);
		hint->SetColor({0.8f, 0.8f, 0.8f, 1.0f});
	}

	// 各行（ラベル＋値）
	labelTexts_.resize(items_.size(), nullptr);
	valueTexts_.resize(items_.size(), nullptr);
	for (size_t i = 0; i < items_.size(); ++i) {
		const float y = kStartY + static_cast<float>(i) * kRowStep;

		TextObject* label = tm->CreateText(font, items_[i].label, {kLabelX, y});
		if (label) {
			label->SetHorizontalAlign(0); // 左揃え
			label->SetVerticalAlign(1);
		}
		labelTexts_[i] = label;

		if (items_[i].valueText) {
			TextObject* value = tm->CreateText(font, items_[i].valueText(), {kValueX, y});
			if (value) {
				value->SetHorizontalAlign(0);
				value->SetVerticalAlign(1);
			}
			valueTexts_[i] = value;
		}
	}

	// カーソル
	cursor_ = tm->CreateText(font, "▶", {kLabelX - kCursorGap, kStartY});
	if (cursor_) {
		cursor_->SetHorizontalAlign(1);
		cursor_->SetVerticalAlign(1);
	}
}

void OptionScene::RefreshValues() {
	for (size_t i = 0; i < items_.size(); ++i) {
		if (items_[i].valueText && i < valueTexts_.size() && valueTexts_[i]) {
			valueTexts_[i]->SetText(items_[i].valueText());
		}
	}
}

void OptionScene::ApplySelection() {
	for (size_t i = 0; i < labelTexts_.size(); ++i) {
		const Math::Vector4 c = (static_cast<int>(i) == selected_) ? kColorSelected : kColorNormal;
		if (labelTexts_[i]) labelTexts_[i]->SetColor(c);
		if (i < valueTexts_.size() && valueTexts_[i]) valueTexts_[i]->SetColor(c);
	}
	if (cursor_ && !labelTexts_.empty()) {
		const size_t sel = static_cast<size_t>(selected_);
		if (labelTexts_[sel]) {
			const Math::Vector2 p = labelTexts_[sel]->GetPosition();
			cursor_->SetPosition({p.x - kCursorGap, p.y});
		}
	}
}

void OptionScene::Update() {
	camera_->Update();

	// フェードアウト中(遷移予約済み)は入力を止めて、真っ黒になったら切り替える。
	if (pendingScene_ >= 0) {
		TextManager::GetInstance()->UpdateAll();
		fadeScreen_->Update();
		if (fadeScreen_->IsFadeOuting())
			SceneManager::GetInstance()->ChangeScene(pendingScene_);
		return;
	}

	const int itemCount = static_cast<int>(items_.size());
	if (itemCount > 0) {
		// 項目移動
		if (int dir = TakeVerticalInput(); dir != 0) {
			selected_ = (selected_ + dir + itemCount) % itemCount;
			ApplySelection();
			AudioManager::GetInstance()->PlaySe("cursor_move.mp3");
		}

		// 値変更（← →）
		if (int dir = TakeHorizontalInput(); dir != 0) {
			Item& item = items_[static_cast<size_t>(selected_)];
			if (item.onAdjust) {
				item.onAdjust(dir);
				RefreshValues();
			}
		}

		// 決定
		if (TakeDecideInput()) {
			AudioManager::GetInstance()->PlaySe("decide.mp3");
			Item& item = items_[static_cast<size_t>(selected_)];
			if (item.onDecide) {
				item.onDecide();
			}
		}
	}

	// キャンセル（保存してタイトルへ）
	if (TakeCancelInput()) {
		Settings::GetInstance()->Save();
		pendingScene_ = TITLE; // フェードアウト完了後にタイトルへ
		fadeScreen_->FadeOut();
	}

	TextManager::GetInstance()->UpdateAll();

	// フェード進行(入場フェードイン用。退場は上の早期分岐で処理)。
	fadeScreen_->Update();
}

int OptionScene::TakeVerticalInput() const {
	auto* in = Input::GetInstance();
	bool up = in->TriggerKey(DIK_UP) || in->TriggerKey(DIK_W);
	bool down = in->TriggerKey(DIK_DOWN) || in->TriggerKey(DIK_S);

	XINPUT_STATE cur{}, prev{};
	if (in->GetJoystickState(0, cur) && in->GetJoystickStatePrevious(0, prev)) {
		auto trigger = [&](WORD mask) { return (cur.Gamepad.wButtons & mask) && !(prev.Gamepad.wButtons & mask); };
		if (trigger(XINPUT_GAMEPAD_DPAD_UP)) up = true;
		if (trigger(XINPUT_GAMEPAD_DPAD_DOWN)) down = true;
	}

	if (up && !down) return -1;
	if (down && !up) return +1;
	return 0;
}

int OptionScene::TakeHorizontalInput() const {
	auto* in = Input::GetInstance();
	bool left = in->TriggerKey(DIK_LEFT) || in->TriggerKey(DIK_A);
	bool right = in->TriggerKey(DIK_RIGHT) || in->TriggerKey(DIK_D);

	XINPUT_STATE cur{}, prev{};
	if (in->GetJoystickState(0, cur) && in->GetJoystickStatePrevious(0, prev)) {
		auto trigger = [&](WORD mask) { return (cur.Gamepad.wButtons & mask) && !(prev.Gamepad.wButtons & mask); };
		if (trigger(XINPUT_GAMEPAD_DPAD_LEFT)) left = true;
		if (trigger(XINPUT_GAMEPAD_DPAD_RIGHT)) right = true;
	}

	if (left && !right) return -1;
	if (right && !left) return +1;
	return 0;
}

bool OptionScene::TakeDecideInput() const {
	auto* in = Input::GetInstance();
	bool decide = in->TriggerKey(DIK_RETURN) || in->TriggerKey(DIK_NUMPADENTER) || in->TriggerKey(DIK_SPACE);

	XINPUT_STATE cur{}, prev{};
	if (in->GetJoystickState(0, cur) && in->GetJoystickStatePrevious(0, prev)) {
		auto trigger = [&](WORD mask) { return (cur.Gamepad.wButtons & mask) && !(prev.Gamepad.wButtons & mask); };
		if (trigger(XINPUT_GAMEPAD_A)) decide = true;
	}

	return decide;
}

bool OptionScene::TakeCancelInput() const {
	auto* in = Input::GetInstance();
	bool cancel = in->TriggerKey(DIK_ESCAPE) || in->TriggerKey(DIK_BACK);

	XINPUT_STATE cur{}, prev{};
	if (in->GetJoystickState(0, cur) && in->GetJoystickStatePrevious(0, prev)) {
		auto trigger = [&](WORD mask) { return (cur.Gamepad.wButtons & mask) && !(prev.Gamepad.wButtons & mask); };
		if (trigger(XINPUT_GAMEPAD_B)) cancel = true;
	}

	return cancel;
}

void OptionScene::Finalize() {
	TextManager::GetInstance()->ClearAllTexts();
	TextManager::GetInstance()->ClearAllSprites();
	labelTexts_.clear();
	valueTexts_.clear();
	cursor_ = nullptr;
	items_.clear();
}

void OptionScene::Object3DDraw() {}
void OptionScene::SpriteDraw() {
	TextManager::GetInstance()->DrawAll();
	fadeScreen_->Draw();
}
void OptionScene::ImGuiDraw() { TextManager::GetInstance()->DrawImGui(); }
void OptionScene::ParticleDraw() {}
