#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATTERNS 10000
#define DB_SIZE 1000000
#define STR_LEN 15

const char *PIVOTS[2] = {
    "AAAAAAAAAAAAAAA", 
    "ABCDEFGHIJKLMNO"
};

typedef struct {
    unsigned char len;
    unsigned char hist_pack[5];
    unsigned char piv_dist[2];
} Metadata;

typedef int RowID;

char database[DB_SIZE][STR_LEN + 1];
int counts[4][MAX_PATTERNS];
Metadata *meta_data[4][MAX_PATTERNS];
RowID *row_ids[4][MAX_PATTERNS];
int visited[DB_SIZE];
int current_query_id = 0;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#define ABS(a) ((a) < 0 ? -(a) : (a))

int char_to_int(char c) { return c - 'A'; }
int get_part_id(const char *str, int start, int len) {
    int id = 0;
    for (int i = 0; i < len; i++) id = id * 10 + char_to_int(str[start + i]);
    return id;
}
void make_hist(const char *str, int len, unsigned char *hist) {
    for(int i=0; i<10; i++) hist[i] = 0;
    for(int i=0; i<len; i++) {
        int c = str[i] - 'A';
        if (c >= 0 && c < 10) hist[c]++;
    }
}
static inline int calc_full_dist(const char *s1, const char *s2) {
    int len = STR_LEN;
    int dp[16][16];
    for(int i=0; i<=len; i++) dp[i][0] = i;
    for(int j=0; j<=len; j++) dp[0][j] = j;
    for(int i=1; i<=len; i++) {
        for(int j=1; j<=len; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            dp[i][j] = MIN3(dp[i-1][j]+1, dp[i][j-1]+1, dp[i-1][j-1]+cost);
        }
    }
    return dp[len][len];
}
static inline int calc_edit_dist(const char *s1, int len1, const char *s2, int len2, int limit) {
    int dp[17][17];
    for (int i = 0; i <= len1; i++) dp[i][0] = i;
    for (int j = 0; j <= len2; j++) dp[0][j] = j;
    for (int i = 1; i <= len1; i++) {
        int min_val = 100;
        int start_j = (i - limit > 1) ? i - limit : 1;
        int end_j   = (i + limit < len2) ? i + limit : len2;
        for (int j = 1; j <= len2; j++) {
            if (j < start_j || j > end_j) { dp[i][j] = 100; continue; }
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            int v = MIN3(dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost);
            dp[i][j] = v;
            if (v < min_val) min_val = v;
        }
        if (min_val > limit) return limit + 1;
    }
    return dp[len1][len2];
}

int main(int argc, char *argv[]) {
    if (argc < 3) { fprintf(stderr, "Usage: .\\search5.exe test\\db_1 test\\query_1\n"); return 1; }

    fprintf(stderr, "Loading database...\n");
    FILE *fp_db = fopen(argv[1], "r");
    if (!fp_db) return 1;
    char line[100];
    int row = 0;
    while (fgets(line, sizeof(line), fp_db) && row < DB_SIZE) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) >= 10) { strcpy(database[row], line); row++; }
    }
    fclose(fp_db);

    fprintf(stderr, "Loading index5.bin...\n");
    FILE *fp_idx = fopen("index5.bin", "rb");
    if (!fp_idx) { fprintf(stderr, "Error: Run prep5.exe first.\n"); return 1; }
    
    fread(counts, sizeof(int), 4 * MAX_PATTERNS, fp_idx);
    for (int p = 0; p < 4; p++) {
        for (int i = 0; i < MAX_PATTERNS; i++) {
            if (counts[p][i] > 0) {
                meta_data[p][i] = (Metadata*)malloc(counts[p][i] * sizeof(Metadata));
                fread(meta_data[p][i], sizeof(Metadata), counts[p][i], fp_idx);
                row_ids[p][i] = (RowID*)malloc(counts[p][i] * sizeof(RowID));
                fread(row_ids[p][i], sizeof(RowID), counts[p][i], fp_idx);
            }
        }
    }
    fclose(fp_idx);

    fprintf(stderr, "Start searching (SoA Mode + Fix)...\n");
    FILE *fp_query = fopen(argv[2], "r");
    if (!fp_query) return 1;

    int total_hits = 0;
    unsigned char q_hist[10];

    while (fgets(line, sizeof(line), fp_query)) {
        line[strcspn(line, "\n")] = 0;
        int q_len = strlen(line);
        
        // ★修正点: ゴミ掃除
        memset(line + q_len, 0, sizeof(line) - q_len);

        if (q_len < 10) {
            // 短すぎるクエリはヒット0として出力しておく（行ずれ防止）
            printf("0\n"); 
            continue;
        }

        make_hist(line, q_len, q_hist);
        unsigned char q_d1 = (unsigned char)calc_full_dist(line, PIVOTS[0]);
        unsigned char q_d2 = (unsigned char)calc_full_dist(line, PIVOTS[1]);
        
        current_query_id++;
        int found = 0;
        
        int ids[4];
        ids[0] = get_part_id(line, 0, 4);
        ids[1] = get_part_id(line, 4, 4);
        ids[2] = get_part_id(line, 8, 4);
        ids[3] = get_part_id(line, 12, 3);

        for (int p = 0; p < 4; p++) {
            if (found) break;
            int id = ids[p];
            int num = counts[p][id];
            
            Metadata *metas = meta_data[p][id];
            RowID *rids = row_ids[p][id];

            for (int k = 0; k < num; k++) {
                if (ABS((int)metas[k].len - q_len) > 3) continue;
                if (ABS((int)metas[k].piv_dist[0] - q_d1) > 3) continue;
                if (ABS((int)metas[k].piv_dist[1] - q_d2) > 3) continue;

                int diff_sum = 0;
                unsigned char *pack = metas[k].hist_pack;
                diff_sum += ABS(q_hist[0] - (pack[0] >> 4));
                diff_sum += ABS(q_hist[1] - (pack[0] & 0x0F));
                if (diff_sum > 6) continue;
                diff_sum += ABS(q_hist[2] - (pack[1] >> 4));
                diff_sum += ABS(q_hist[3] - (pack[1] & 0x0F));
                if (diff_sum > 6) continue;
                diff_sum += ABS(q_hist[4] - (pack[2] >> 4));
                diff_sum += ABS(q_hist[5] - (pack[2] & 0x0F));
                if (diff_sum > 6) continue;
                diff_sum += ABS(q_hist[6] - (pack[3] >> 4));
                diff_sum += ABS(q_hist[7] - (pack[3] & 0x0F));
                if (diff_sum > 6) continue;
                diff_sum += ABS(q_hist[8] - (pack[4] >> 4));
                diff_sum += ABS(q_hist[9] - (pack[4] & 0x0F));
                if (diff_sum > 6) continue;

                int target_row = rids[k];
                if (visited[target_row] == current_query_id) continue;
                visited[target_row] = current_query_id;

                if (calc_edit_dist(line, q_len, database[target_row], metas[k].len, 3) <= 3) {
                    found = 1;
                    break;
                }
            }
        }
        if (found) total_hits++;
        
        // ★検証用: 出力を有効化しています
        // 本計測時はここをコメントアウトしてください
        // printf("%d\n", found);
    }
    fclose(fp_query);
    fprintf(stderr, "Finished. Total hits: %d\n", total_hits);
    return 0;
}// search_group1.c
