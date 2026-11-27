#pragma once
#include "Camera.h"
#include "InstanceData.h"
#include "SPH/InstancedMeshRenderer.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <wrl/client.h>

// =========================================
// シールドの見た目（ハニカム球エフェクト）
// - 球面をゴールドバーグ多面体で六角形セルに分け、セル 1 個ずつを
//   インスタンス描画（1 DrawCall）で個別に動かす
// - タイルのモデル/テクスチャは tools/gen_shield_assets.py で生成
// - シールド値がある間は常に表示。数値の変化はセル単位で見える
//     残量率   → 残っているセルの数（減るとセルが 1 枚ずつ縮んで消え、回復で 1 枚ずつ戻る）
//                + 色（青 → 黄 → 赤）
//     出力率   → 球の大きさ
//     被弾     → 輪郭のくっきりした衝撃リングがセルを伝わる（白く点灯・わずかに押し出し）
//     破壊     → シールド値 0 で即座に全セルが消える（Break Duration > 0 で飛び散り演出も可）
//     待機中   → 無機質な動き：セルは固定、走査線が一定速度で通過
// =========================================
class ShieldEffect {
public:
	// 毎フレーム Shield から受け取る状態
	struct State {
		TuboEngine::Math::Vector3 position{}; // 球の中心（プレイヤーの中心）
		float shieldRate = 1.0f;              // 残量率 0～1
		float outputRate = 1.0f;              // 出力率 0～1
	};

	// 調整用パラメータ（ImGui で変更）
	struct Params {
		float radius = 2.6f;           // 出力率 100% のときの半径
		float minRadiusRate = 0.8f;    // 出力率 0% のときの半径倍率（プレイヤーの立方体の角 √3≈1.73 より外に）
		float tileFill = 0.88f;        // セルの大きさ（1 で隙間なし）
		float alpha = 0.85f;           // 全体の不透明度
		float popSpeed = 15.0f;        // セルの出入りの速さ（1/秒）。大きいほど早く覆い終わる
		float scanPeriod = 2.4f;       // 走査線が上から下まで通る時間（秒）
		float scanWidth = 0.06f;       // 走査線の太さ
		float hitDuration = 0.45f;     // 被弾の波紋が伝わる時間
		float hitPush = 0.05f;         // 被弾時にセルが押し出される量（半径比）
		float breakDuration = 0.0f;    // 破壊演出時間。0 = 演出なし（シールド値 0 で即座に何も表示しない）
		float breakDistance = 1.2f;    // 破壊時にセルが飛ぶ距離（半径比）
		TuboEngine::Math::Vector4 fullColor{0.25f, 0.75f, 1.0f, 1.0f}; // 満タン
		TuboEngine::Math::Vector4 midColor{1.0f, 0.85f, 0.2f, 1.0f};   // 半分
		TuboEngine::Math::Vector4 lowColor{1.0f, 0.15f, 0.1f, 1.0f};   // 残りわずか
	};

	void Initialize(TuboEngine::Camera* camera);
	void Update(float deltaTime, const State& state);
	void Draw();
	void DrawImGui();

	// 被弾した（シールドがダメージを吸収した）
	void NotifyHit();
	// シールドが 0 になった
	void NotifyBreak();

	Params& GetParams() { return params_; }

private:
	// 1 セル分の情報
	struct Cell {
		TuboEngine::Math::Vector3 normal;   // 球面上の向き（単位ベクトル）
		TuboEngine::Math::Vector3 tangent;  // タイルの角の向き（normal に垂直）
		float size = 0.0f;                  // 単位球でのセル半径
		float threshold = 0.0f;             // 残量率がこれ未満だと消える（1/N～1 を均等に割り振り）
		float phase = 0.0f;                 // 個体差（回転方向など）
		float speed = 1.0f;                 // 出入りの速さの個体差
		float presence = 0.0f;              // 表示量 0～1（アニメーション）
		float flySpeed = 1.0f;              // 破壊時に飛ぶ速さの個体差
		float flyPresence = 0.0f;           // 破壊した瞬間の表示量
	};

	// 球面をセルに分割する
	void BuildCells(int frequency);
	// 残量率から色を求める
	TuboEngine::Math::Vector4 RateToColor(float rate) const;

	TuboEngine::Camera* camera_ = nullptr;
	InstancedMeshRenderer renderer_;
	Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
	TuboEngine::InstanceData* instanceData_ = nullptr;
	uint32_t drawCount_ = 0;

	std::vector<Cell> cells_;
	std::vector<uint32_t> order_;       // 描画順（奥 → 手前）
	std::vector<float> depth_;          // ソート用のカメラ距離
	Params params_{};

	float time_ = 0.0f;
	float hitTimer_ = 0.0f;             // 被弾波紋の残り時間
	TuboEngine::Math::Vector3 hitDir_{0.0f, 0.0f, 1.0f}; // 波紋の中心方向
	float breakTimer_ = 0.0f;           // 破壊演出の残り時間
	uint32_t random_ = 12345u;          // 簡易乱数
};
