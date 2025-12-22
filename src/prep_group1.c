// prep_group1.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct __attribute__((packed)) {
    char key[4];
    uint32_t id;
} IndexEntry;

typedef struct {
    char str[15]; // ヌル文字を含めない15バイト固定
} RawEntry;

int compare_idx(const void *a, const void *b) {
    return memcmp(((IndexEntry*)a)->key, ((IndexEntry*)b)->key, 4);
}

int main() {
    FILE *fp = fopen("db_test", "r");
    if (!fp) return 1;

    char line[32];
    RawEntry *db = malloc(sizeof(RawEntry) * 1000000);
    int n = 0;
    while (n < 1000000 && fscanf(fp, "%s", line) == 1) {
        memcpy(db[n].str, line, 15);
        n++;
    }
    fclose(fp);

    FILE *out = fopen("index.bin", "wb");
    fwrite(&n, sizeof(int), 1, out);
    fwrite(db, 15, n, out); // 文字列実体（15バイト × n）

    IndexEntry *idx = malloc(sizeof(IndexEntry) * n);
    int offsets[4] = {0, 4, 8, 11};
    for (int p = 0; p < 4; p++) {
        for (int i = 0; i < n; i++) {
            memcpy(idx[i].key, &db[i].str[offsets[p]], 4);
            idx[i].id = i;
        }
        qsort(idx, n, sizeof(IndexEntry), compare_idx);
        fwrite(idx, sizeof(IndexEntry), n, out);
    }
    fclose(out);
    return 0;
}