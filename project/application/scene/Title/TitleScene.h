#pragma once
#include "IScene.h"
#include "Camera.h"
#include "Object3d.h" // 背景スカイボックス
#include "Vector2.h"
#include "FadeScreen.h" // シーン遷移フェード
#include <array>
#include <memory>
#include <string>

namespace TuboEngine {
class TextObject; // 前方宣言（メニュー項目/カーソルの生ポインタを保持するだけ）
}

// タイトル画面（ここから開始）
// メニュー（ゲームスタート / 設定 / 終了）をキーボード＋パッドで操作する。
// 見た目・配置はすべて Resources/Text/Title.json 側で定義する。
class TitleScene : public IScene {
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
	// メニュー項目（Title.json の name と対応）
	enum MenuItem {
		kMenuStart = 0, // "menu_start" ゲームスタート → STAGE へ
		kMenuOption,    // "menu_option" 設定（プレースホルダ：専用シーン未実装）
		kMenuExit,      // "menu_exit" 終了 → アプリ終了
		kMenuCount,
	};

	// メニューの見た目（登場アニメ・選択色・カーソル移動・明滅）を毎フレーム更新する。
	void UpdateMenuVisual(float dt);
	// 現在の選択項目を決定（Enter/A）したときの処理。
	void DecideSelection();
	// 上下入力（キーボード＋パッド）のトリガーを取る。戻り値: -1=上, +1=下, 0=なし。
	int TakeVerticalInput() const;
	// 決定入力（Enter/Space/Aボタン）のトリガー。
	bool TakeDecideInput() const;

	std::unique_ptr<TuboEngine::Camera> camera_;

	
	std::unique_ptr<TuboEngine::Object3d> background;//背景オブジェクト

	// Title.json からロードしたメニュー本体（TextManager 所有。ここは参照用の生ポインタ）
	std::array<TuboEngine::TextObject*, kMenuCount> menuItems_{};
	TuboEngine::TextObject* cursor_ = nullptr;
	TuboEngine::TextObject* title_ = nullptr; // タイトル文字（登場アニメ用）
	TuboEngine::TextObject* hint_ = nullptr;  // 操作ヒント（登場アニメ用）

	int selected_ = kMenuStart;

	// --- タイトル演出（登場アニメ・カーソル移動・明滅・パーティクル）---
	float introTimer_ = 0.0f; // 登場アニメの経過（秒）。開始時に 0 から数える。
	float animTime_ = 0.0f;   // 常時進む時計（選択項目のパルス／カーソル明滅用）。
	// 各テキストの「最終的な表示位置」（登場アニメはここへ向かって寄せる）。
	std::array<TuboEngine::Math::Vector2, kMenuCount> menuHome_{};
	TuboEngine::Math::Vector2 titleHome_{};
	// イージング移動するカーソルの現在位置。
	TuboEngine::Math::Vector2 cursorPos_{};
	bool cursorInit_ = false;
	// タイトル背景に舞うパーティクル（Finalize で片付ける用の名前）。
	std::string particleName_;

	std::unique_ptr<FadeScreen> fadeScreen_; // 入場フェードイン＋退場フェードアウト
	int pendingScene_ = -1;                  // フェードアウト完了後に切り替える先(-1=無し)
};
