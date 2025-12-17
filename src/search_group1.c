// search_group1.c

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

typedef struct { uint64_t P_eq[10]; } PatternData;

// 【修正】static inline に変更してリンクエラーを回避
static inline int myers_distance(const char* text, PatternData* pdata) {
    uint64_t VP = ~0ULL, VN = 0ULL;
    int score = STR_LEN;
    uint64_t PM, D0, HP, HN;
    int char_idx;

    // ループアンローリング
    #define STEP(i) \
        char_idx = text[i] - 'A'; \
        PM = (char_idx >= 0 && char_idx < 10) ? pdata->P_eq[char_idx] : 0; \
        D0 = (((PM & VP) + VP) ^ VP) | PM | VN; \
        HP = VN | ~(D0 | VP); \
        HN = VP & D0; \
        if (HP & (1ULL << 14)) score++; \
        if (HN & (1ULL << 14)) score--; \
        HP = (HP << 1) | 1ULL; HN = (HN << 1); \
        VP = HN | ~(D0 | HP); VN = HP & D0;

    STEP(0); STEP(1); STEP(2); STEP(3); STEP(4);
    STEP(5); STEP(6); STEP(7); STEP(8); STEP(9);
    STEP(10); STEP(11); STEP(12); STEP(13); STEP(14);

    return score;
}

void precompute_pattern(const char* pattern, PatternData* pdata) {
    for (int i = 0; i < 10; i++) pdata->P_eq[i] = 0;
    for (int i = 0; i < STR_LEN; i++) {
        int char_idx = pattern[i] - 'A';
        if (char_idx >= 0 && char_idx < 10) pdata->P_eq[char_idx] |= (1ULL << i);
    }
}

const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) val = val * 10 + (str[offset + i] - 'A');
    return val;
}

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

#define OUT_BUF_SIZE 65536
char out_buffer[OUT_BUF_SIZE];
int out_buf_pos = 0;

// 【修正】static inline に変更
static inline void fast_put(char c) {
    out_buffer[out_buf_pos++] = c;
    if (out_buf_pos >= OUT_BUF_SIZE) {
        fwrite(out_buffer, 1, OUT_BUF_SIZE, stdout);
        out_buf_pos = 0;
    }
}

// 【修正】static inline に変更
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

    int num_lines = *(int*)idx_ptr;
    char* current_ptr = idx_ptr + sizeof(int);
    
    int* offsets[4];
    DataEntry* data_lists[4];
    
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

            // シーケンシャルアクセス
            for (int k = start; k < end; k++) {
                const char* db_str = data_lists[p][k].str;
                if (myers_distance(db_str, &pdata) <= THRESHOLD) {
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
