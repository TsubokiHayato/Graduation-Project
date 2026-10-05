#pragma once
#include"engine/Framework/Framework.h"

// ゲームの起動クラス（Framework を継承）。各フレームの初期化/更新/描画/終了を担う。
namespace TuboEngine {
class Order : public Framework {
public:
	void Initialize() override;
	void Update() override;
	void Finalize() override;
	void Draw() override;
};
}
