# アルゴリズムとデータ構造B グループワーク 1班

## 概要

## 実装機能

## 動作環境

## メンバーと役割

| 氏名 | 役割 | GitHubアカウント |
| :--- | :--- | :-------------------- |
| 氏名1 | 代表者, 担当名 | [@your_github_username] |
| 氏名2 | 担当名 | [@your_github_username] |
| 氏名3 | 担当名 | [@your_github_username] |
| 氏名4 | 担当名 | [@your_github_username] |
| 氏名5 | 担当名 | [@your_github_username] |

## ディレクトリ構成

```
.
├── src/          # ソースファイル (.c)
├── include/      # ヘッダーファイル (.h)
├── bin/          # 実行ファイル (Git管理外)
├── doc/          # ドキュメント (レポート、設計書など)
├── test/         # テスト
├── .gitignore    # Git無視リスト
└── README.md     # このファイル
```

## ビルド (コンパイル) 方法

プロジェクトのルートディレクトリ (`ADSB_GW`) で以下のコマンドを実行します。

```sh
# すべての.cファイルをコンパイルし、'main'という名前で'bin'フォルダに実行ファイルを作成する
# -Wall: 全ての警告を表示
# -Wextra: 追加の警告を表示
# -std=c11: C11規格に準拠
# -O2: 最適化レベル2
gcc -Wall -Wextra -std=c11 -O2 -o ./bin/main -I./include ./src/*.c
```

## 実行方法

コンパイルが成功した後、プロジェクトのルートディレクトリで以下のコマンドを実行します。

```sh
# binフォルダ内の実行ファイルを起動
./bin/main
```

## 参考文献

- [Gitでのチーム開発の流れ](https://zenn.dev/taku_sid/articles/20250409_github_team)