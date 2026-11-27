#pragma once
#include "JsonManager.h"
#include "Vector3.h"
#include "Vector4.h"
#include <map>
#include <string>
#include <vector>

// =============================================================================
//  BossData
//  Resources/Boss.json に保存するボスのデータ（攻撃ごとの数値は各攻撃クラスが持つ）。
//
//  JSON の形：
//    "common"   : ボス共通の数値（最大HP・コアの角度幅など）
//    "patterns" : 攻撃パターン（タイムライン。BossEdit シーンで作る）
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
	std::string defeatCue = "Defeated"; // 撃破したときに出す演出の合図の名前
};

// タイムラインに置いた攻撃1つ
struct BossTimelineClip {
	std::string attack;        // 攻撃名（攻撃クラスの GetId()）
	float start = 0.0f;        // パターンの始まりからの開始時刻（秒）
	int track = 0;             // タイムラインの段（表示用。同じ時刻に重ねるときに段を分ける）
	bool controlsBody = true;  // ボスの向き・高さ・色を動かすか（重なる攻撃のうち1つだけにする）
	// 攻撃の標準の数値から変えた項目だけ（例：{"params": {"trackingTime": 2.5}}）。無い項目は標準の数値を使う
	TuboEngine::JsonManager::Json overrides = TuboEngine::JsonManager::Json::object();
};

// 攻撃パターン（タイムライン）。BossEdit シーンで作る
// 硬直はパターンの最後に1回だけ：最後に終わる攻撃が recoveryTime 秒の硬直に入る
struct BossPatternData {
	std::string name;
	float recoveryTime = 1.5f;            // パターンの最後の硬直（秒）
	std::vector<BossTimelineClip> clips;  // 置いた攻撃

	// ---- 古い形式（連続技・同時攻撃）。読み込み時にタイムラインへ変換する。保存はしない ----
	std::string legacyType;               // "Sequence" / "Parallel"
	std::vector<std::string> legacyAttacks;
};

// 形態の中で使う攻撃（抽選の候補）
struct BossPatternEntry {
	std::string pattern;  // パターン名（攻撃名、または patterns で作ったタイムラインの名前）
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

// コア（背中の弱点）の見た目
struct BossCoreLook {
	bool visible = true;
	std::string model = "boss_core/boss_core.obj";   // ボス・自機と共有しないコア専用のモデル
	// ボスの中心からの位置（ボスの向きで回る。x = 右、y = 上、z = 前。背中は -z）。ボスの大きさに合わせて伸びる
	TuboEngine::Math::Vector3 offset = {0.0f, 1.0f, -1.15f};
	float scale = 0.35f;
	TuboEngine::Math::Vector4 color = {0.2f, 1.0f, 1.0f, 1.0f};         // 通常の色
	TuboEngine::Math::Vector4 recoveryColor = {1.0f, 1.0f, 0.3f, 1.0f}; // 硬直中に点滅する色
	float blinkHz = 4.0f;          // 硬直中の点滅の速さ（回/秒）
	bool showArcLines = true;      // 狙える範囲を線で示すか
};

// 形態ごとの見た目
struct BossLookData {
	std::string model = "boss_block/boss_block.obj"; // モデル（Resources/Models からの相対パス。自機などと共有しないボス専用のもの）
	std::string texture;                             // テクスチャ（Resources/Textures からの相対パス。空ならモデルのテクスチャ）
	TuboEngine::Math::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // 体の色（攻撃の演出の色に掛け合わせる）
	TuboEngine::Math::Vector3 scale = {1.0f, 1.0f, 1.0f};       // 大きさ
	// ---- モデルの補正 ----
	TuboEngine::Math::Vector3 offset = {0.0f, 1.0f, 0.0f};      // 位置のずれ（ボスの向きで回り、大きさに合わせて伸びる。block は高さ1で地面に乗る）
	TuboEngine::Math::Vector3 rotationDeg = {0.0f, 0.0f, 0.0f}; // 向きの補正（度。前が +Z を向いていないモデル用）
	// ---- 材質 ----
	float shininess = 70.0f;              // 光沢
	float environmentCoefficient = 1.0f;  // 環境の映り込み
	// ---- 攻撃の演出 ----
	TuboEngine::Math::Vector4 effectTint = {1.0f, 1.0f, 1.0f, 1.0f}; // 攻撃範囲・弾・ビームの線の色に掛け合わせる色
	BossCoreLook core;
};

// 移行演出（この形態に入るとき）
struct BossTransitionData {
	float riseHeight = 1.5f;     // 浮き上がる高さ
	TuboEngine::Math::Vector4 glowColor = {1.0f, 0.2f, 0.9f, 1.0f}; // 光る色
	float blinkStartHz = 2.0f;   // 点滅の速さ（始まり）
	float blinkEndHz = 12.0f;    // 点滅の速さ（終わり）
	float lookSwitchRatio = 0.5f; // 見た目を新しい形態に切り替える時点（演出の長さに対する割合 0〜1）
	std::string startCue;        // 移行演出の始まりに出す演出の合図の名前（ライティングなどの制御側が使う）
	std::string endCue;          // 移行演出の終わりに出す合図の名前
};

// 攻撃ごとのアニメーション名
struct BossAttackAnimation {
	std::string telegraph;
	std::string attack;
	std::string recovery;
};

// アニメーション名の対応（空ならアニメーションの指示を出さない）
struct BossAnimationMap {
	std::string idle;
	std::string transition;
	std::string defeated;
	std::map<std::string, BossAttackAnimation> attacks; // キーは攻撃名（攻撃クラスの GetId()）
};

// 形態
struct BossPhaseData {
	std::string name = "Phase";
	BossPhaseTrigger trigger;     // この形態に入るきっかけ（最初の形態では使わない）
	float transitionTime = 2.0f;  // この形態に入るときの移行演出の長さ（秒。この間ボスは無敵）
	BossTransitionData transition; // 移行演出の中身
	float turnSpeedDeg = 69.0f;   // 旋回の速さ（度/秒）
	float idleTime = 0.25f;       // 攻撃の合間の待機時間（秒）
	BossLookData look;            // 見た目
	BossAnimationMap animations;  // アニメーション名の対応
	std::vector<BossPatternEntry> patterns; // 使う攻撃と重み
};

// ボスのデータ全体（攻撃ごとの数値は除く）
struct BossData {
	BossCommonData common;
	std::vector<BossPatternData> patterns; // 攻撃パターン（タイムライン）
	std::vector<BossPhaseData> phases;     // 形態（上から順に移行する）
};

// ---- JSON 変換（項目一覧を1か所に書く） ----
#define BOSS_COMMON_FIELDS(X)     \
	X("maxHp", maxHp)             \
	X("coreArcDeg", coreArcDeg)   \
	X("faceTolDeg", faceTolDeg)   \
	X("faceMaxWait", faceMaxWait) \
	X("firstDelay", firstDelay)   \
	X("defeatCue", defeatCue)
TUBO_JSON_DEFINE(BossCommonData, BOSS_COMMON_FIELDS)

// overrides は JSON をそのまま持つので手で書く
inline void to_json(nlohmann::json& j, const BossTimelineClip& value) {
	j = nlohmann::json::object();
	j["attack"] = value.attack;
	j["start"] = value.start;
	j["track"] = value.track;
	j["controlsBody"] = value.controlsBody;
	j["overrides"] = value.overrides.is_object() ? value.overrides : nlohmann::json::object();
}
inline void from_json(const nlohmann::json& j, BossTimelineClip& value) {
	TuboEngine::JsonManager::Read(j, "attack", value.attack);
	TuboEngine::JsonManager::Read(j, "start", value.start);
	TuboEngine::JsonManager::Read(j, "track", value.track);
	TuboEngine::JsonManager::Read(j, "controlsBody", value.controlsBody);
	if (j.is_object() && j.contains("overrides") && j["overrides"].is_object()) {
		value.overrides = j["overrides"];
	}
}

// パターンは古い形式も読むため、手で書く（保存はタイムラインの形式だけ）
inline void to_json(nlohmann::json& j, const BossPatternData& value) {
	j = nlohmann::json::object();
	j["name"] = value.name;
	j["recoveryTime"] = value.recoveryTime;
	j["clips"] = value.clips;
}
inline void from_json(const nlohmann::json& j, BossPatternData& value) {
	TuboEngine::JsonManager::Read(j, "name", value.name);
	TuboEngine::JsonManager::Read(j, "recoveryTime", value.recoveryTime);
	TuboEngine::JsonManager::Read(j, "clips", value.clips);
	TuboEngine::JsonManager::Read(j, "type", value.legacyType);
	TuboEngine::JsonManager::Read(j, "attacks", value.legacyAttacks);
}

#define BOSS_PATTERN_ENTRY_FIELDS(X) \
	X("pattern", pattern)            \
	X("weight", weight)              \
	X("cooldown", cooldown)
TUBO_JSON_DEFINE(BossPatternEntry, BOSS_PATTERN_ENTRY_FIELDS)

#define BOSS_PHASE_TRIGGER_FIELDS(X) \
	X("type", type)                  \
	X("value", value)
TUBO_JSON_DEFINE(BossPhaseTrigger, BOSS_PHASE_TRIGGER_FIELDS)

#define BOSS_CORE_LOOK_FIELDS(X)    \
	X("visible", visible)           \
	X("model", model)               \
	X("offset", offset)             \
	X("scale", scale)               \
	X("color", color)               \
	X("recoveryColor", recoveryColor) \
	X("blinkHz", blinkHz)           \
	X("showArcLines", showArcLines)
TUBO_JSON_DEFINE(BossCoreLook, BOSS_CORE_LOOK_FIELDS)

#define BOSS_LOOK_FIELDS(X)                          \
	X("model", model)                                \
	X("texture", texture)                            \
	X("color", color)                                \
	X("scale", scale)                                \
	X("offset", offset)                              \
	X("rotationDeg", rotationDeg)                    \
	X("shininess", shininess)                        \
	X("environmentCoefficient", environmentCoefficient) \
	X("effectTint", effectTint)                      \
	X("core", core)
TUBO_JSON_DEFINE(BossLookData, BOSS_LOOK_FIELDS)

#define BOSS_TRANSITION_FIELDS(X)         \
	X("riseHeight", riseHeight)           \
	X("glowColor", glowColor)             \
	X("blinkStartHz", blinkStartHz)       \
	X("blinkEndHz", blinkEndHz)           \
	X("lookSwitchRatio", lookSwitchRatio) \
	X("startCue", startCue)               \
	X("endCue", endCue)
TUBO_JSON_DEFINE(BossTransitionData, BOSS_TRANSITION_FIELDS)

#define BOSS_ATTACK_ANIMATION_FIELDS(X) \
	X("telegraph", telegraph)           \
	X("attack", attack)                 \
	X("recovery", recovery)
TUBO_JSON_DEFINE(BossAttackAnimation, BOSS_ATTACK_ANIMATION_FIELDS)

#define BOSS_ANIMATION_MAP_FIELDS(X) \
	X("idle", idle)                  \
	X("transition", transition)      \
	X("defeated", defeated)          \
	X("attacks", attacks)
TUBO_JSON_DEFINE(BossAnimationMap, BOSS_ANIMATION_MAP_FIELDS)

#define BOSS_PHASE_FIELDS(X)            \
	X("name", name)                     \
	X("trigger", trigger)               \
	X("transitionTime", transitionTime) \
	X("transition", transition)         \
	X("turnSpeedDeg", turnSpeedDeg)     \
	X("idleTime", idleTime)             \
	X("look", look)                     \
	X("animations", animations)         \
	X("patterns", patterns)
TUBO_JSON_DEFINE(BossPhaseData, BOSS_PHASE_FIELDS)
