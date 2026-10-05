#pragma once
#include "IScene.h"
#include "Camera.h"
#include "FadeScreen.h" // シーン遷移フェード
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace TuboEngine {
class TextObject; // 前方宣言（行テキスト/カーソルの参照を持つだけ）
}

// 設定画面。
// 項目は「データ駆動」で items_ に積むだけ。表示・入力・カーソルは共通処理が回す。
//
// ★設定項目を1つ増やすには BuildItems() に Item を push_back するだけ:
//    - 値を持つ項目 : label / valueText / onAdjust を書く
//    - アクション項目: label / onDecide を書く（valueText, onAdjust は nullptr）
class OptionScene : public IScene {
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
	// 設定項目1つ分の定義。
	struct Item {
		std::string label;                     // 項目名
		std::function<std::string()> valueText; // 現在値の表示文字列（null=値なしのアクション項目）
		std::function<void(int)> onAdjust;      // ← → で値を変える（dir = -1 / +1）
		std::function<void()> onDecide;         // 決定時の動作（任意）
	};

	void BuildItems();    // ★ここに項目を積む（増やすのはここだけ）
	void CreateTexts();   // items_ から TextObject を生成する
	void RefreshValues(); // 値の表示文字列を最新化する
	void ApplySelection(); // 選択行のハイライトとカーソル位置を反映する

	// 入力（トリガー）。-1/+1/0 や true/false を返す。
	int TakeVerticalInput() const;
	int TakeHorizontalInput() const;
	bool TakeDecideInput() const;
	bool TakeCancelInput() const;

	std::unique_ptr<TuboEngine::Camera> camera_;

	std::vector<Item> items_;
	std::vector<TuboEngine::TextObject*> labelTexts_; // 各項目のラベル
	std::vector<TuboEngine::TextObject*> valueTexts_; // 各項目の値（アクション項目は nullptr）
	TuboEngine::TextObject* cursor_ = nullptr;

	int selected_ = 0;

	std::unique_ptr<FadeScreen> fadeScreen_; // 入場フェードイン＋退場フェードアウト
	int pendingScene_ = -1;                  // フェードアウト完了後に切り替える先(-1=無し)
};
