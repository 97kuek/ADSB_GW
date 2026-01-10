// search_1_v8.c (Optimization: Columnar SoA + AVX2 32-way Filter)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <immintrin.h>

#pragma GCC target("avx2,popcnt")

#define STR_LEN 15
#define LINE_LEN 16
#define THRESHOLD 3
#define MAX_BUCKETS 10000

typedef struct { uint64_t P_eq[10]; } PatternData;

// Forward declaration
static inline void check_candidate(int k, char* s_ptr, uint64_t q1, uint64_t q2, const PatternData* pdata, int* found);

// Myers
static inline int myers_distance(const char* text, const PatternData* pdata) {
    uint64_t VP = ~0ULL, VN = 0ULL;
    int score = STR_LEN;
    uint64_t PM, D0, HP, HN;
    #define STEP(i) \
        PM = pdata->P_eq[text[i] - 'A']; \
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

void precompute_pattern(const char* pattern, PatternData* pdata) {
    for (int i = 0; i < 10; i++) pdata->P_eq[i] = 0;
    for (int i = 0; i < STR_LEN; i++) {
        pdata->P_eq[pattern[i] - 'A'] |= (1ULL << i);
    }
}

const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

static inline uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) val = val * 10 + (str[offset + i] - 'A');
    return val;
}

char* read_all(const char* filename, size_t* size) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) { perror("fopen"); exit(1); }
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = malloc(*size);
    if (!data) { perror("malloc"); exit(1); }
    if (fread(data, 1, *size, fp) != *size) { perror("fread"); exit(1); }
    fclose(fp);
    return data;
}

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
    char* q_data = read_all(argv[1], &q_sz);
    char* idx_ptr = read_all(argv[2], &idx_sz);

    char* current_ptr = idx_ptr + sizeof(int);
    
    int* offsets[4];
    uint8_t* streams[4][10]; // 10 chars per part
    char* str_lists[4];
    
    for(int p=0; p<4; p++) {
        offsets[p] = (int*)current_ptr;
        current_ptr += sizeof(int) * MAX_BUCKETS;
        
        int count = *(int*)current_ptr;
        current_ptr += sizeof(int);
        
        for(int c=0; c<10; c++) {
            streams[p][c] = (uint8_t*)current_ptr;
            current_ptr += count; // 1 byte per entry
        }
        
        str_lists[p] = (char*)current_ptr;
        current_ptr += LINE_LEN * count;
    }

    int q_count = q_sz / LINE_LEN;
    
    __m256i thresh = _mm256_set1_epi8(6); 
    // Wait, accumulation can exceed 255? 
    // Max diff for one char is 15. Total diff <= 150. Matches fit in uint8.
    // So we can use simple add_epi8. Even if it wraps (unlikely <= 6 check),
    // Wait, if total > 255 it wraps. 
    // SAD can be large. Max diff is 15 * 10 = 150. Safe for uint8.

    for (int i = 0; i < q_count; i++) {
        const char* q_str = q_data + i * LINE_LEN;
        PatternData pdata;
        precompute_pattern(q_str, &pdata);
        
        uint8_t q_cnts[10];
        memset(q_cnts, 0, 10);
        for(int x=0; x<15; x++) q_cnts[q_str[x] - 'A']++;
        
        // Prepare broadcasted vectors for query counts
        __m256i q_vecs[10];
        for(int x=0; x<10; x++) q_vecs[x] = _mm256_set1_epi8(q_cnts[x]);

        uint64_t q_u64_1 = *(uint64_t*)q_str;
        uint64_t q_u64_2 = *(uint64_t*)(q_str + 8);

        int found = 0;

        for (int p = 0; p < 4; p++) {
            uint16_t key = encode_part(q_str, PART_OFFSET[p], PART_LEN[p]);
            int start = offsets[p][key];
            int end   = offsets[p][key + 1];

            char* s_ptr = str_lists[p];
            uint8_t* st[10];
            for(int x=0; x<10; x++) st[x] = streams[p][x];

            int k = start;
            int end_32 = end - 31;
            
            for (; k < end_32; k += 32) {
                // Initialize total diff to 0
                __m256i total = _mm256_setzero_si256();
                
                // Unroll loop for 10 chars? Compiler usually does ok, but manual is finer.
                // We use PSADBW equivalent? No, we have individual bytes.
                // We need ABS(a - b). avx2 doesn't have pabs_epi8 diff instruction directly for subtraction.
                // Use _mm256_subs_epu8 (saturation) logic: (a-b) | (b-a)? Or (a-b) + (b-a)
                // subs_epu8 handles unsigned saturation (0 if a<b).
                // So abs(a-b) = subs(a,b) + subs(b,a).
                
                // Char 0
                __m256i v0 = _mm256_loadu_si256((__m256i const*)&st[0][k]);
                total = _mm256_add_epi8(total, _mm256_or_si256(_mm256_subs_epu8(v0, q_vecs[0]), _mm256_subs_epu8(q_vecs[0], v0)));
                
                // Char 1
                __m256i v1 = _mm256_loadu_si256((__m256i const*)&st[1][k]);
                total = _mm256_add_epi8(total, _mm256_or_si256(_mm256_subs_epu8(v1, q_vecs[1]), _mm256_subs_epu8(q_vecs[1], v1)));

                // ... Char 2-9
                #define ACC(id) \
                    { \
                       __m256i v = _mm256_loadu_si256((__m256i const*)&st[id][k]); \
                       total = _mm256_add_epi8(total, _mm256_or_si256(_mm256_subs_epu8(v, q_vecs[id]), _mm256_subs_epu8(q_vecs[id], v))); \
                    }
                
                ACC(2); ACC(3); ACC(4); ACC(5); ACC(6); ACC(7); ACC(8); ACC(9);
                
                // Check <= 6. We use PCMPGT again?
                // total is epi8. thresh is epi8(6).
                // We want !(total > 6)
                // However, unsigned compare for epu8 is not directly available in AVX2 (only min/max/subs).
                // Actually _mm256_cmpgt_epi8 is SIGNED.
                // But our max value is 150 which fits in pos signed char (0-127)? No, 150 overflows signed char!
                // Wait. 150 is -106 in int8.
                // So checking > 6 with signed compare is risky if values > 127.
                // Solution: use _mm256_max_epu8(total, thresh) == thresh?
                // If total <= 6, max(total, 6) is 6. If total > 6, max is total.
                // So check if max(total, 6) == 6.
                
                __m256i vmax = _mm256_max_epu8(total, thresh);
                __m256i vcmp = _mm256_cmpeq_epi8(vmax, thresh); // 0xFF where total <= 6
                
                int mask = _mm256_movemask_epi8(vcmp);
                
                if (mask != 0) {
                    // Iterate set bits
                    // Since bits correspond to byte lanes 0-31
                   // If mask has bit set, check candidate.
                   // Optimize: `__builtin_ctz` to skip zeros
                   while(mask) {
                       int idx = __builtin_ctz(mask);
                       check_candidate(k + idx, s_ptr, q_u64_1, q_u64_2, &pdata, &found);
                       if(found) goto NEXT_QUERY; // Found
                       mask &= ~(1 << idx);
                   }
                }
            }

            for (; k < end; k++) {
                 // Scalar tail fallback
                 int diff = 0;
                 for(int c=0; c<10; c++) {
                     int val = st[c][k];
                     diff += abs(val - q_cnts[c]);
                 }
                 if (diff <= 6) {
                     check_candidate(k, s_ptr, q_u64_1, q_u64_2, &pdata, &found);
                     if(found) goto NEXT_QUERY;
                 }
            }
        }
        NEXT_QUERY:
        fast_put(found ? '1' : '0');
    }
    
    fast_put('\n'); 
    flush_out();
    
    // Timing output to stderr (comment out for final)
    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    // fprintf(stderr, "Exec Time: %.6f sec\n", time_spent);
    return 0;
}

static inline void check_candidate(int k, char* s_ptr, uint64_t q1, uint64_t q2, const PatternData* pdata, int* found) {
    const char* entry_str = &s_ptr[k * LINE_LEN];
    uint64_t* db_u64 = (uint64_t*)entry_str;
    
    uint64_t x1 = q1 ^ db_u64[0];
    uint64_t x2 = q2 ^ db_u64[1];
    int hamming = __builtin_popcountll(x1) + __builtin_popcountll(x2 & 0x00FFFFFFFFFFFFFFULL);
    
    if (hamming <= 3) {
        *found = 1;
        return;
    }
    if (myers_distance(entry_str, pdata) <= THRESHOLD) {
        *found = 1;
        return;
    }
}