#pragma once
#include "JsonManager.h"
#include "Vector3.h"
#include "Vector4.h"
#include <string>
#include <vector>

// =============================================================================
//  BossData
//  Resources/Boss.json に保存するボスのデータ（攻撃ごとの数値は各攻撃クラスが持つ）。
//
//  JSON の形：
//    "common"   : ボス共通の数値（最大HP・コアの角度幅など）
//    "patterns" : 連続技・同時攻撃の定義（攻撃名を並べる）
//    "phases"   : 形態の一覧（上から順に移行する）
//    "attacks"  : 攻撃ごとの数値（キーは攻撃クラスの GetId()）
// =============================================================================

// ボス共通の数値
struct BossCommonData {
	float maxHp = 4000.0f;     // 最大HP
	float coreArcDeg = 120.0f; // コアを狙える角度の幅（真後ろから左右に半分ずつ）
	float faceTolDeg = 10.0f;  // 攻撃を始めてよい向きのずれ（度）
	float faceMaxWait = 1.5f;  // 向きがそろうのを待つ最大時間（秒）
	float firstDelay = 1.5f;   // 戦闘開始から最初の攻撃までの待機時間（秒）
};

// 連続技・同時攻撃の定義
// type : "Sequence"（連続技）/ "Parallel"（同時攻撃。先頭が主）
// attacks : 攻撃名（攻撃クラスの GetId()）を並べる
// ※ 攻撃名はそのまま単発のパターン名としても使える（定義しなくてよい）
struct BossPatternData {
	std::string name;
	std::string type = "Sequence";
	std::vector<std::string> attacks;
};

// 形態の中で使う攻撃（抽選の候補）
struct BossPatternEntry {
	std::string pattern;  // パターン名（攻撃名、または patterns で定義した名前）
	float weight = 1.0f;  // 出やすさ（重み）
	int cooldown = 0;     // 再使用までの回数（出した後、次の○回は選ばれない）
};

// 形態に入るきっかけ
// type : "HpRatio" … ボスのHPが「最大HP × value」以下になったら移行
//        （部位破壊など、きっかけの種類はここに足していく。知らない type では移行しない）
struct BossPhaseTrigger {
	std::string type = "HpRatio";
	float value = 1.0f;
};

// 形態ごとの見た目
struct BossLookData {
	std::string model = "block/block.obj";     // モデル（Resources/Models からの相対パス）
	TuboEngine::Math::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // 色（攻撃の演出の色に掛け合わせる）
	TuboEngine::Math::Vector3 scale = {1.0f, 1.0f, 1.0f};       // 大きさ
};

// 形態
struct BossPhaseData {
	std::string name = "Phase";
	BossPhaseTrigger trigger;     // この形態に入るきっかけ（最初の形態では使わない）
	float transitionTime = 2.0f;  // この形態に入るときの移行演出の長さ（秒。この間ボスは無敵）
	float turnSpeedDeg = 69.0f;   // 旋回の速さ（度/秒）
	float idleTime = 0.25f;       // 攻撃の合間の待機時間（秒）
	BossLookData look;            // 見た目
	std::vector<BossPatternEntry> patterns; // 使う攻撃と重み
};

// ボスのデータ全体（攻撃ごとの数値は除く）
struct BossData {
	BossCommonData common;
	std::vector<BossPatternData> patterns; // 連続技・同時攻撃
	std::vector<BossPhaseData> phases;     // 形態（上から順に移行する）
};

// ---- JSON 変換（項目一覧を1か所に書く） ----
#define BOSS_COMMON_FIELDS(X)     \
	X("maxHp", maxHp)             \
	X("coreArcDeg", coreArcDeg)   \
	X("faceTolDeg", faceTolDeg)   \
	X("faceMaxWait", faceMaxWait) \
	X("firstDelay", firstDelay)
TUBO_JSON_DEFINE(BossCommonData, BOSS_COMMON_FIELDS)

#define BOSS_PATTERN_FIELDS(X) \
	X("name", name)            \
	X("type", type)            \
	X("attacks", attacks)
TUBO_JSON_DEFINE(BossPatternData, BOSS_PATTERN_FIELDS)

#define BOSS_PATTERN_ENTRY_FIELDS(X) \
	X("pattern", pattern)            \
	X("weight", weight)              \
	X("cooldown", cooldown)
TUBO_JSON_DEFINE(BossPatternEntry, BOSS_PATTERN_ENTRY_FIELDS)

#define BOSS_PHASE_TRIGGER_FIELDS(X) \
	X("type", type)                  \
	X("value", value)
TUBO_JSON_DEFINE(BossPhaseTrigger, BOSS_PHASE_TRIGGER_FIELDS)

#define BOSS_LOOK_FIELDS(X) \
	X("model", model)       \
	X("color", color)       \
	X("scale", scale)
TUBO_JSON_DEFINE(BossLookData, BOSS_LOOK_FIELDS)

#define BOSS_PHASE_FIELDS(X)            \
	X("name", name)                     \
	X("trigger", trigger)               \
	X("transitionTime", transitionTime) \
	X("turnSpeedDeg", turnSpeedDeg)     \
	X("idleTime", idleTime)             \
	X("look", look)                     \
	X("patterns", patterns)
TUBO_JSON_DEFINE(BossPhaseData, BOSS_PHASE_FIELDS)
