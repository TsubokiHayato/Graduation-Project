# Graduation-Project

[TuboEngine-Core](https://github.com/TsubokiHayato/TuboEngine-Core) をサブモジュールとして利用する、DirectX12 ゲーム制作の**スターターテンプレート**です。
エンジン本体には手を入れず、`project/application/` にゲーム側のコードを足していく構成になっています。

## 構成

```
Graduation-Project/
├── project/
│   ├── EngineCore/            … TuboEngine-Core（git submodule）
│   ├── GraduationProject.sln  … Visual Studio ソリューション
│   ├── GraduationProject.vcxproj … アプリ側プロジェクト
│   ├── main.cpp               … エントリポイント（Order を Run するだけ）
│   ├── FadeScreen.{h,cpp}     … シーン遷移用フェード（共通UI）
│   ├── application/
│   │   ├── Order.{h,cpp}      … Framework 継承の起動クラス（初期化/更新/描画/終了）
│   │   ├── scene/
│   │   │   ├── GameScenes.h            … シーン番号 enum
│   │   │   ├── SceneRegistration.{h,cpp} … 番号→生成関数の登録
│   │   │   ├── Sample/       … エンジンの使い方を一通り学べる教材シーン
│   │   │   ├── Title/        … メニュー付きタイトル画面
│   │   │   └── Option/       … データ駆動の設定画面
│   │   ├── settings/         … 設定の保存/読み込み（Resources/Settings.json）
│   │   └── audio/            … BGM/SE の再生マネージャ
│   └── Resources/            … モデル・テクスチャ・フォント・音・テキストレイアウト
└── .gitmodules
```

## セットアップ

```sh
# サブモジュールごとクローン
git clone --recursive https://github.com/TsubokiHayato/Graduation-Project.git

# すでにクローン済みなら
git submodule update --init --recursive
```

## ビルド

- Visual Studio 2026（v145 ツールセット）以降、Windows SDK 10、C++20。
- `project/GraduationProject.sln` を開き、構成 `Debug|x64` または `Release|x64` でビルド。
- ビルド時に PreBuild イベントでエンジンのシェーダが `project/Resources/Shaders/` にコピーされ、
  PostBuild で `dxcompiler.dll` / `dxil.dll` が出力先へコピーされます。

## シーンの増やし方

1. `project/application/scene/` 配下に `IScene` を継承したシーンクラスを追加する。
2. `GameScenes.h` の enum に番号を追加する。
3. `SceneRegistration.cpp` の `RegisterGameScenes()` に生成関数を登録する。
4. 起動シーンを変えたいときは `Order.cpp` の `SceneManager::Initialize(...)` を変更する（既定は `TITLE`）。

## 収録シーン

| シーン | 内容 |
| --- | --- |
| `SAMPLE` | 3D/2D/パーティクル/音/デバッグカメラ/ImGui を一通り触れる教材シーン |
| `TITLE`  | メニュー（スタート／設定／終了）付きタイトル。スタートで `SAMPLE` へ遷移 |
| `OPTION` | 音量などの設定を編集して `Settings.json` に保存する設定画面 |

エンジン本体の詳細は [TuboEngine-Core](https://github.com/TsubokiHayato/TuboEngine-Core) を参照してください。
