# TuboEngine 改良予定表 ＆ 実装済み機能ガイド

作成日: 2026-10-23　対象: `project/EngineCore`（ブランチ `Ver6.0_Feature_DebugLayOut` / c18990a）＋ ゲーム側 `project/application`

このドキュメントは 3 部構成です。

1. [改良予定表](#1-改良予定表) … これからやること・順番・目安の日程
2. [実装済み機能の仕組みと使い方](#2-実装済み機能の仕組みと使い方)
3. [バグになりそうな箇所](#3-バグになりそうな箇所)

---

## 1. 改良予定表

### 1-1. 一覧

> 状態: ✅ 実装済み / 🟡 一部実装済み / ⬜ 未着手　　日数は 1 人で作業した場合の目安

| # | 項目 | 状態 | 目安 | 依存 |
|---|---|---|---|---|
| 0 | **Time（deltaTime）クラス** を作る ※下の項目の土台 | ✅（`GameTime.h`） | 0.5 日 | — |
| 1 | **マウスのウィンドウ固定を切り替え可能にする** | ✅（`WinApp::SetCursorMode`） | 0.5〜1 日 | — |
| 2 | **イージング関数をまとめる**（`Easing.h` ＋ Tween） | ✅（`Easing.h` / `Tween.h`） | 1 日 | 0 |
| 3 | **JsonManager を作る**（読み書き・Vector 変換・エラーログ） | ✅（`JsonManager.h`） | 1.5 日 | — |
| 4 | **ParticleManager の JSON 処理を分離**（3 の適用第 1 弾） | ✅（Settings / TextManager も移行済み） | 1 日 | 3 |
| 5 | **アニメーション** | ✅（イベント・ステートマシン・キャッシュ・ボーン取得を追加。複数メッシュは未対応） | 2〜3 日（残り） | 0, 2 |
| 6 | **ImGui の UI を Unity 風にする**（Debug のみ） | 🟡（レイアウト・Hierarchy・Project・Console 実装済み） | 2〜3 日（残り） | 3 |
| B | **バグ修正**（[3 章](#3-バグになりそうな箇所) の「高」） | ✅（#1〜#4 修正済み） | 1 日 | — |

> 1〜3 週目の分（0〜5・B）は EngineCore ブランチ `Ver7.0_Feature_EngineImprove` / 親リポジトリ ブランチ `Ver1.1_Feature_EngineImprove` で実装済み。使い方は [2-10](#2-10-ver70-で追加したもの)。

### 1-2. 日程（目安）

| 週 | 期間 | やること |
|---|---|---|
| 1 週目 | 10/26（月）〜 10/30（金） | **B**（高優先度バグ）→ **0** Time → **1** マウス固定切り替え → **2** Easing |
| 2 週目 | 11/2（月）〜 11/6（金） | **3** JsonManager → **4** Particle 移行 → Settings / TextManager も JsonManager へ |
| 3 週目 | 11/9（月）〜 11/13（金） | **5** アニメーションの残り（イベント・ステート・読み込みキャッシュ・UI 用 Tween） |
| 4 週目 | 11/16（月）〜 11/20（金） | **6** Unity 風 UI の残り（オブジェクト Inspector・Gizmo）／予備日 |

順番の理由: Time が無いとイージングとアニメーションの速度がフレームレートに依存したままになる。JsonManager を先に作ると、6 の「Inspector の値を保存する」にそのまま使える。

---

### 1-3. 各項目の設計案

#### 0. Time（deltaTime）

今は `1.0f / 60.0f` がコードのあちこちに直接書かれている（TsubokiScene / TitleScene / SampleScene / Particle / VHSEffect）。
フレーム落ちするとゲーム全体が遅くなるので、1 か所で計測して配る。

```cpp
// engine/base/Utilities/Time.h（案）
namespace TuboEngine {
class Time {
public:
	static void Update();               // Framework::Update の先頭で 1 回呼ぶ
	static float DeltaTime();           // 前フレームからの秒（上限 0.1 秒でクランプ）
	static float UnscaledDeltaTime();   // timeScale の影響を受けない（UI 演出用）
	static void SetTimeScale(float s);  // 0 で一時停止、0.5 でスロー
	static float TotalTime();
};
}
```

- エディタの一時停止中は `DeltaTime() == 0` にすると、ツールバーと自然に連動する。

#### 1. マウスのウィンドウ固定の切り替え

**現状**: `Framework::Update()` が毎フレーム `WinApp::ToggleCursorClip(前面 && !ImGui操作中)` を呼んでいて、**オフにする手段が無い**（[Framework.cpp:79](../project/EngineCore/engine/Framework/Framework.cpp)）。

**案**: 「モード」を持たせて、ゲーム側・キー・Settings から切り替える。

```cpp
// WinApp.h（案）
enum class CursorMode {
	Free,     // 固定しない（タイトル・メニュー・デバッグ時）
	Confined, // ウィンドウ内に閉じ込める（今の動作）
	Locked,   // 中央に固定＋非表示（FPS 視点カメラ用。移動量は Input::GetMouseMove で取る）
};
void SetCursorMode(CursorMode mode);
CursorMode GetCursorMode() const;
```

実装のポイント:
- `ClipCursor` は Alt+Tab やウィンドウ移動で Windows 側に解除されるので、**毎フレーム呼ぶ今の方式は維持**しつつ、モードが `Free` のときだけ `ClipCursor(nullptr)` にする。
- Debug エディタ表示中は、閉じ込める範囲を **Game ウィンドウの画像の範囲** にする（`ImGuiManager::DrawGameWindow` で計算済みの `imageMin/imageMax` を WinApp に渡す）。
- 切り替えキー: **Esc で一時的に Free**、ゲーム画面をクリックで元のモードに戻す（Unity の Play モードと同じ感覚）。
- `Locked` のときは `ShowCursor(FALSE)` と `SetCursorPos(中央)` を毎フレーム。`ShowCursor` は**呼んだ回数で数えるカウンタ式**なので、切り替え時に 1 回だけ呼ぶこと（毎フレーム呼ぶと戻らなくなる）。
- Settings に `cursorConfine` を追加すればオプション画面から変更可能。

#### 2. イージング関数まとめ

**現状**: `TitleScene.cpp` の無名名前空間に `EaseOutCubic` と `Clamp01` があるだけ。他の人は使えない。

**案**: `engine/math/Easing.h`（ヘッダのみ）

```cpp
namespace TuboEngine::Easing {
enum class Type {
	Linear,
	InSine, OutSine, InOutSine,
	InQuad, OutQuad, InOutQuad,
	InCubic, OutCubic, InOutCubic,
	InQuart, OutQuart, InOutQuart,
	InExpo, OutExpo, InOutExpo,
	InBack, OutBack, InOutBack,
	InElastic, OutElastic,
	InBounce, OutBounce, InOutBounce,
};

float OutCubic(float t);                // 個別関数（t は 0〜1、内部でクランプ）
float Apply(Type type, float t);        // 種類を変数で切り替えたいとき（JSON/ImGui から選べる）
const char* ToString(Type type);        // ImGui の Combo / JSON 保存用
Type FromString(const std::string& s);

// 値の補間（float / Vector2 / Vector3 / Vector4 に対応）
template <class T>
T Ease(const T& start, const T& end, float t, Type type) {
	return start + (end - start) * Apply(type, t);
}
}
```

さらに演出用に **Tween**（時間を自分で数えなくてよくするクラス）を用意する:

```cpp
TuboEngine::Tween<Vector2> slideIn;
slideIn.Start({-300, 100}, {100, 100}, 0.4f, Easing::Type::OutBack, /*delay*/0.1f);
// Update 内
sprite->SetPosition(slideIn.Update(Time::DeltaTime()));
if (slideIn.IsFinished()) { ... }
```

- ImGui 用に「カーブのプレビューを描く関数」を付けると、パラメータ調整がしやすい（6 の Unity 風 UI と相性が良い）。
- 移行: TitleScene の `EaseOutCubic` を `Easing::OutCubic` に置き換える。

#### 3. JsonManager

**現状**: `ParticleManager` / `TextManager` / `Settings` がそれぞれ `nlohmann::json` を直接使い、
- ファイルを開く・パースする・失敗時の処理
- `Vector3` ⇔ `[x, y, z]` の変換

を毎回手で書いている。Particle だけで**同じ変換コードが 4 か所**あり、実際に項目の書き忘れが起きている（[3 章 #1](#3-バグになりそうな箇所)）。

**案**: 2 つに分ける。

**(a) 型と JSON の変換（`engine/base/Json/JsonConvert.h`）**

```cpp
#include <externals/nlohmann/json.hpp>
#include "Vector2.h" "Vector3.h" "Vector4.h"

namespace TuboEngine::Math {
// これを書いておくと j["pos"] = vec3; / vec3 = j["pos"]; がそのまま使える
inline void to_json(nlohmann::json& j, const Vector3& v) { j = {v.x, v.y, v.z}; }
inline void from_json(const nlohmann::json& j, Vector3& v) {
	if (j.is_array() && j.size() == 3) { v = {j[0], j[1], j[2]}; }
}
// Vector2 / Vector4 も同様
}
```

構造体は `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` で **項目を 1 回書くだけ** にする:

> **実装時の変更**: 同梱の nlohmann が 3.7 でこのマクロ（3.9 以降）が無かったため、同じ働きをする `TUBO_JSON_DEFINE` を自作した（[2-10](#2-10-ver70-で追加したもの)）。

```cpp
// ParticlePresetJson.h（Particle の JSON 定義はここだけ）
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParticlePreset,
	name, texture, maxInstances, billboard, center,
	posMin, posMax, velMin, velMax, scaleMin, scaleMax, colMin, colMax,
	lifeMin, lifeMax, autoEmit, emitRate, burstCount,
	gravity, drag, rotSpeedRangeZ, initialRotRangeZ,
	scaleStart, scaleEnd, colorStart, colorEnd,
	blendModeOverride, simulateInWorldSpace)
```

`_WITH_DEFAULT` 版はキーが無いとき既定値を使うので、**古い JSON もそのまま読める**。

**(b) ファイル入出力（`JsonManager`）**

```cpp
namespace TuboEngine {
class JsonManager {
public:
	// 成功で true。失敗時は Logger に "[Json] 読み込み失敗: パス (理由)" を出す（Console に赤で出る）
	static bool Load(const std::string& path, nlohmann::json& out);
	static bool Save(const std::string& path, const nlohmann::json& j, int indent = 2);

	// 型を直接読み書きするショートカット
	template <class T> static bool Load(const std::string& path, T& out);
	template <class T> static bool Save(const std::string& path, const T& value);
};
}
```

実装のポイント:
- **保存は「一時ファイルに書く → 置き換え」**（書き込み途中で落ちても元ファイルが壊れない）。
- `Resources/` からの相対パスは**実行ファイルの場所を基準に解決**する（[3 章 #10](#3-バグになりそうな箇所)）。
- 余裕があれば「ファイル更新を検知して自動再読み込み」（Debug のみ）。

#### 4. ParticleManager の JSON 分離

1. `ParticlePresetJson.h` を作り、上の定義を書く（エミッター固有値 `spinSpeedY` などは `type` と一緒に別キーで保存）。
2. `BuildSnapshotJson` / `ApplySnapshot` / `LoadAll` / `SaveSelected` / `LoadMerge` の手書き変換を `j = preset;` / `preset = j.get<ParticlePreset>();` に置き換え。
3. ファイル入出力を `JsonManager::Load/Save` に置き換え。
4. `all.json` を読み込み → 保存 → 再読み込みして値が変わらないことを確認。

→ これで 3 章 #1 のバグも同時に直る。続けて `Settings`、`TextManager` も同じ方式にする。

#### 5. アニメーション（残り作業）

基本機能は実装済み（[2-6](#2-6-アニメーションanimator)）。残りは次の通り。

| 内容 | 理由 |
|---|---|
| `Update()` の既定引数 `1/60` をやめ `Time::DeltaTime()` を使う | フレームレート依存をなくす |
| **読み込みキャッシュ**（同じファイルを Assimp で何度も読まない） | `Initialize("human/walk.gltf")` ＋ `AddAnimation("Walk", "human/walk.gltf")` で同じファイルを 3 回パースしている |
| **アニメーションイベント**（「0.3 秒で足音」「終了時コールバック」） | SE・エフェクト・攻撃判定のタイミング合わせ |
| **簡易ステートマシン**（Idle ⇔ Walk ⇔ Run を条件で遷移） | シーン側の `if` が増えすぎないように。遷移表は JSON に置く（3 を使う） |
| **ボーンにアタッチ**（`GetJointWorldMatrix("RightHand")`） | 武器・エフェクトを手に持たせる |
| 複数メッシュのノードアニメーション | 今はメッシュ 1 個分の行列しか反映しない |
| Sprite / UI 用アニメーション | 2 の Tween で対応（スプライトシート再生も追加候補） |

#### 6. Unity 風 UI（残り作業、Debug のみ）

レイアウト・Hierarchy・Inspector（情報表示）・Project・Console・再生ツールバーは実装済み（[docs/DebugEditor.md](DebugEditor.md)）。
足りないのは **「オブジェクトを選んで値をいじる」** 部分。

| 内容 | 概要 |
|---|---|
| **Inspector への登録の仕組み** | `IInspectable { virtual const char* GetName(); virtual void OnInspectorGUI(); }` を作り、Object3d / Animator / Emitter / Camera を登録すると Hierarchy に並び、選ぶと Inspector に Transform などが出る |
| **Gizmo**（ImGuizmo） | Game ウィンドウ上で移動・回転・拡大をドラッグ操作 |
| 値の保存 | Inspector の値を JsonManager でシーンごとに保存・読み込み |
| ショートカットの整理 | Ctrl+Shift+P と Ctrl+Alt+P は同時押しで両方反応する（[3 章 #12](#3-バグになりそうな箇所)） |
| Console の改良 | ログに種類（Info/Warning/Error）を持たせる。今は文字列に「error」等が含まれるかで判定 |

---

## 2. 実装済み機能の仕組みと使い方

### 2-1. 全体の流れ（Framework / Order）

- `main.cpp` → `Order`（`Framework` を継承）の `Run()` が **Initialize → (Update → Draw) の繰り返し → Finalize** を回す。
- `Framework::Initialize` でエンジンの各シングルトンを初期化、`Order::Initialize` でゲームのシーン登録と開始シーンの指定を行う。
- 描画順（`Order::Draw`）: オフスクリーンへ 3D → スプライト → ライン → パーティクル → ポストエフェクトで最終画面 → ImGui。
- FPS は `DirectXCommon` の `UpdateFixFPS` で 60 に固定（＋ VSync）。

**開始シーンを変えるとき**: [Order.cpp](../project/application/Order.cpp) の `SceneManager::GetInstance()->Initialize(TSUBOKI);` を変更。

### 2-2. シーン（SceneManager / IScene）

- シーンは `IScene` を継承し、`Initialize / Update / Finalize / Object3DDraw / SpriteDraw / ParticleDraw / ImGuiDraw / GetMainCamera` を実装する。
- シーン番号 → 生成関数 の表を持っていて、`ChangeScene(番号)` で **次の Update の頭** で切り替わる（描画途中で切り替わらない）。同じ番号を渡すとリロード。

**シーンを追加する手順**
1. `application/scene/GameScenes.h` の enum に番号を追加
2. シーンクラスを作る
3. [SceneRegistration.cpp](../project/application/scene/SceneRegistration.cpp) に `sm->RegisterScene(番号, []{ return std::make_unique<新シーン>(); }, "表示名");`

```cpp
SceneManager::GetInstance()->ChangeScene(TITLE); // 遷移
```

### 2-3. 入力（Input）

- DirectInput（キーボード・マウス・パッド）＋ XInput。`Framework::Update` で毎フレーム更新される。

```cpp
auto* in = TuboEngine::Input::GetInstance();
in->PushKey(DIK_SPACE);      // 押している間
in->TriggerKey(DIK_SPACE);   // 押した瞬間
in->IsTriggerMouse(0);       // 0=左 1=右 2=中
in->GetMousePosition();      // ゲーム解像度(1280x720)の座標。Debug の Game ウィンドウ表示中も自動補正
in->GetMouseMove();          // 移動量（カメラ操作用）
XINPUT_STATE pad;
if (in->GetJoystickState(0, pad)) { ... } // ※ 3 章 #2 の注意あり
in->FlushTriggers();         // ポーズ解除直後などに「押した瞬間」判定を消す
```

### 2-4. マウスのウィンドウ固定（Ver6.0 時点。Ver7.0 での変更は [2-10](#2-10-ver70-で追加したもの)）

- [WinApp.cpp:96](../project/EngineCore/engine/base/Common/WinApp.cpp) `ToggleCursorClip(bool)` が `ClipCursor` でクライアント領域に閉じ込める。
- [Framework.cpp:79](../project/EngineCore/engine/Framework/Framework.cpp) が毎フレーム「ウィンドウが前面 かつ ImGui がマウスを使っていない」ときに ON にしている。
- 結果として **ゲームウィンドウが前面にある間は常に閉じ込められる**（ImGui のウィンドウ上にカーソルがあるときだけ外れる）。切り替える API は無い → 1-3 の 1 で対応。

### 2-5. デバッグエディタ（ImGuiManager / EditorPanels）

詳しい操作は [docs/DebugEditor.md](DebugEditor.md)。仕組みだけ書くと:

- `USE_IMGUI` が定義されているビルドだけ有効。ImGui は docking 版。
- ゲームの最終画面をテクスチャに描き、それを「Game」ウィンドウ内に `AddImage` で表示している。マウス座標は `Input::SetMouseRemap` でゲーム解像度に変換。
- 一時停止は `ImGuiManager::ConsumeSceneUpdate()` が false を返して **`SceneManager::Update()` だけ止める**（描画・入力・ポストエフェクトは動く）。
- 配色は `ApplyUnityStyle()`、初期配置は `BuildDefaultLayout()`、配置は `imgui.ini` に保存。

**自分のデバッグウィンドウを作るとき**（Window メニュー・Hierarchy から表示切り替えできる）:

```cpp
#ifdef USE_IMGUI
auto* gui = TuboEngine::ImGuiManager::GetInstance();
if (gui->BeginPanel("Player")) {
	ImGui::DragFloat3("Position", &pos_.x, 0.1f);
}
gui->EndPanel(); // if の外で必ず呼ぶ
#endif
```

### 2-6. アニメーション（Animator）

- `Animator` = `Object3d` ＋ `Skeleton` ＋（ボーンがあれば）`SkinCluster`。
  - ボーンありのモデル → スキニング（頂点シェーダでマトリックスパレット変形）
  - ボーン無しのモデル → ノードアニメーション（メッシュのノード行列でモデル全体を動かす）
- glTF を Assimp で読み、右手系→左手系に変換（位置は x 反転、回転は y・z 反転）。
- キーフレーム補間: 位置・スケールは線形、回転は Slerp。
- `PlayAnimation(名前, blendTime)` で前のアニメーションから **クロスフェード**。
- `CapturePose` で今の姿勢を 1 フレームのアニメーションとして保存できる（TsubokiScene の待機ポーズで使用）。

```cpp
human_ = std::make_unique<TuboEngine::Animator>();
human_->Initialize("human/walk.gltf");                 // Resources/Models からの相対パス。中のアニメは全部登録される
human_->SetCamera(camera_.get());
human_->AddAnimation("Walk", "human/walk.gltf");        // 別名で登録（ファイル間で名前が重なるとき）
human_->AddAnimation("SneakWalk", "human/sneakWalk.gltf");
human_->PlayAnimation("Walk", 0.2f);                    // 0.2 秒かけて切り替え

// Update
human_->Update(1.0f / 60.0f);
// Draw（スキニングモデルは描画設定が別）
if (human_->IsSkinned()) { Object3dCommon::GetInstance()->DrawSettingsSkinning(); }
human_->Draw();
// ImGui
human_->DrawImGui("Human");   // 再生/停止・速度・時間・アニメ切り替え
```

その他: `SetLoop / SetSpeed（負で逆再生）/ IsFinished / SetAnimationTime / GetAnimationNames`。

### 2-7. パーティクル（ParticleManager）

- `ParticlePreset`（発生数・寿命・速度・色・重力など）＋ エミッターの種類（`Default / Primitive / Ring / Cylinder / Original / OrbitTrail / Aura`）。
- 種類は文字列 → 生成関数の表（`RegisterDefaultEmitters`）で管理。
- 起動時（最初の `GetInstance()`）に `Resources/Particles/all.json` を自動で読み込む。
- ImGui で作成・編集・保存（Save All / Save Selected）・Undo/Redo・ライブプレビューができる。

```cpp
auto* pm = TuboEngine::ParticleManager::GetInstance();
ParticlePreset p; p.name = "Hit"; p.texture = "Resources/Textures/white.png"; p.burstCount = 20;
pm->CreateEmitterByType("Ring", p);           // 名前が重複すると自動で連番が付く
pm->Find("Hit")->Emit(20);                    // 任意のタイミングで発生
pm->Update(dt, camera_.get());                // シーンの Update で
pm->Draw();                                   // シーンの ParticleDraw で
pm->DrawImGui();                              // シーンの ImGuiDraw で
```

### 2-8. テキスト（TextManager）

- テキストとスプライトを **識別名付き** で管理し、レイアウトを JSON（`Resources/Text/*.json`）で読み書きする。
- ImGui の TextManager パネルで位置・色を調整 → Save で JSON に書き戻せる（TitleScene がこの方式）。

### 2-9. 設定（Settings）

- `Resources/Settings.json` に `bgmVolume / seVolume` を保存。起動時に `Order::Initialize` で `Load()`。
- 項目を増やす手順は [Settings.h](../project/application/settings/Settings.h) 冒頭のコメント（3 か所に追記）。JsonManager 導入後は 1 か所で済むようにできる。

### 2-10. Ver7.0 で追加したもの

#### Time（経過時間） — `engine/base/Utilities/GameTime.h`

> ファイル名を `Time.h` にすると Windows の標準ヘッダ `<time.h>` とぶつかってビルドが壊れるので `GameTime.h`。クラス名は `TuboEngine::Time`。

- `Framework::Update` が毎フレーム自動で計測する。ゲーム側は取得するだけ。
- エディタの一時停止中は `DeltaTime()` が 0。1 フレームの最大は 0.1 秒（ブレークポイントで止めた後に一気に進まない）。

```cpp
#include "GameTime.h"
pos_ += velocity_ * TuboEngine::Time::DeltaTime();   // ゲームの処理はこれ
uiTimer_ += TuboEngine::Time::UnscaledDeltaTime();   // 一時停止・スロー中も進めたい UI 演出
TuboEngine::Time::SetTimeScale(0.3f);                // スロー演出（1.0 で元に戻す）
```

- `TitleScene` / `TsubokiScene` / `SampleScene` の `1.0f / 60.0f` を `Time::DeltaTime()` に置き換え済み。
- `Animator::Update()` は引数を省略すると `Time::DeltaTime()` を使う。

#### マウスカーソルの固定 — `WinApp`

| モード | 動作 |
|---|---|
| `Free`（既定） | 固定しない |
| `Confined` | ウィンドウ内に閉じ込める。Debug で Game ウィンドウ表示中は **Game の画像の範囲** に閉じ込める |
| `Locked` | 中央に固定してカーソルを隠す（視点操作向け。移動量は `Input::GetMouseMove()`） |

```cpp
auto* win = TuboEngine::WinApp::GetInstance();
win->SetCursorMode(TuboEngine::WinApp::CursorMode::Free);    // タイトル・メニュー
win->SetCursorMode(TuboEngine::WinApp::CursorMode::Locked);  // FPS 視点のステージ
win->SetCursorCaptured(false); // モードはそのまま一時的に外す（ポーズ画面など）
```

Debug ビルドでの操作（Unity の Play モードと同じ）:
- 起動直後・Game 表示に切り替えた直後・別ウィンドウに切り替えた後は **固定されていない**。Game 画面を **クリックすると固定**。
- **Esc で一時解除**（ゲーム側にも Esc は届く）。
- メニュー **Layout → マウスカーソル** でモードを切り替え可能。Inspector に今の状態が出る。
- **既定は `Free`（固定しない）**。固定したいシーンの `Initialize` で `SetCursorMode` を呼ぶ（シーンを抜ける時に `Free` に戻すこと）。
- Release ビルドでは Esc での一時解除は無し（`Confined` / `Locked` にしたら、ウィンドウが前面にある間は固定）。

#### イージング — `engine/math/Easing.h`

Linear / Sine / Quad / Cubic / Quart / Quint / Expo / Circ / Back / Elastic / Bounce（それぞれ In / Out / InOut）。

```cpp
#include "Easing.h"
using namespace TuboEngine;
float e = Easing::OutCubic(t);                                  // t は 0〜1（範囲外はクランプ）
float e = Easing::Apply(Easing::Type::OutBack, t);              // 種類を変数で選ぶ
Vector3 p = Easing::Ease(start, end, t, Easing::Type::InOutSine); // 値を直接補間

// ImGui で種類を選ぶ
int type = static_cast<int>(type_);
if (ImGui::Combo("Easing", &type, Easing::Names(), static_cast<int>(Easing::Type::Count))) {
	type_ = static_cast<Easing::Type>(type);
}
// JSON 保存は Easing::ToString(type_) / Easing::FromString(str)
```

#### Tween — `engine/math/Tween.h`

経過時間を自分で数えずに「A から B へ N 秒で動かす」。

```cpp
#include "Tween.h"
TuboEngine::Tween<Vector2> slide;
slide.Start({-300, 100}, {100, 100}, 0.4f, Easing::Type::OutBack, /*delay*/ 0.1f);
slide.SetLoopMode(TuboEngine::Tween<Vector2>::LoopMode::PingPong); // 必要なら（None / Restart / PingPong）

// Update
sprite_->SetPosition(slide.Update(TuboEngine::Time::DeltaTime()));
if (slide.IsFinished()) { /* 終わった */ }
```

その他: `Restart()` / `Complete()`（終点へ飛ばす）/ `Pause()` / `Resume()` / `GetProgress()`。

#### JsonManager — `engine/base/Utilities/JsonManager.h`

```cpp
#include "JsonManager.h"
using TuboEngine::JsonManager;

// 読む（失敗したら false。理由は Console に赤で出る）
JsonManager::Json j;
if (JsonManager::Load("Resources/Player.json", j)) {
	JsonManager::Read(j, "speed", speed_);       // キーが無い・型が違う時は speed_ はそのまま
	JsonManager::Read(j, "position", position_); // Vector2/3/4 は [x, y, z] の配列
}

// 書く（フォルダが無ければ作る。一時ファイル→置き換えなので途中で落ちても壊れない）
JsonManager::Json out;
out["speed"] = speed_;
out["position"] = position_;
JsonManager::Save("Resources/Player.json", out);
```

**構造体を丸ごと保存する**（保存と読み込みの項目がずれない書き方）:

```cpp
struct PlayerParam {
	float speed = 5.0f;
	int hp = 100;
	Vector3 spawn{0, 0, 0};
};
// 構造体と同じ名前空間に書く。X("JSONのキー", メンバ名) を並べるだけ
#define PLAYER_PARAM_FIELDS(X) X("speed", speed) X("hp", hp) X("spawn", spawn)
TUBO_JSON_DEFINE(PlayerParam, PLAYER_PARAM_FIELDS)

PlayerParam param;
JsonManager::LoadObject("Resources/Player.json", param); // 無いキーは初期値のまま
JsonManager::SaveObject("Resources/Player.json", param);
```

- `Load(path, j, false)` … ファイルが無い時はログを出さない（初回起動で無いのが普通の設定ファイル用）。
- 空のファイルは「データ無し」として false（エラー表示はしない）。
- enum は数値で保存される。

**移行済み**:
- `ParticleManager` … 対応表は [ParticlePresetJson.h](../project/EngineCore/engine/graphic/Particle/ParticlePresetJson.h)。**項目を増やしたらここに 1 行足すだけ**。ファイル形式は今までと同じ（古いファイルもそのまま読める）。
- `TextManager` … `TextManager.cpp` 先頭の `TUBO_TEXT_DEFINITION_FIELDS` / `TUBO_SPRITE_DEFINITION_FIELDS`。キー名は今までと同じ。
- `Settings` … `Settings.h` 末尾の `SETTINGS_FIELDS`。項目追加は「フィールド・一覧・OptionScene」の 3 か所。

#### アニメーション（Ver7.0 で追加）

**イベント**（決まった時間に処理を呼ぶ）:

```cpp
human_->AddEvent("Walk", 0.30f, [this] { PlaySe("step.mp3"); });          // 0.3秒を通過するたびに呼ばれる（ループ毎周）
human_->AddEvent("Attack", 0.15f, nullptr, "hit");                         // 名前だけ付けて…
human_->SetEventListener([this](const std::string& anim, const std::string& ev) {
	if (ev == "hit") { CheckAttackHit(); }                                  // …ここでまとめて受け取る
});
human_->SetOnFinished([this](const std::string& anim) { /* ループしない再生が終わった */ });
```

- 逆再生でも通過すれば呼ばれる。0 秒のイベントは再生開始時と毎周の先頭で呼ばれる。
- クロスフェード中の「前のアニメーション」のイベントは呼ばれない。

**AnimatorController**（Unity の Animator Controller のような状態遷移）— `engine/graphic/Animation/AnimatorController.h`:

```cpp
using Op = TuboEngine::AnimatorController::Op;
controller_.AddState("Idle", "Idle");                 // 状態名, Animator に登録したアニメーション名
controller_.AddState("Walk", "Walk");
controller_.AddState("Attack", "Attack", /*loop*/ false);
controller_.AddTransition("Idle", "Walk", {{"speed", Op::Greater, 0.1f}});
controller_.AddTransition("Walk", "Idle", {{"speed", Op::LessEqual, 0.1f}});
controller_.AddTransition("*", "Attack", {{"attack", Op::Trigger}});      // "*" = どの状態からでも
controller_.AddTransition("Attack", "Idle", {}, -1.0f, /*exitTime*/ 1.0f); // 最後まで再生したら戻る
controller_.AddStateEvent("Walk", 0.3f, "footstep");                       // SetEventListener に届く

// Update（animator->Update より前）
controller_.SetFloat("speed", speed);
if (attackPressed) { controller_.SetTrigger("attack"); }
controller_.Update(*human_);
human_->Update();
// ImGui
controller_.DrawImGui("Human"); // 今の状態・パラメータ（その場で変更可）・遷移一覧。状態ボタンで強制切り替え
```

- 比較: `Greater / GreaterEqual / Less / LessEqual / Equal / NotEqual / True / False / Trigger`。
- 同じ状態からの遷移は追加順に調べ、最初に成立したものを使う。`"*"` の遷移が先。
- JSON でも定義できる: `controller_.LoadFromJson("Resources/Animations/human.json")`（形式は `AnimatorController.cpp` 冒頭）。`SaveToJson` で今の定義を書き出せる。
- 使用例: `TsubokiScene`（Idle / Walk / SneakWalk を `moving` と `sneak` で切り替え）。

**ボーンの位置**（武器を持たせる・エフェクトを出す）:

```cpp
if (human_->HasJoint("mixamorig:RightHand")) {
	sword_->SetLocalMatrix(human_->GetJointWorldMatrix("mixamorig:RightHand")); // ※ sword_ 自身の SRT は単位にしておく
	Vector3 hand = human_->GetJointWorldPosition("mixamorig:RightHand");     // 座標だけ欲しい時
}
```

- `Update()` の後に呼ぶ。TsubokiScene の ImGui「右手の位置を表示」で確認できる。

**その他**: 同じ glTF の読み込みは2回目からキャッシュ（`ClearAnimationCache()` で消せる）。存在しないファイルは落ちずに Console にログ。

---

## 3. バグになりそうな箇所

重要度: **高** = 実際に値が壊れる・落ちる可能性が高い / **中** = 条件次第で起きる / **低** = 今は問題ないが将来ハマる

| # | 重要度 | 場所 | 内容 | 起きること・対策 |
|---|---|---|---|---|
| 1 | **高** ✅修正済 | [ParticleManager.cpp](../project/EngineCore/engine/graphic/Particle/ParticleManager.cpp) の `BuildSnapshotJson` / `SaveSelected` / `LoadAll` / `LoadMerge` / `ApplySnapshot` | 同じ JSON 変換が 4〜5 か所に手書きされていて**項目がそろっていない**。<br>・全部: `scaleMin/Max`・`colMin/Max`・`rotSpeedRangeZ`・`initialRotRangeZ`・`blendModeOverride` を保存しない<br>・`SaveSelected`: `center` を保存しない<br>・`LoadMerge`: `colorStart/End`・`drag`・`center`・`simulateInWorldSpace` を読まない | ImGui で調整して保存 → 再起動や Undo で**一部の値が既定値に戻る**。→ 1-3 の 4（JSON 定義を 1 か所に）で直す |
| 2 | **高** ✅修正済 | [Input.cpp](../project/EngineCore/engine/input/Input.cpp) `Update` / `GetJoystickState(…, XINPUT_STATE&)` | ・XInput の更新で `xinputIndex_` ではなく**配列番号 `i`** を渡している<br>・XInput パッドは DirectInput でも列挙されるので**同じパッドが 2 回登録**される<br>・`GetJoystickState(0, XINPUT_STATE&)` は種類を確認せず共用体を返す | パッドを挿すと 0 番が DirectInput 側になり、**XInput として読むと値がでたらめ**（キャラが勝手に動く等）。→ 種類チェックを入れる・XInput 対応パッドは DirectInput 側から除外する |
| 3 | **高** ✅修正済 | [SceneManager.cpp](../project/EngineCore/engine/scene/SceneManager.cpp) `Update` 先頭 | 未登録の番号に遷移すると `currentScene` が null になり、`Update` が先頭で return するので**以後 `ChangeScene` が処理されない**（Release は assert が消えるので気付きにくい） | 黒画面のまま戻れない。→ null チェックを遷移処理の後ろに移す／生成失敗時は元のシーン番号に戻す |
| 4 | **高** ✅修正済 | [Framework.cpp](../project/EngineCore/engine/Framework/Framework.cpp) `Finalize` | `SceneManager::Finalize`（＝シーンの `Finalize`）が、`Input`・`ParticleManager`・`TextureManager`・`ModelManager`・`SrvManager` の終了**より後**に呼ばれている | シーンの Finalize でこれらに触ると、解放済みを使う or `GetInstance()` で**新しく作り直されてリーク**。→ `SceneManager::Finalize` を一番最初に移す |
| 5 | 中 ✅修正済 | [Framework.cpp:79](../project/EngineCore/engine/Framework/Framework.cpp) / [WinApp.cpp:96](../project/EngineCore/engine/base/Common/WinApp.cpp) | マウス固定が常時 ON。Debug エディタ表示中もウィンドウ全体に閉じ込める | 2 枚目のモニタに移れない、ブレークポイントで止めたとき操作しにくい。→ 1-3 の 1 |
| 6 | 中 🟡一部修正 | 各所の `1.0f / 60.0f` | deltaTime が固定値 | 処理落ちするとゲーム全体がスローになる。高リフレッシュレート環境で速度が変わる可能性。→ 1-3 の 0 |
| 7 | 中 ✅修正済 | [WinApp.cpp](../project/EngineCore/engine/base/Common/WinApp.cpp) `ProcessMessage` | 1 フレームにメッセージを **1 個しか処理しない**（`if (PeekMessage…)`） | メッセージが溜まるとウィンドウのドラッグ・リサイズや入力が重くなる。→ `while` にする（`WM_QUIT` 判定は中で） |
| 8 | 中 ✅修正済 | [Animation.cpp](../project/EngineCore/engine/graphic/Animation/Animation.cpp) `LoadAnimation` / `LoadAnimations` | 読み込み失敗を `assert(scene)` だけで確認 | Release でパスを間違えると **null 参照で落ちる**。→ Logger に出して空のアニメを返す |
| 9 | 中 ✅修正済 | `Animator::Initialize` ＋ `AddAnimation(名前, ファイル)` | 同じ glTF を Assimp で何度もパース | モデルが増えるとシーン読み込みが遅くなる。→ キャッシュ（1-3 の 5） |
| 10 | 中 🟡一部修正 | `"Resources/..."` の相対パス全般（Settings / Particle / Text） | 作業ディレクトリ基準 | exe を別の場所から起動すると、読み込みも保存も**黙って失敗**（設定が保存されない）。→ JsonManager で exe の場所基準に |
| 11 | 低 ✅修正済 | [Framework.h](../project/EngineCore/engine/Framework/Framework.h) | `ImGuiManager.h` の include が `#ifdef _DEBUG` なのに、使う側は `#ifdef USE_IMGUI` | Release で USE_IMGUI を有効にする（またはその逆の）構成を作るとコンパイルエラー。→ 条件をそろえる |
| 12 | 低 | `ImGuiManager::Begin` のショートカット | Ctrl+Shift+P（一時停止切替）と Ctrl+Alt+P（コマ送り）は Ctrl+Shift+Alt+P で両方反応 | 一時停止が解除されつつコマ送りになる等。→ 修飾キーを厳密に判定 |
| 13 | 低 | `Animator::DrawImGui` | `ImGui::Begin` を直接使用（`BeginPanel` ではない） | Window メニューから非表示にできない |
| 14 | 低 | `Input::SetJoystickDeadZone` | 値を保存するだけで、どこにも使われていない | デッドゾーンを設定しても効かない（今は各シーンで自前判定） |
| 15 | 低 | `WinApp::Finalize` | `CloseWindow`（最小化するだけ）を呼んでいて `DestroyWindow` していない。`timeBeginPeriod(1)` に対する `timeEndPeriod(1)` も無い | 終了時なので実害は小さい |
| 16 | 低 | `ImGuiManager::Finalize` | `delete instance` が `#ifdef USE_IMGUI` の中 | ImGui 無しビルドでわずかにリーク |
| 17 | 低 | `ParticleManager::GetInstance` | 関数内 static。最初の `GetInstance()` 時に JSON を読み、プログラム終了時にもデストラクタが走る | 初期化順序や終了順序が見えにくい。他のシングルトンと同じ明示的な Initialize/Finalize に寄せると安全 |
| 18 | **高** ✅修正済 | `TextManager::LoadTextLayout` / TitleScene・OptionScene | レイアウトを読み込むとテキストが全部作り直されるのに、シーンは `GetTextByName` で取った古いポインタを使い続けていた | ImGui の TextManager パネルで Load すると**アクセス違反で落ちる**。→ `GetLayoutRevision()` が変わったらシーンが取り直す。Load ボタンは次の `UpdateAll` で読み込む |

### Ver7.0 での修正内容

- **#2 パッド**: XInput は XInput のコントローラー番号で取得。XInput 対応パッドは DirectInput 側に登録しない（デバイスパスの `IG_` で判定）。XInput パッドを先に並べるので 0 番が XInput パッドになる。`GetJoystickState` は種類が違うと false を返す。
- **#3 SceneManager**: 未登録の番号への遷移はログ（Debug は assert）を出して今のシーンのまま続ける。開始シーンが無い状態からでも `ChangeScene` できる。
- **#4 終了順**: `SceneManager::Finalize` を `Framework::Finalize` の最初に移動。
- **#5 マウス固定**: `CursorMode` で切り替え可能に（[2-10](#2-10-ver70-で追加したもの)）。
- **#6 deltaTime**: `Time` を追加し、TitleScene / TsubokiScene / SampleScene / Animator の既定値を置き換え。エンジン内部の `Particle.cpp` / `ParticleEmitter.cpp` / `VHSEffect.cpp` は未対応。
- **#7 メッセージ処理**: `while (PeekMessage…)` で溜まった分を全て処理。
- **#11 include**: `ImGuiManager.h` を常に include（中で `USE_IMGUI` を見ている）。

- **#1 Particle**: 保存・読み込み・Undo の全てが [ParticlePresetJson.h](../project/EngineCore/engine/graphic/Particle/ParticlePresetJson.h) の対応表 1 つを使うようにした。抜けていた `scaleMin/Max`・`colMin/Max`・`rotSpeedRangeZ`・`initialRotRangeZ`・`blendModeOverride` も保存される。
- **#10 相対パス**: `JsonManager` は作業フォルダに無ければ exe の隣も探す。失敗時は Console に理由と作業フォルダを出す（黙って失敗しない）。JSON 以外（テクスチャ・モデル等）は未対応。

- **#18 レイアウト再読み込みで落ちる**: TextManager に `GetLayoutRevision()`（作り直した回数）と `RequestLoadTextLayout()`（次の UpdateAll で読み込む）を追加。テキストのポインタを持つシーンは、値が変わったら取り直す（TitleScene の `RefreshTextRefs` が例）。

- **#8 アニメーション読み込み失敗**: `assert` をやめ、Console にログを出して空のアニメーション（duration 0）を返す。
- **#9 同じ glTF を何度もパース**: 読み込み結果をファイルごとにキャッシュ（初回 32ms → 2回目 0.15ms。human/sneakWalk.gltf で計測）。

### 先に直すべきもの（1 週目の B）

1. **#3 SceneManager** と **#4 Finalize の順番** … 数行の修正で済み、落ちる原因を消せる
2. **#2 パッド** … パッドを使う人がいるならすぐ。種類チェックだけでも先に入れる
3. **#1 Particle** … JsonManager（2 週目）で根本対応。それまでは「保存すると一部の値が戻る」ことをチームに周知
