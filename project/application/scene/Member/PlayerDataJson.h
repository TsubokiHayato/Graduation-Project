#pragma once
#include "JsonManager.h"
#include "PlayerData.h"

// JSONファイルのパス
inline constexpr char kPlayerJsonPath[] = "Resources/Player.json";

// JSON全体のデータ
struct PlayerDataJsonFormat {
    // 初期値
    PlayerData defaultData;
    // 最新値
    PlayerData currentData;
};

// プレイヤ―のパラメータJson読み込みクラス
class PlayerDataJson {
public:
    // JSON読み込み
    bool Load(PlayerData& data);

    // JSON保存
    // 最新値(current)というパラメータ生成、保存する
    bool Save(const PlayerData& data);
    // JSON全体を保存
    bool SaveJson();

    // 初期値(default)に戻す
    bool Reset(PlayerData& data);

    // 最新値が存在するか
    bool HasCurrent() const {
        return hasCurrent_;
    }
private:
	// JSON全体のデータ
    PlayerDataJsonFormat jsonData_{};
    // JSONに最新値が存在するか
    bool hasCurrent_ = false;
private:
    // PlayerDataを丸めたJSONに変換
    static TuboEngine::JsonManager::Json ToRoundedJson(const PlayerData& data);
    // 小数点以下3桁に丸める
    // 戻り値はdoubleにする
    static double Round3(float value) { return std::round(static_cast<double>(value) * 1000.0) / 1000.0; }
};
// JSONに変換するためのマクロ(Transform)
#define PLAYER_TRANSFORM_FIELDS(X) \
    X("scale", scale)              \
    X("rotate", rotate)            \
    X("translate", translate)

namespace TuboEngine { TUBO_JSON_DEFINE(Transform, PLAYER_TRANSFORM_FIELDS) }

// ★各パラメータの項目。増やしたらここに1行足す（保存・読み込みが使う）
#define PLAYER_DATA_FIELDS(X) \
    X("transform", transform) \
    X("speed", speed)

TUBO_JSON_DEFINE(PlayerData, PLAYER_DATA_FIELDS)
// 初期値と最新値の2つを持つJSONの項目一覧
#define PLAYER_JSON_FIELDS(X)       \
    X("default", defaultData)       \
    X("current", currentData)

TUBO_JSON_DEFINE(PlayerDataJsonFormat, PLAYER_JSON_FIELDS)