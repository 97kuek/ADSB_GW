#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATTERNS 10000
#define DB_SIZE 1000000
#define STR_LEN 15

// ピボット2つ
const char *PIVOTS[2] = {
    "AAAAAAAAAAAAAAA", 
    "ABCDEFGHIJKLMNO"
};

// 検査用データ (8バイト)
typedef struct {
    unsigned char len;          
    unsigned char hist_pack[5]; 
    unsigned char piv_dist[2];  
} Metadata;

// IDデータ (4バイト)
typedef int RowID;

// 一時保存用
typedef struct {
    Metadata meta;
    RowID id;
} TempEntry;

int counts[4][MAX_PATTERNS];
TempEntry *temp_data[4][MAX_PATTERNS];

int char_to_int(char c) { return c - 'A'; }

int get_part_id(const char *str, int start, int len) {
    int id = 0;
    for (int i = 0; i < len; i++) {
        id = id * 10 + char_to_int(str[start + i]);
    }
    return id;
}

void make_hist(const char *str, int len, unsigned char *hist) {
    for(int i=0; i<10; i++) hist[i] = 0;
    for(int i=0; i<len; i++) {
        int c = str[i] - 'A';
        if (c >= 0 && c < 10) hist[c]++;
    }
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a, b, c) (MIN(MIN(a, b), c))

// ピボット用距離計算（常に15文字分比較するので、後ろが0埋めされている必要がある）
int calc_full_dist(const char *s1, const char *s2) {
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

int main(int argc, char *argv[]) {
    if (argc < 2) { printf("Usage: .\\prep5.exe test\\db_1\n"); return 1; }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) return 1;

    printf("1. Counting patterns...\n");
    char line[100];
    int line_id = 0;

    // 1周目
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        int len = strlen(line);
        // ★修正点: ゴミ掃除
        memset(line + len, 0, sizeof(line) - len);
        
        if (len < 10) continue; 
        int ids[4];
        ids[0] = get_part_id(line, 0, 4);
        ids[1] = get_part_id(line, 4, 4);
        ids[2] = get_part_id(line, 8, 4);
        ids[3] = get_part_id(line, 12, 3);
        for(int p=0; p<4; p++) counts[p][ids[p]]++;
        line_id++;
    }

    // メモリ確保
    for (int p = 0; p < 4; p++) {
        for (int i = 0; i < MAX_PATTERNS; i++) {
            if (counts[p][i] > 0) {
                temp_data[p][i] = (TempEntry*)malloc(counts[p][i] * sizeof(TempEntry));
            }
            counts[p][i] = 0; // リセット
        }
    }

    // 2周目
    printf("2. Building SoA Data...\n");
    rewind(fp);
    line_id = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        int len = strlen(line);
        
        // ★修正点: ゴミ掃除 (重要！)
        // ここで後ろを0埋めしないと、ピボット距離計算が狂う
        memset(line + len, 0, sizeof(line) - len);

        if (len < 10) continue;

        int ids[4];
        ids[0] = get_part_id(line, 0, 4);
        ids[1] = get_part_id(line, 4, 4);
        ids[2] = get_part_id(line, 8, 4);
        ids[3] = get_part_id(line, 12, 3);

        unsigned char h[10];
        make_hist(line, len, h);

        unsigned char d1 = (unsigned char)calc_full_dist(line, PIVOTS[0]);
        unsigned char d2 = (unsigned char)calc_full_dist(line, PIVOTS[1]);

        for(int p=0; p<4; p++) {
            int id = ids[p];
            int idx = counts[p][id]++;
            TempEntry *entry = &temp_data[p][id][idx];
            
            entry->id = line_id;
            entry->meta.len = (unsigned char)len;
            entry->meta.piv_dist[0] = d1;
            entry->meta.piv_dist[1] = d2;
            
            for(int i=0; i<5; i++) {
                entry->meta.hist_pack[i] = (h[i*2] << 4) | (h[i*2+1] & 0x0F);
            }
        }
        line_id++;
        if (line_id % 20000 == 0) printf("\rProcessed: %d", line_id);
    }
    printf("\n");
    fclose(fp);

    // 3. 書き出し
    printf("3. Writing to index5.bin...\n");
    FILE *out = fopen("index5.bin", "wb");
    
    fwrite(counts, sizeof(int), 4 * MAX_PATTERNS, out);

    for (int p = 0; p < 4; p++) {
        for (int i = 0; i < MAX_PATTERNS; i++) {
            int num = counts[p][i];
            if (num > 0) {
                for(int k=0; k<num; k++) {
                    fwrite(&temp_data[p][i][k].meta, sizeof(Metadata), 1, out);
                }
                for(int k=0; k<num; k++) {
                    fwrite(&temp_data[p][i][k].id, sizeof(RowID), 1, out);
                }
                free(temp_data[p][i]);
            }
        }
    }

    fclose(out);
    printf("Done! Created SoA Index.\n");
    return 0;
}
