#include "EquipmentDataJson.h"

using namespace TuboEngine;

// =============================================================================
// JSON読み込み
// =============================================================================
bool EquipmentDataJson::Load(EquipmentData& data) {
    JsonManager::Json json;
    // ファイルを読み込む
    if (!JsonManager::Load(kEquipmentJsonPath, json)) {
        return false;
    }

    // まずdefaultを読み込む
    EquipmentData defaultData{};

    if (!JsonManager::Read(json, "default", defaultData)) {
        return false;
    }

    jsonData_.defaultData = defaultData;
    // 最初はdefaultを使用する
    jsonData_.currentData = defaultData;

    // currentが正常に読み込めた場合だけcurrentを優先する
    EquipmentData currentData = defaultData;
    hasCurrent_ = false;
    if (JsonManager::Read(json, "current", currentData)) {
        jsonData_.currentData = currentData;
        hasCurrent_ = true;
    }

    // 最終的なデータを反映
    data = jsonData_.currentData;
    return true;
}

// =============================================================================
// JSON保存
// =============================================================================
bool EquipmentDataJson::Save(const EquipmentData& data) {
    // 保存用のコピーを作成
    jsonData_.currentData = data;
    // JSONに保存
    if (!SaveJson()) {
        return false;
    }
    hasCurrent_ = true;
    return true;
}

// =============================================================================
// 初期値に戻す
// =============================================================================
bool EquipmentDataJson::Reset(EquipmentData& data) {
    // defaultをcurrentに反映
    jsonData_.currentData = jsonData_.defaultData;
    // データに反映
    data = jsonData_.defaultData;
    // JSONにも反映
    return SaveJson();
}

// =============================================================================
// JSONファイルに保存
// =============================================================================
bool EquipmentDataJson::SaveJson() {
    using Json = JsonManager::Json;
    Json json;
    // defaultを保存
    json["default"] = ToRoundedJson(jsonData_.defaultData);
    // currentを保存
    json["current"] = ToRoundedJson(jsonData_.currentData);
    return JsonManager::Save(kEquipmentJsonPath, json);
}

// =============================================================================
// EquipmentData全体を小数点以下3桁に丸めたJSONに変換
// =============================================================================
JsonManager::Json EquipmentDataJson::ToRoundedJson(const EquipmentData& data) {
    using Json = JsonManager::Json;
    Json json;

    // =========================================
    // シールドデータ
    // =========================================
    json["shield"]["shield"] = Round3(data.shield.shield);
    json["shield"]["maxshield"] = Round3(data.shield.maxshield);
    json["shield"]["rechargetime"] = Round3(data.shield.rechargetime);
    json["shield"]["rechargerate"] = Round3(data.shield.rechargerate);

    // =========================================
    // シールドオブジェクト設定
    // =========================================
    json["shield"]["isvisible"] = data.shield.isvisible;

    // Offset    
    json["shield"]["offset"] = TuboEngine::Math::Vector3{
        static_cast<float>(Round3(data.shield.offset.x)),
        static_cast<float>(Round3(data.shield.offset.y)),
        static_cast<float>(Round3(data.shield.offset.z))
    };

    return json;
}