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

## ビルドと実行方法
### 1. ビルド
ルートディレクトリで make コマンドを実行すると、索引構築用(prep_group1)と検索用(search_group1)の実行ファイルが生成されます。

```bash
make
```
※ make がない場合は以下のコマンドでコンパイルしてください 。

```bash
gcc -O3 -o prep_group1 src/prep_group1.c
gcc -O3 -o search_group1 src/search_group1.c
```
### 2. 索引の構築
データベースファイル (database.txt) を読み込み、検索用のバイナリ索引 (index.bin) を生成します。

```bash
# database がカレントディレクトリにある場合
./prep_group1
```
- 入力: database.txt (100万行の文字列) 
- 出力: index.bin (独自フォーマットの索引ファイル)

### 3. 検索の実行
生成された索引 (index.bin) を利用して、クエリ (query.txt) に対する検索を行います。

```bash
# query がカレントディレクトリにある場合
./search_group1
```

## 制約事項
- メモリ制限: 使用メモリの上限は 5GB です。
- 構築時間: 索引構築 (prep) に使える時間は最大 4分 です。
- 検索時間: 検索処理 (search) に使える時間は最大 1分 です。

## 参考文献
- [Gitでのチーム開発の流れ](https://zenn.dev/taku_sid/articles/20250409_github_team)