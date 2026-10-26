#include "PlayerData.h"

using namespace TuboEngine;

bool PlayerData::Load(TuboEngine::Object3d* object) {
    // オブジェクトが渡されていない場合
    if (object == nullptr) {
        return false;
    }

    // JSONを読み込む
    if (TuboEngine::JsonManager::LoadObject("Resources/Player.json", objectData_)) {
        // 読み込んだデータを3Dオブジェクトに反映
        object->SetPosition(objectData_.transform.translate);
        object->SetRotation(objectData_.transform.rotate);
        object->SetScale(objectData_.transform.scale);
        return true;
    }

    // 読み込み失敗
    return false;
}