#pragma once
#include "BossWarning.h"
#include "Camera.h"
#include "Object3d.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

// =============================================================================
//  警告範囲の描画（GP-12）
//  - Warning の Shape から、地面に貼った半透明メッシュの位置・向き・拡縮を決める
//  - 外枠(zone) = 薄い赤の固定範囲 / 塗り(fill) = 中心から広がる濃い赤
//  - 判定フェーズでは、塗りの色を赤→白へイージングする（Model の色で制御）
//  - メッシュは「単位サイズ」を拡縮して使う（円: 半径1 / 帯: 幅1×長さ1 / 扇: 半径1）
//  - 扇は角度ごとにメッシュ(OBJ)を用意したプリセット制
//  - 外枠と塗りは別ファイルの OBJ（xxx.obj / xxx_fill.obj）。色は Model 単位なので、
//    塗りだけ色を変えるには別 Model にする必要がある
//  - 1 インスタンスで同時に 1 つの警告を描ける（Object3d の定数バッファを描画ごとに
//    共有できないため。同時に複数出すなら Renderer をその数だけ作る）
// =============================================================================
namespace BossHit {

class WarningRenderer {
public:
	// 扇メッシュを用意している角度（度）。gen_warning_assets.py の FAN_ANGLES と揃える
	static constexpr int kFanPresets[] = {30, 45, 60, 90, 120, 180, 270};
	static constexpr int kFanPresetCount = static_cast<int>(sizeof(kFanPresets) / sizeof(kFanPresets[0]));

	// プリセット番号 → ラジアン（Fan::angle に入れる値）
	static float FanPresetRad(int index);

	void Initialize(TuboEngine::Camera* camera);
	// カメラ更新後に毎フレーム呼ぶ
	void Update(const Warning& w);
	// 先に外枠、後から塗りを描く（塗りの方が少し上にあるので、この順でないと隠れる）
	void Draw();

	// true なら、扇の角度がプリセットに無くて何も描けていない
	bool HasMissingFanPreset() const { return missingFanPreset_; }

private:
	struct Layer {
		std::unique_ptr<TuboEngine::Object3d> zone; // 外枠
		std::unique_ptr<TuboEngine::Object3d> fill; // 塗り（別 Model）
	};

	static void MakeLayer(Layer& l, const std::string& path, TuboEngine::Camera* camera);
	static void Place(Layer& l, const Vec3& pos, float yaw, float zoneX, float zoneZ, float fillX, float fillZ, const TuboEngine::Math::Vector4& fillColor);
	Layer* FindFan(int deg);

	Layer circle_;
	Layer beam_;
	std::vector<std::pair<int, Layer>> fans_;
	Layer* active_ = nullptr;
	bool drawFill_ = false;
	bool missingFanPreset_ = false;
};

} // namespace BossHit
