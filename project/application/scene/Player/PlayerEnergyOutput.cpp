#include "PlayerEnergyOutput.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// =============================================================================
//  Initialize
// =============================================================================
void PlayerEnergyOutput::Initialize(const PlayerEnergyData& energyData) {
    energyData_ = energyData;
}

// =============================================================================
//  Update
// =============================================================================
void PlayerEnergyOutput::Update() {

}

// =============================================================================
//  ImGui（Debug ビルドのみ）
// =============================================================================
void PlayerEnergyOutput::DrawImGui() {
#ifdef USE_IMGUI
    // =========================================
    // エネルギー配分
    // =========================================
    if (ImGui::CollapsingHeader("Energy Output", ImGuiTreeNodeFlags_DefaultOpen)) {

        // =========================================
        // エネルギー合計
        // =========================================
        float totalEnergy = energyData_.shield + energyData_.fight + energyData_.beam + energyData_.booster;

        ImGui::Text("Max Energy: %.1f", energyData_.maxenergy);
        ImGui::Text("Total Energy: %.1f / %.1f", totalEnergy, energyData_.maxenergy);

        // =========================================
        // エネルギーゲージ
        // =========================================
        ImGui::Separator();
        ImGui::Text("Energy Distribution");

        float maxEnergy = energyData_.maxenergy;

        if (maxEnergy > 0.0f) {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float width = ImGui::GetContentRegionAvail().x;
            float height = 24.0f;
            const float values[4] = { energyData_.shield, energyData_.fight, energyData_.beam, energyData_.booster };

            const ImU32 colors[4] = {
                IM_COL32(60, 140, 255, 255),
                IM_COL32(255, 100, 80, 255),
                IM_COL32(255, 190, 50, 255),
                IM_COL32(80, 210, 120, 255)
            };

            float currentX = pos.x;

            for (int i = 0; i < 4; i++) {
                float segmentWidth = width * (values[i] / maxEnergy);
                if (segmentWidth > 0.0f) {
                    drawList->AddRectFilled(ImVec2(currentX, pos.y), ImVec2(currentX + segmentWidth, pos.y + height), colors[i]);
                }
                currentX += segmentWidth;
            }

            // 未使用エネルギー部分
            drawList->AddRect(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(200, 200, 200, 255));
            ImGui::Dummy(ImVec2(width, height));
        }

        // =========================================
        // 配分表示
        // =========================================
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.25f, 0.55f, 1.0f, 1.0f), "Shield : %.1f", energyData_.shield);
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "Fight  : %.1f", energyData_.fight);
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Beam : %.1f", energyData_.beam);
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.45f, 1.0f), "Booster: %.1f", energyData_.booster);

        // =========================================
        // エネルギー入力
        // =========================================
        ImGui::Separator();
        ImGui::Text("Adjust Energy");

        // 入力前の値を保存
        float oldShield = energyData_.shield;
        float oldFight = energyData_.fight;
        float oldBeam = energyData_.beam;
        float oldBooster = energyData_.booster;

        // 入力
        bool shieldChanged = ImGui::InputFloat("Shield", &energyData_.shield, 0.1f, 1.0f, "%.1f");
        bool fightChanged = ImGui::InputFloat("Fight", &energyData_.fight, 0.1f, 1.0f, "%.1f");
        bool beamChanged = ImGui::InputFloat("Beam", &energyData_.beam, 0.1f, 1.0f, "%.1f");
        bool boosterChanged = ImGui::InputFloat("Booster", &energyData_.booster, 0.1f, 1.0f, "%.1f");

        // =========================================
        // 合計値のチェック
        // =========================================
        if (shieldChanged || fightChanged || beamChanged || boosterChanged) {
            float newTotal = energyData_.shield + energyData_.fight + energyData_.beam + energyData_.booster;
            // 最大エネルギーを超えた場合は変更を取り消す
            if (newTotal > energyData_.maxenergy) {
                energyData_.shield = oldShield;
                energyData_.fight = oldFight;
                energyData_.beam = oldBeam;
                energyData_.booster = oldBooster;
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Energy limit exceeded!");
            }
        }
    }

#endif
}