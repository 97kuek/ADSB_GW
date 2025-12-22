// search_group1.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h> // 追加

#define THRESHOLD 3

typedef struct __attribute__((packed)) {
    char key[4];
    uint32_t id;
} IndexEntry;

static inline int edit_dist(const char *s1, const char *s2) {
    int dp[16][16];
    for (int i = 0; i <= 15; i++) dp[i][0] = i;
    for (int j = 0; j <= 15; j++) dp[0][j] = j;
    for (int i = 1; i <= 15; i++) {
        for (int j = 1; j <= 15; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            int a = dp[i-1][j] + 1;
            int b = dp[i][j-1] + 1;
            int c = dp[i-1][j-1] + cost;
            dp[i][j] = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
        }
    }
    return dp[15][15];
}

int main() {
    // --- 初期化・ロード処理 ---
    int fd = open("index.bin", O_RDONLY);
    if (fd < 0) { perror("index.bin"); return 1; }
    struct stat st;
    fstat(fd, &st);
    uint8_t *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) { perror("mmap"); return 1; }
    
    int n = *(int*)map;
    char (*db)[15] = (char (*)[15])(map + sizeof(int));
    IndexEntry *idxs[4];
    uint8_t *ptr = (uint8_t*)(map + sizeof(int) + 15 * n);
    for (int i = 0; i < 4; i++) {
        idxs[i] = (IndexEntry*)ptr;
        ptr += sizeof(IndexEntry) * n;
    }

    FILE *qf = fopen("query_test", "r");
    if (!qf) { perror("query_1"); return 1; }

    char *result_buf = malloc(1000000 + 10);
    int res_idx = 0;
    char query[32];
    int offsets[4] = {0, 4, 8, 11};

    // --- 時間計測開始 ---
    struct timespec start_ts, end_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);

    // 3. 検索ループ
    while (fscanf(qf, "%s", query) == 1) {
        int found = 0;
        if (strlen(query) != 15) continue;

        for (int p = 0; p < 4 && !found; p++) {
            char qk[4];
            memcpy(qk, &query[offsets[p]], 4);

            int L = 0, R = n - 1;
            while (L <= R) {
                int mid = L + (R - L) / 2;
                int cmp = memcmp(idxs[p][mid].key, qk, 4);
                if (cmp == 0) {
                    for (int i = mid; i >= 0 && memcmp(idxs[p][i].key, qk, 4) == 0; i--) {
                        if (edit_dist(query, db[idxs[p][i].id]) <= THRESHOLD) {
                            found = 1; break;
                        }
                    }
                    for (int i = mid + 1; !found && i < n && memcmp(idxs[p][i].key, qk, 4) == 0; i++) {
                        if (edit_dist(query, db[idxs[p][i].id]) <= THRESHOLD) {
                            found = 1; break;
                        }
                    }
                    break;
                }
                if (cmp < 0) L = mid + 1;
                else R = mid - 1;
            }
        }
        result_buf[res_idx++] = found ? '1' : '0';
    }

    // --- 時間計測終了 ---
    clock_gettime(CLOCK_MONOTONIC, &end_ts);
    
    // 秒とナノ秒を計算
    double elapsed = (end_ts.tv_sec - start_ts.tv_sec) + 
                     (end_ts.tv_nsec - start_ts.tv_nsec) * 1e-9;

    result_buf[res_idx++] = '\n';

    // 4. 結果保存
    FILE *rf = fopen("result.txt", "w");
    if (rf) {
        fwrite(result_buf, 1, res_idx, rf);
        fclose(rf);
    }

    // --- 時間の出力 ---
    printf("Search finished in: %.6f seconds\n", elapsed);

    // 5. 後処理
    fclose(qf);
    munmap(map, st.st_size);
    close(fd);
    free(result_buf);

    return 0;
}