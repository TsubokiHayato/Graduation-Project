#pragma once
#include "JsonManager.h"
#include "EquipmentData.h"

// JSONファイルのパス
inline constexpr char kEquipmentJsonPath[] = "Resources/Equipment.json";

// JSON全体のデータ
struct EquipmentDataJsonFormat {
    // 初期値
    EquipmentData defaultData;
    // 最新値
    EquipmentData currentData;
};

// 装備データのJSON読み込みクラス
class EquipmentDataJson {
public:
    // JSON読み込み
    bool Load(EquipmentData& data);

    // JSON保存
    // 最新値(current)というパラメータ生成、保存する
    bool Save(const EquipmentData& data);
    // JSON全体を保存
    bool SaveJson();

    // 初期値(default)に戻す
    bool Reset(EquipmentData& data);

    // 最新値が存在するか
    bool HasCurrent() const {
        return hasCurrent_;
    }
private:
	// JSON全体のデータ
    EquipmentDataJsonFormat jsonData_{};
    // JSONに最新値が存在するか
    bool hasCurrent_ = false;
private:
    // EquipmentDataを丸めたJSONに変換
    static TuboEngine::JsonManager::Json ToRoundedJson(const EquipmentData& data);
    // 小数点以下3桁に丸める
    // 戻り値はdoubleにする
    static double Round3(float value) { return std::round(static_cast<double>(value) * 1000.0) / 1000.0; }
};

// =========================================
// シールドデータのJSON変換
// =========================================
#define SHIELD_FIELDS(X) \
    X("shield", shield) \
    X("maxshield", maxshield) \
    X("rechargetime", rechargetime) \
    X("rechargerate", rechargerate)

TUBO_JSON_DEFINE(ShieldData, SHIELD_FIELDS)

// =========================================
// 装備データのJSON変換
// =========================================
#define EQUIPMENT_DATA_FIELDS(X) \
    X("shield", shield)

TUBO_JSON_DEFINE(EquipmentData, EQUIPMENT_DATA_FIELDS)

// =========================================
// JSON全体の変換
// =========================================
#define EQUIPMENT_JSON_FIELDS(X) \
    X("default", defaultData) \
    X("current", currentData)

TUBO_JSON_DEFINE(EquipmentDataJsonFormat, EQUIPMENT_JSON_FIELDS)