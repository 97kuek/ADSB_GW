// 中間計測(search_1.c)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define STR_LEN 15
#define LINE_LEN 16
#define THRESHOLD 3
#define MAX_BUCKETS 10000

typedef struct { uint64_t P_eq[10]; } PatternData;

// Myersのビット並列アルゴリズム
// 編集距離のCPテーブルの差分をビット列で管理し、1クロックで複数のセルを計算
// 15文字固定であることを利用し、ループ展開によって分岐を排除
static inline int myers_distance(const char* text, PatternData* pdata) {
    uint64_t VP = ~0ULL, VN = 0ULL;
    int score = STR_LEN;
    uint64_t PM, D0, HP, HN;
    int char_idx;

    #define STEP(i) \
        char_idx = text[i] - 'A'; \
        PM = (char_idx >= 0 && char_idx < 10) ? pdata->P_eq[char_idx] : 0; \
        D0 = (((PM & VP) + VP) ^ VP) | PM | VN; \
        HP = VN | ~(D0 | VP); \
        HN = VP & D0; \
        score += (HP >> 14) & 1; \
        score -= (HN >> 14) & 1; \
        HP = (HP << 1) | 1ULL; HN = (HN << 1); \
        VP = HN | ~(D0 | HP); VN = HP & D0;

    STEP(0); STEP(1); STEP(2); STEP(3); STEP(4);
    STEP(5); STEP(6); STEP(7); STEP(8); STEP(9);
    STEP(10); STEP(11); STEP(12); STEP(13); STEP(14);
    return score;
}

// クエリをP_eqに変換する
// Myersアルゴリズムの事前準備
void precompute_pattern(const char* pattern, PatternData* pdata) {
    for (int i = 0; i < 10; i++) pdata->P_eq[i] = 0;
    for (int i = 0; i < STR_LEN; i++) {
        int char_idx = pattern[i] - 'A';
        if (char_idx >= 0 && char_idx < 10) pdata->P_eq[char_idx] |= (1ULL << i);
    }
}

const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

// 分割された文字列を数値キーに変換し、インデックスのバケットを特定する
uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) val = val * 10 + (str[offset + i] - 'A');
    return val;
}

// クエリ文字列の文字出現頻度を計算
uint64_t make_hist(const char* str) {
    uint64_t h = 0;
    for(int i=0; i<15; i++) h += (1ULL << ((str[i] - 'A') * 4));
    return h;
}

//ヒストグラム距離フィルタ
//編集距離がK以下なら、文字カウントの差の合計は2K以下になる性質を利用
//K=3 のため、差が6を超えれば編集距離3以内になることはない
static inline int hist_diff(uint64_t h1, uint64_t h2) {
    int diff = 0;
    #define H_STEP(k) \
        diff += abs((int)((h1 >> (k*4)) & 0xF) - (int)((h2 >> (k*4)) & 0xF));
    
    H_STEP(0); H_STEP(1); H_STEP(2); H_STEP(3); H_STEP(4);
    H_STEP(5); H_STEP(6); H_STEP(7); H_STEP(8); H_STEP(9);
    return diff;
}

// mmapを使用してファイルをメモリ空間に直接マッピング
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
    uint64_t hist;
    char str[16];
} DataEntry;

//出力
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
    if (out_buf_pos > 0) fwrite(out_buffer, 1, out_buf_pos, stdout);
}

int main(int argc, char *argv[]) {
    clock_t start_time = clock();
    if (argc != 3) return 1;

    size_t q_sz, idx_sz;
    char* q_data = map_file(argv[1], &q_sz);
    char* idx_ptr = map_file(argv[2], &idx_sz);

    int num_lines = *(int*)idx_ptr;
    char* current_ptr = idx_ptr + sizeof(int);
    
    int* offsets[4];
    DataEntry* data_lists[4];
    
    // インデックスファイルをメモリ上のポインタとして再構築
    for(int p=0; p<4; p++) {
        offsets[p] = (int*)current_ptr;
        current_ptr += sizeof(int) * (MAX_BUCKETS + 1);
        data_lists[p] = (DataEntry*)current_ptr;
        current_ptr += sizeof(DataEntry) * num_lines;
    }

    int q_count = q_sz / LINE_LEN;

    // クエリごとにループ
    for (int i = 0; i < q_count; i++) {
        const char* q_str = q_data + i * LINE_LEN;
        PatternData pdata;
        precompute_pattern(q_str, &pdata);
        uint64_t q_hist = make_hist(q_str);
        
        // 文字列を64bit整数として一気にロード（ハミング距離計算用）
        uint64_t q_u64_1 = *(uint64_t*)q_str;
        uint64_t q_u64_2 = *(uint64_t*)(q_str + 8);

        int found = 0;

        //鳩の巣原理による探索
        for (int p = 0; p < 4; p++) {
            uint16_t key = encode_part(q_str, PART_OFFSET[p], PART_LEN[p]);
            int start = offsets[p][key];
            int end   = offsets[p][key + 1];

            // 同じパーツを持つ候補データを順次チェック
            for (int k = start; k < end; k++) {
                DataEntry* entry = &data_lists[p][k];
                
                //ヒストグラムチェック（超高速排除）
                if (hist_diff(q_hist, entry->hist) > 6) continue;

                //ハミング距離（置換のみの判定）
                uint64_t* db_u64 = (uint64_t*)entry->str;
                uint64_t diff1 = q_u64_1 ^ db_u64[0];
                uint64_t diff2 = q_u64_2 ^ db_u64[1];
                
                //ビット差異のカウント。3bit以下なら確実に編集距離3以内
                int hamming = __builtin_popcountll(diff1) + __builtin_popcountll(diff2 & 0x00FFFFFFFFFFFFFFULL);
                
                if (hamming <= 3) {
                    found = 1;
                    goto NEXT_QUERY;
                }

                // Myers（挿入・削除を含む厳密な編集距離計算）
                if (myers_distance(entry->str, &pdata) <= THRESHOLD) {
                    found = 1;
                    goto NEXT_QUERY;
                }
            }
        }
        NEXT_QUERY:
        fast_put(found ? '1' : '0'); // 結果をバッファへ
    }
    
    fast_put('\n'); 
    flush_out();
    
    // 実行時間を標準エラー出力に表示（リダイレクトを汚さないため）
    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    fprintf(stderr, "Exec Time: %.6f sec\n", time_spent);
    return 0;
}