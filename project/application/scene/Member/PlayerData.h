#pragma once
#include "Object3d.h"
#include "JsonManager.h"

class PlayerData {
public:
	// プレイヤーのオブジェクトデータ
	struct ObjectData {
		TuboEngine::Transform transform;
	};
public:
	// JSONを読み込み、指定された3Dオブジェクトに反映する
	bool Load(TuboEngine::Object3d* object);
	// 読み込んだデータを取得したい場合に使用
	const ObjectData& GetObjectData() const {
		return objectData_;
	}

private:
	ObjectData objectData_{};
};


// TransformのJSON変換定義
#define PLAYER_TRANSFORM_FIELDS(X) \
    X("scale", scale)              \
    X("rotate", rotate)            \
    X("translate", translate)

namespace TuboEngine {
	TUBO_JSON_DEFINE(Transform, PLAYER_TRANSFORM_FIELDS)
}

// ObjectDataのJSON変換定義
#define PLAYER_OBJECT_DATA_FIELDS(X) \
    X("transform", transform)

TUBO_JSON_DEFINE(PlayerData::ObjectData, PLAYER_OBJECT_DATA_FIELDS)