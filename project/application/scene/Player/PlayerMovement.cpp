#include "PlayerMovement.h"
#include "Input.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace TuboEngine;

void PlayerMovement::Initialize(TuboEngine::Object3d* object) {
    object_ = object;
}

// =========================================
// 通常移動
// =========================================
void PlayerMovement::Move(float speed) {
    Input* input = Input::GetInstance();
    Math::Vector3 pos = object_->GetPosition();
    if (input->PushKey(DIK_D)) pos.x += speed;
    if (input->PushKey(DIK_A)) pos.x -= speed;
    if (input->PushKey(DIK_S)) pos.z -= speed;
    if (input->PushKey(DIK_W)) pos.z += speed;
    object_->SetPosition(pos);
}


void PlayerMovement::Update() {

    // ブースター更新
   // UpdateBooster();

    // ダッシュクールタイム更新
    if (dashCooldownTimer_ > 0) {
        dashCooldownTimer_--;
    }

    // Spaceキーの入力確認
    Input* input = Input::GetInstance();

    bool spacePressed = input->PushKey(DIK_SPACE);

    // Spaceキーを押した瞬間
    if (spacePressed && !spaceWasPressed_) {
        if (!isDashing_ && dashCooldownTimer_ <= 0) {
           // StartDash();
        }
    }

    spaceWasPressed_ = spacePressed;

    // ダッシュ中かどうかで処理を分ける
    if (isDashing_) {
       // UpdateDash();
    } else {
        //Move();
    }
}