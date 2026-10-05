#include "TitleScene.h"
#include "GameScenes.h"
#include "SceneManager.h" // シーン遷移を使うとき用
#include "TextManager.h"
#include "TextObject.h"
#include "Input.h"   // キーボード / パッド入力
#include "audio/AudioManager.h" // BGM / SE
#include "ParticleManager.h"    // タイトル背景のパーティクル
#include "IParticleEmitter.h"   // ParticlePreset 構造体
#include <Windows.h> // PostQuitMessage（終了）
#include <cstdlib>   // rand
#include <cmath>     // sin

using namespace TuboEngine;

namespace {
// 選択中／非選択のメニュー文字色
constexpr Math::Vector4 kColorNormal   = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr Math::Vector4 kColorSelected = {1.0f, 0.85f, 0.2f, 1.0f};

// カーソル「▶」を選択項目の中心からどれだけ左に置くか（Title.json の cursor 位置と揃える）
constexpr float kCursorOffsetX = 175.0f;

// Title.json 内のメニュー項目 name（enum MenuItem の並びと対応）
constexpr const char* kMenuNames[] = {"menu_start", "menu_option", "menu_exit"};

// --- 登場アニメのタイミング（秒）---
constexpr float kTitleStart  = 0.0f;  // タイトル文字が出始める時刻
constexpr float kTitleDur    = 0.5f;  // タイトルのフェード時間
constexpr float kMenuStartT  = 0.35f; // 最初のメニュー項目が出始める時刻
constexpr float kMenuStagger = 0.12f; // 項目ごとの遅延（順番に出す）
constexpr float kMenuDur     = 0.4f;  // 各メニュー項目のフェード時間
constexpr float kCursorStart = 0.9f;  // カーソル／ヒントが出始める時刻

// 0〜1 にクランプ
float Clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }
// イーズアウト（最後にゆっくり止まる）
float EaseOutCubic(float t) {
	t = Clamp01(t);
	float u = 1.0f - t;
	return 1.0f - u * u * u;
}
} // namespace

void TitleScene::Initialize() {
	// キューブを斜め上から見る固定カメラ(立体感を出す)。
	// StageScene と同じ球面座標の規約: pos=半径*(cosP sinY, sinP, cosP cosY), rot={P, Y+π, 0}
	camera_ = std::make_unique<TuboEngine::Camera>();
	camera_->SetTranslate({ 3.64f, 2.73f, 5.32f });
	camera_->setRotation({ 0.4f, 0.6f + 3.141592f, 0.0f });
	camera_->setScale({ 1.0f, 1.0f, 1.0f });
	camera_->Update();

	background = std::make_unique<TuboEngine::Object3d>();
	background->Initialize("skyBox/skyBox.obj");

	auto* tm = TextManager::GetInstance();

	// タイトル画面の見た目（タイトル・メニュー・カーソル・ヒント・バージョン）はすべて JSON で定義。
	// ImGui の TextManager パネルで位置や色を調整でき、Save で書き戻せる。
	tm->LoadTextLayout("Resources/Text/Title.json");

	// 選択で色替え／移動する要素だけ、name で引いて生ポインタを保持しておく。
	for (int i = 0; i < kMenuCount; ++i) {
		menuItems_[i] = tm->GetTextByName(kMenuNames[i]);
		// JSON 由来の位置を「最終位置」として覚える（登場アニメはここへ寄せる）。
		if (menuItems_[i])
			menuHome_[i] = menuItems_[i]->GetPosition();
	}
	cursor_ = tm->GetTextByName("cursor");
	title_ = tm->GetTextByName("title");
	hint_ = tm->GetTextByName("hint");
	if (title_)
		titleHome_ = title_->GetPosition();

	selected_ = kMenuStart;

	// 登場アニメを最初から再生する。
	introTimer_ = 0.0f;
	animTime_ = 0.0f;
	cursorInit_ = false;
	UpdateMenuVisual(0.0f); // 1フレーム目から「まだ出ていない」状態にしておく

	// タイトル背景に、ゆっくり上へ舞う四角い粒を出す。
	// Default エミッターの板ポリは正方形なので、不透明な正方形テクスチャ(yellow.png)を
	// 貼ると「四角いパーティクル」になる。色・透明度は color で制御する。
	ParticlePreset preset;
	preset.name = "TitleSquare";
	preset.texture = "yellow.png"; // "Resources/Textures/" から。不透明な黄色い正方形
	preset.maxInstances = 256;
	preset.billboard = true; // 常にカメラを向く
	preset.autoEmit = true;  // 出し続ける
	preset.emitRate = 18.0f; // 1秒あたりの放出数（控えめ）
	preset.center = {0.0f, -3.5f, 0.0f};
	preset.posMin = {-6.0f, 0.0f, -2.0f};
	preset.posMax = {6.0f, 0.0f, 2.0f};
	preset.velMin = {-0.1f, 0.4f, -0.1f};
	preset.velMax = {0.1f, 1.0f, 0.1f};
	preset.lifeMin = 2.5f;
	preset.lifeMax = 4.5f;
	preset.gravity = {0.0f, 0.0f, 0.0f};
	preset.scaleStart = {0.12f, 0.12f, 0.12f};
	preset.scaleEnd = {0.04f, 0.04f, 0.04f}; // 四角形を保ったまま少し小さくして消える
	preset.colorStart = {1.0f, 1.0f, 1.0f, 0.9f}; // テクスチャの黄色をそのまま活かす
	preset.colorEnd = {1.0f, 1.0f, 1.0f, 0.0f};   // だんだん透明に
	if (IParticleEmitter* e = ParticleManager::GetInstance()->CreateEmitterByType("Default", preset))
		particleName_ = e->GetName();

	// タイトルを開いた合図の SE（メニューがスッと出てくる感じの音）。
	AudioManager::GetInstance()->PlaySe("window_open.mp3");

	// タイトル BGM を無音から 1.2 秒かけてフェードインしつつループ再生。
	// （別シーンから戻ってきて同じ曲が鳴っているときは、フェードせず継続する）
	AudioManager::GetInstance()->PlayBgmFadeIn("title.wav", 1.2f);

	// シーン遷移フェード(入場で黒→クリア、退場で FadeOut)。
	fadeScreen_ = std::make_unique<FadeScreen>();
	fadeScreen_->Initialize();
}

void TitleScene::Update() {
	const float dt = 1.0f / 60.0f;
	animTime_ += dt;
	introTimer_ += dt;

	camera_->Update();

	// BGM フェードインを進める。
	AudioManager::GetInstance()->UpdateFade(dt);

	// フェードアウト中(遷移予約済み)は操作を止める。
	if (pendingScene_ < 0) {
		// メニュー移動（上下）
		if (int dir = TakeVerticalInput(); dir != 0) {
			selected_ = (selected_ + dir + kMenuCount) % kMenuCount; // 端でループ
			AudioManager::GetInstance()->PlaySe("cursor_move.mp3"); // カーソル移動音
		}

		// 決定
		if (TakeDecideInput()) {
			AudioManager::GetInstance()->PlaySe("decide.mp3"); // 決定音
			DecideSelection();
		}
	}

	// メニューの見た目（登場アニメ・選択色・カーソル移動・明滅）を更新する。
	UpdateMenuVisual(dt);

	background->SetCamera(camera_.get());
	background->Update();

	// パーティクルはエンジンが自動更新しないので、シーンが駆動する。
	ParticleManager::GetInstance()->Update(dt, camera_.get());

	TuboEngine::TextManager::GetInstance()->UpdateAll();

	// フェード進行と、真っ黒になったら実際のシーン切り替え。
	fadeScreen_->Update();
	if (pendingScene_ >= 0 && fadeScreen_->IsFadeOuting()) {
		SceneManager::GetInstance()->ChangeScene(pendingScene_);
	}
}

// メニューの見た目を毎フレーム作り直す。
//  ・登場アニメ: introTimer_ に応じてタイトル→メニュー→カーソルの順にフェードイン＋寄せ
//  ・選択表現 : 選択項目は色を変え、わずかに拡大パルスさせる
//  ・カーソル : 目標位置へイージングで滑らかに移動し、ゆっくり明滅する
void TitleScene::UpdateMenuVisual(float dt) {
	(void)dt;

	// --- タイトル文字（上から少し降りてフェードイン）---
	if (title_) {
		float e = EaseOutCubic((introTimer_ - kTitleStart) / kTitleDur);
		title_->SetColor({1.0f, 1.0f, 1.0f, e});
		title_->SetPosition({titleHome_.x, titleHome_.y - (1.0f - e) * 40.0f});
	}

	// --- メニュー項目（順番に下から寄ってフェードイン）---
	for (int i = 0; i < kMenuCount; ++i) {
		if (!menuItems_[i])
			continue;
		float start = kMenuStartT + kMenuStagger * static_cast<float>(i);
		float e = EaseOutCubic((introTimer_ - start) / kMenuDur);

		// 選択中は黄色、その他は白。アルファは登場アニメの進み具合。
		const Math::Vector4& base = (i == selected_) ? kColorSelected : kColorNormal;
		menuItems_[i]->SetColor({base.x, base.y, base.z, e * base.w});

		// 下から寄せる。
		menuItems_[i]->SetPosition({menuHome_[i].x, menuHome_[i].y + (1.0f - e) * 30.0f});

		// 選択項目だけ軽く拡大パルス（今どこを選んでいるか分かりやすく）。
		float scale = 1.0f;
		if (i == selected_)
			scale = 1.0f + 0.08f * std::sin(animTime_ * 6.0f);
		menuItems_[i]->SetScale(scale);
	}

	// --- カーソル（目標へイージング移動＋明滅、遅れてフェードイン）---
	if (cursor_) {
		Math::Vector2 target = {menuHome_[selected_].x - kCursorOffsetX, menuHome_[selected_].y};
		if (!cursorInit_) {
			cursorPos_ = target; // 初回はワープ（最初だけ滑らせない）
			cursorInit_ = true;
		}
		// 指数的に目標へ寄せる（フレーム毎に距離の25%を詰める）。
		cursorPos_.x += (target.x - cursorPos_.x) * 0.25f;
		cursorPos_.y += (target.y - cursorPos_.y) * 0.25f;
		cursor_->SetPosition(cursorPos_);

		float e = EaseOutCubic((introTimer_ - kCursorStart) / kMenuDur);
		float blink = 0.6f + 0.4f * std::sin(animTime_ * 5.0f); // ゆっくり明滅
		cursor_->SetColor({kColorSelected.x, kColorSelected.y, kColorSelected.z, e * blink});
	}

	// --- 操作ヒント（カーソルと同じタイミングでフェードイン）---
	if (hint_) {
		float e = EaseOutCubic((introTimer_ - kCursorStart) / kMenuDur);
		const Math::Vector4& c = hint_->GetColor();
		hint_->SetColor({c.x, c.y, c.z, e}); // JSON の灰色を保ちつつアルファだけ動かす
	}
}

void TitleScene::DecideSelection() {
	switch (selected_) {
	case kMenuStart:
		AudioManager::GetInstance()->PlayBgm("game.wav"); // ゲーム BGM へ切替
		pendingScene_ = SAMPLE; // フェードアウト後にサンプルシーンへ（テンプレの遷移先）
		fadeScreen_->FadeOut();
		break;
	case kMenuOption:
		pendingScene_ = OPTION; // フェードアウト後に設定画面へ
		fadeScreen_->FadeOut();
		break;
	case kMenuExit:
		PostQuitMessage(0); // アプリ終了（WM_QUIT）
		break;
	default:
		break;
	}
}

int TitleScene::TakeVerticalInput() const {
	auto* in = Input::GetInstance();

	bool up = in->TriggerKey(DIK_UP) || in->TriggerKey(DIK_W);
	bool down = in->TriggerKey(DIK_DOWN) || in->TriggerKey(DIK_S);

	// パッド（XInput）の十字キー。押した瞬間だけ拾う（前フレームとの差分）。
	XINPUT_STATE cur{}, prev{};
	if (in->GetJoystickState(0, cur) && in->GetJoystickStatePrevious(0, prev)) {
		auto trigger = [&](WORD mask) {
			return (cur.Gamepad.wButtons & mask) && !(prev.Gamepad.wButtons & mask);
		};
		if (trigger(XINPUT_GAMEPAD_DPAD_UP)) up = true;
		if (trigger(XINPUT_GAMEPAD_DPAD_DOWN)) down = true;
	}

	if (up && !down) return -1;
	if (down && !up) return +1;
	return 0;
}

bool TitleScene::TakeDecideInput() const {
	auto* in = Input::GetInstance();

	bool decide = in->TriggerKey(DIK_RETURN) || in->TriggerKey(DIK_NUMPADENTER) || in->TriggerKey(DIK_SPACE);

	XINPUT_STATE cur{}, prev{};
	if (in->GetJoystickState(0, cur) && in->GetJoystickStatePrevious(0, prev)) {
		auto trigger = [&](WORD mask) {
			return (cur.Gamepad.wButtons & mask) && !(prev.Gamepad.wButtons & mask);
		};
		if (trigger(XINPUT_GAMEPAD_A) || trigger(XINPUT_GAMEPAD_START)) decide = true;
	}

	return decide;
}

void TitleScene::Finalize() {
	// タイトルの UI を次シーンへ持ち越さないよう片付ける。
	TuboEngine::TextManager::GetInstance()->ClearAllTexts();
	TuboEngine::TextManager::GetInstance()->ClearAllSprites();
	menuItems_.fill(nullptr);
	cursor_ = nullptr;
	title_ = nullptr;
	hint_ = nullptr;

	// タイトル専用のパーティクルを片付ける（次シーンへ残さない）。
	if (!particleName_.empty()) {
		ParticleManager::GetInstance()->Remove(particleName_);
		particleName_.clear();
	}
}

void TitleScene::Object3DDraw() {
	background->Draw();
} // 3Dオブジェクト描画(背景スカイボックス)
void TitleScene::SpriteDraw() {
	TuboEngine::TextManager::GetInstance()->DrawAll();
	fadeScreen_->Draw();
} // 2Dスプライト描画
void TitleScene::ImGuiDraw() { TuboEngine::TextManager::GetInstance()->DrawImGui(); } // ImGui描画
void TitleScene::ParticleDraw() { ParticleManager::GetInstance()->Draw(); }           // 背景パーティクル描画
