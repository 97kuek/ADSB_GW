// prep_group1.c

// Ver.5コメントアウト付き(hoshina)
// 変更点: データを索引に埋め込むことでシーケンシャルアクセスを実現

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define LINE_LEN 16       // 文字列長(15) + 改行(1)
#define MAX_BUCKETS 10000 // 鳩の巣原理の分割数に応じたバケット数

// 鳩の巣原理：15文字を4分割する場合のオフセットと長さ
const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

// 索引に埋め込むためのデータ構造
typedef struct {
    char str[16];
} DataEntry;

// 文字列の一部を整数(バケットID)に変換する関数
uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) {
        val = val * 10 + (str[offset + i] - 'A');
    }
    return val;
}

// ファイルを一括でメモリに読み込む関数
char* read_all(const char* fname, size_t* size) {
    FILE* fp = fopen(fname, "rb");
    if(!fp) exit(1);
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = malloc(*size);
    if(!data) exit(1);
    if (fread(data, 1, *size, fp) != *size) exit(1);
    fclose(fp);
    return data;
}

int main(int argc, char* argv[]) {
    if (argc != 2) return 1;

    size_t db_size;
    char* db_data = read_all(argv[1], &db_size);
    int num_lines = db_size / LINE_LEN;

    // データ総数を出力
    // (search側でメモリマッピングの位置計算に使用)
    fwrite(&num_lines, sizeof(int), 1, stdout);

    static int counts[MAX_BUCKETS];
    static int offsets[MAX_BUCKETS];
    
    // ソート済みデータを格納する配列
    DataEntry* sorted_data = malloc(sizeof(DataEntry) * num_lines);
    if(!sorted_data) return 1;

    // 4つの分割パートごとにバケットソートを行う
    for (int p = 0; p < 4; p++) {
        // 1. 各バケットに入る個数をカウント
        memset(counts, 0, sizeof(counts));
        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            uint16_t key = encode_part(str, PART_OFFSET[p], PART_LEN[p]);
            counts[key]++;
        }

        // 2. 累積和をとってオフセット位置（開始位置）を計算
        int current_idx = 0;
        for (int k = 0; k < MAX_BUCKETS; k++) {
            offsets[k] = current_idx;
            current_idx += counts[k];
        }

        // オフセットテーブルを出力
        fwrite(offsets, sizeof(int), MAX_BUCKETS, stdout);
        
        // 総数も出力（フォーマット整合性のため）
        int total_count = num_lines; 
        fwrite(&total_count, sizeof(int), 1, stdout); 

        // 3. データをバケット順に並べ替え
        // --- 【高速化の重要ポイント：データ埋め込み】 ---
        // 以前はIDのみを保存していましたが、文字列データそのものをここに並べます。
        // これにより、search実行時にメモリ上を連続して読み進める「シーケンシャルアクセス」が可能になり、
        // キャッシュヒット率が劇的に向上します。
        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            uint16_t key = encode_part(str, PART_OFFSET[p], PART_LEN[p]);
            int pos = offsets[key]++; 
            memcpy(sorted_data[pos].str, str, LINE_LEN);
        }
        
        // ソート済みデータを出力
        fwrite(sorted_data, sizeof(DataEntry), num_lines, stdout);
    }
    
    free(sorted_data);
    free(db_data);
    return 0;
}