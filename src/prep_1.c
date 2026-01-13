// prep_1.c(最終版)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define LINE_LEN 16
#define MAX_BUCKETS 10000 // 各パーツ(4文字分)が取り得る値の最大数 (10^4)

// 合計15文字を {4, 4, 4, 3} の長さに分ける
const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

// 検索時のさらなる高速化のため、インデックスには生の文字列だけでなく文字の出現頻度を計算して同梱する
// これにより、Myersアルゴリズムを実行する前に、文字の過不足だけで編集距離3を超えるものをフィルタリングする
typedef struct {
    uint64_t hist; // 各文字(A-J)のカウントを4bitずつ格納したパックデータ
    char str[16];  // 元の文字列データ
} DataEntry;

// ハミング距離や編集距離の枝刈りに使うヒストグラムを生成
// Aなら0-3bit、Bなら4-7bit...というように、10種類の文字の出現数を64bit整数の中にパッキングします
uint64_t make_hist(const char* str) {
    uint64_t h = 0;
    for(int i=0; i<15; i++) {
        int shift = (str[i] - 'A') * 4;
        h += (1ULL << shift);
    }
    return h;
}

// 文字列の特定のパーツ（A-Jの並び）を 0〜9999 の数値に変換
uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) {
        val = val * 10 + (str[offset + i] - 'A');
    }
    return val;
}

// ファイル全体をメモリに一括読み込み
char* read_all(const char* fname, size_t* size) {
    FILE* fp = fopen(fname, "rb");
    if(!fp) exit(1);
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = malloc(*size);
    if (fread(data, 1, *size, fp) != *size) exit(1);
    fclose(fp);
    return data;
}

int main(int argc, char* argv[]) {
    if (argc != 2) return 1;

    size_t db_size;
    char* db_data = read_all(argv[1], &db_size);
    int num_lines = db_size / LINE_LEN;

    // 索引の先頭にデータ件数を記録
    fwrite(&num_lines, sizeof(int), 1, stdout);

    static int counts[MAX_BUCKETS];
    static int offsets[MAX_BUCKETS];
    
    DataEntry* sorted_data = malloc(sizeof(DataEntry) * num_lines);

    // 4つのパーツそれぞれについてインデックスを作成するループ。
    // 各ループでは「計数ソート（Counting Sort）」の要領でデータを並べ替えます。
    for (int p = 0; p < 4; p++) {
        // 各バケット（特定の文字列パーツ）に何件のデータがあるか数える
        memset(counts, 0, sizeof(counts));
        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            uint16_t key = encode_part(str, PART_OFFSET[p], PART_LEN[p]);
            counts[key]++;
        }

        // 各バケットの開始位置を計算
        int current_idx = 0;
        for (int k = 0; k < MAX_BUCKETS; k++) {
            offsets[k] = current_idx;
            current_idx += counts[k];
        }

        // オフセット情報を書き出し（検索時はこれを見て特定のバケットにジャンプする）
        fwrite(offsets, sizeof(int), MAX_BUCKETS, stdout);
        int total_count = num_lines; 
        fwrite(&total_count, sizeof(int), 1, stdout); 

        // データを実際にバケット順に並べ替えながら、追加情報を付与
        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            uint16_t key = encode_part(str, PART_OFFSET[p], PART_LEN[p]);
            int pos = offsets[key]++; // 書き込み位置を確定させてインクリメント
            
            // 検索時に役立つヒストグラムをあらかじめ計算して格納
            sorted_data[pos].hist = make_hist(str);
            memcpy(sorted_data[pos].str, str, LINE_LEN);
        }
        
        // ソート済みデータエントリを書き出し
        fwrite(sorted_data, sizeof(DataEntry), num_lines, stdout);
    }

    free(sorted_data);
    free(db_data);
    return 0;
}