# アルゴリズムとデータ構造B Group1

## 実行方法

### 1. コンパイル
以下のコマンドでソースコードをコンパイルします。

```bash
gcc -O2 -o prep_1 src/prep_1.c -lm
gcc -O2 -o search_1 src/search_1.c -lm
```

### 2. インデックス作成 (prep_1)
データベースファイルから検索用インデックスを作成します。
`test/db_2` はテスト用のデータベースです。

```bash
./prep_1 test/db_2 > index_2
```
※ `index_2` というファイルにインデックスが出力されます。

### 3. 検索実行 (search_1)
作成したインデックスを使って、クエリファイルに対する検索を行います。
`test/query_2` はテスト用のクエリです。

```bash
./search_1 test/query_2 index_2 > result_2
```
※ `result_2` に検索結果（0または1の列）が出力されます。