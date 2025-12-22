// search_group1.c

// Ver.5コメントアウト付き(hoshina)
// 変更点: Branchlessアルゴリズム、ループ展開によるパイプライン最適化

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define STR_LEN 15
#define LINE_LEN 16
#define THRESHOLD 3
#define MAX_BUCKETS 10000

// Myersアルゴリズム用のパターンデータ（ビットマスク）
typedef struct { uint64_t P_eq[10]; } PatternData;

// --- 【高速化の核心：Branchless Myers Algorithm】 ---
// 通常のアルゴリズムでは if(HP & bit) score++ のような条件分岐が入ります。
// しかし、現代のCPUにおいて条件分岐の予測ミスは大きな遅延（パイプラインストール）を招きます。
// そこで、全てのif文を「ビット演算と足し算」に置き換えました。
// これにより、CPUが迷うことなく一定の速度で計算し続けられるようになります。
static inline int myers_distance(const char* text, PatternData* pdata) {
    uint64_t VP = ~0ULL;
    uint64_t VN = 0ULL;
    int score = STR_LEN;
    uint64_t PM, D0, HP, HN;
    int char_idx;
    
    const int LAST_BIT = STR_LEN - 1;

    // ループ展開マクロ (ifを使わず計算のみで処理)
    #define STEP(i) \
        char_idx = text[i] - 'A'; \
        /* 文字がA-Jの範囲内かどうかも条件分岐なし(三項演算子/CMOV命令)で処理 */ \
        PM = (char_idx >= 0 && char_idx < 10) ? pdata->P_eq[char_idx] : 0; \
        D0 = (((PM & VP) + VP) ^ VP) | PM | VN; \
        HP = VN | ~(D0 | VP); \
        HN = VP & D0; \
        /* 【Branchless化】 */ \
        /* if文の代わり: ビットシフトして0か1を取り出し、そのまま加減算する */ \
        score += (HP >> LAST_BIT) & 1; \
        score -= (HN >> LAST_BIT) & 1; \
        HP = (HP << 1) | 1ULL; \
        HN = (HN << 1); \
        VP = HN | ~(D0 | HP); \
        VN = HP & D0;

    // 15文字分をマクロ展開し、forループ自体のオーバーヘッドをゼロにする
    STEP(0); STEP(1); STEP(2); STEP(3); STEP(4);
    STEP(5); STEP(6); STEP(7); STEP(8); STEP(9);
    STEP(10); STEP(11); STEP(12); STEP(13); STEP(14);

    return score;
}

// クエリ文字列のビットマスクを事前計算
void precompute_pattern(const char* pattern, PatternData* pdata) {
    for (int i = 0; i < 10; i++) pdata->P_eq[i] = 0;
    for (int i = 0; i < STR_LEN; i++) {
        int char_idx = pattern[i] - 'A';
        if (char_idx >= 0 && char_idx < 10) pdata->P_eq[char_idx] |= (1ULL << i);
    }
}

const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

// 文字列の一部をバケットIDに変換
uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) val = val * 10 + (str[offset + i] - 'A');
    return val;
}

// mmap: ファイルをメモリに直接マッピングする（freadより高速）
char* map_file(const char* filename, size_t* size) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) { perror("open"); exit(1); }
    struct stat sb;
    if (fstat(fd, &sb) == -1) { perror("fstat"); exit(1); }
    *size = sb.st_size;
    char* addr = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return addr;
}

typedef struct {
    char str[16];
} DataEntry;

// 出力バッファリング：1文字ずつfwriteすると遅いため、まとめて書き込む
#define OUT_BUF_SIZE 65536
char out_buffer[OUT_BUF_SIZE];
int out_buf_pos = 0;

static inline void fast_put(char c) {
    out_buffer[out_buf_pos++] = c;
    if (out_buf_pos >= OUT_BUF_SIZE) {
        fwrite(out_buffer, 1, OUT_BUF_SIZE, stdout);
        out_buf_pos = 0;
    }
}

static inline void flush_out() {
    if (out_buf_pos > 0) {
        fwrite(out_buffer, 1, out_buf_pos, stdout);
        out_buf_pos = 0;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) return 1;

    size_t q_sz, idx_sz;
    char* q_data = map_file(argv[1], &q_sz);
    char* idx_ptr = map_file(argv[2], &idx_sz);

    // 索引ファイルの先頭にあるデータ数を取得
    int num_lines = *(int*)idx_ptr;
    char* current_ptr = idx_ptr + sizeof(int);
    
    int* offsets[4];
    DataEntry* data_lists[4];
    
    // 4つのパートそれぞれのオフセットテーブルとデータ本体へのポインタを設定
    for(int p=0; p<4; p++) {
        offsets[p] = (int*)current_ptr;
        current_ptr += sizeof(int) * (MAX_BUCKETS + 1);
        data_lists[p] = (DataEntry*)current_ptr;
        current_ptr += sizeof(DataEntry) * num_lines;
    }

    int q_count = q_sz / LINE_LEN;

    for (int i = 0; i < q_count; i++) {
        const char* q_str = q_data + i * LINE_LEN;
        PatternData pdata;
        precompute_pattern(q_str, &pdata);

        int found = 0;

        for (int p = 0; p < 4; p++) {
            uint16_t key = encode_part(q_str, PART_OFFSET[p], PART_LEN[p]);
            int start = offsets[p][key];
            int end   = offsets[p][key + 1];

            // --- 【高速化ポイント：ループ展開 (Loop Unrolling)】 ---
            // データを1つずつではなく、4つずつまとめてチェックします。
            // ループの終了判定やインクリメントの回数が1/4になり、処理効率が向上します。
            // また、Branchless化した関数を連続で呼ぶことでパイプラインが埋まります。
            int k = start;
            for (; k <= end - 4; k += 4) {
                if (myers_distance(data_lists[p][k].str, &pdata) <= THRESHOLD) { found = 1; goto NEXT_QUERY; }
                if (myers_distance(data_lists[p][k+1].str, &pdata) <= THRESHOLD) { found = 1; goto NEXT_QUERY; }
                if (myers_distance(data_lists[p][k+2].str, &pdata) <= THRESHOLD) { found = 1; goto NEXT_QUERY; }
                if (myers_distance(data_lists[p][k+3].str, &pdata) <= THRESHOLD) { found = 1; goto NEXT_QUERY; }
            }
            
            // 残りの端数（1〜3個）を処理
            // ここで端数チェックを行うため、prep側でのパディングが無くても安全に動作します。
            for (; k < end; k++) {
                if (myers_distance(data_lists[p][k].str, &pdata) <= THRESHOLD) {
                    found = 1;
                    goto NEXT_QUERY;
                }
            }
        }
        NEXT_QUERY:
        fast_put(found ? '1' : '0');
    }
    
    fast_put('\n'); 
    flush_out();
    return 0;
}