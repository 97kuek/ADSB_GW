# アルゴリズムとデータ構造B グループワーク (1班)

編集距離に基づく近似文字列検索を高速に行うシステムの実装プロジェクトです。

## 概要
100万件の文字列データベースから、クエリに対して「編集距離3以内」の近似文字列が存在するかを高速に判定します。

## 実装機能
- 実装途中

## 動作環境

本プログラムは、以下の環境での動作を前提としています。

* **OS**: Ubuntu 24.04.3 LTS
* **Compiler**: gcc v13.3.0
* **Build Tool**: make (または直接gccを利用)
* **Dependencies**: `build-essential` (標準ライブラリのみ使用)

## ディレクトリ構成

```bash
.
├── Makefile            # ビルド用コマンドをまとめたファイル
├── README.md           # README
├── .gitignore          # Gitに上げないファイルの設定
├── src/                # ソースコード置き場
│   ├── prep_group1.c   # 索引構築用ソース
│   └── search_group1.c # 検索用ソース
├── test/               # データファイル置き場 (Git管理外にする)
│   ├── database.txt    # 本番用データベース
│   └── query.txt       # クエリデータ
└── doc/                # ドキュメント置き場
    └── ○○.pdf          # 配布資料など
```

## Gitの使用方法
### 開発を始めるとき(最初だけ)
```Bash
git clone [リポジトリURL]
cd [フォルダ名]
git checkout -b [自分の名前]  # 自分用ブランチを作る
```
### 毎回の作業フロー
```Bash
# 変更を保存して自分のブランチに送る
git add .
git commit -m "変更内容メッセージ"
git push origin [自分の名前]
```
### 他の人のコードを試す
```Bash
# 例: tanakaさんのコードを動かす
git checkout tanaka
git pull
make
./prep_group1 test/db_1
```
### 自分の作業に戻る時
```Bash
git checkout [自分の名前]
```

## 参考文献
- [Gitでのチーム開発の流れ](https://zenn.dev/taku_sid/articles/20250409_github_team)