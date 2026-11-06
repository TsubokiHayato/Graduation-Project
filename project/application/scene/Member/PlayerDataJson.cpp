#include "PlayerDataJson.h"

using namespace TuboEngine;

// =============================================================================
// JSON読み込み
// =============================================================================
bool PlayerDataJson::Load(PlayerData& data) {

    JsonManager::Json json;
    // ファイルを読み込む
    if (!JsonManager::Load(kPlayerJsonPath, json)) {
        return false;
    }
    // まずdefaultを読み込む
    PlayerData defaultData{};

    if (!JsonManager::Read(json, "default", defaultData)) {
        return false;
    }

    jsonData_.defaultData = defaultData;
    // 最初はdefaultを使用する
    jsonData_.currentData = defaultData;
    // currentが正常に読み込めた場合だけcurrentを優先する
    PlayerData currentData = defaultData;

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
bool PlayerDataJson::Save(const PlayerData& data) {
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
// 初期値(default)に戻す
// =============================================================================
bool PlayerDataJson::Reset(PlayerData& data) {
    // currentが存在しない場合は実行しない
    if (!hasCurrent_) {
        return false;
    }

    // defaultをcurrentに反映
    jsonData_.currentData = jsonData_.defaultData;
    // Playerにも反映
    data = jsonData_.defaultData;
    // JSONにも反映
    return SaveJson();
}

// =============================================================================
// JSONファイルに保存
// =============================================================================
bool PlayerDataJson::SaveJson() {
    using Json = JsonManager::Json;
    Json json;
    // defaultを丸めて保存
    json["default"] = ToRoundedJson(jsonData_.defaultData);
    // currentを丸めて保存
    json["current"] = ToRoundedJson(jsonData_.currentData);
    return JsonManager::Save(kPlayerJsonPath, json);
}

// =============================================================================
// PlayerData全体を小数点以下3桁に丸めたJSONに変換
// =============================================================================
JsonManager::Json PlayerDataJson::ToRoundedJson(const PlayerData& data) {
    using Json = JsonManager::Json;
    Json json;
    // Speed
    json["speed"] = Round3(data.speed);

    // Transform
    json["transform"]["scale"] = {
        Round3(data.transform.scale.x),
        Round3(data.transform.scale.y),
        Round3(data.transform.scale.z)
    };

    json["transform"]["rotate"] = {
        Round3(data.transform.rotate.x),
        Round3(data.transform.rotate.y),
        Round3(data.transform.rotate.z)
    };

    json["transform"]["translate"] = {
        Round3(data.transform.translate.x),
        Round3(data.transform.translate.y),
        Round3(data.transform.translate.z)
    };

    return json;
}