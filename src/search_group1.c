#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRLEN 15
#define Q 3
#define MAXN 1000000
#define HASH_SIZE 500009

typedef struct Node {
    char gram[Q+1];
    int *ids;
    int size;
    int cap;
    struct Node *next;
} Node;

static Node *table[HASH_SIZE];
static char **B;
static int N_db = 0;
static int checked[MAXN]; 
static int query_tick = 0;

/* ================= ハッシュ関数 ================= */
static unsigned hash(const char *s) {
    unsigned h = 0;
    for (int i = 0; i < Q; i++)
        h = h * 31 + (unsigned)s[i];
    return h % HASH_SIZE;
}

static void add_to_index(const char *g, int id) {
    unsigned h = hash(g);
    Node *target = NULL;
    for (Node *p = table[h]; p; p = p->next) {
        if (p->gram[0] == g[0] && p->gram[1] == g[1] && p->gram[2] == g[2]) {
            target = p;
            break;
        }
    }
    if (!target) {
        target = calloc(1, sizeof(Node));
        memcpy(target->gram, g, Q);
        target->cap = 8;
        target->ids = malloc(target->cap * sizeof(int));
        target->next = table[h];
        table[h] = target;
    }
    if (target->size == target->cap) {
        target->cap *= 2;
        target->ids = realloc(target->ids, target->cap * sizeof(int));
    }
    target->ids[target->size++] = id;
}

static Node *get_node(const char *g) {
    unsigned h = hash(g);
    for (Node *p = table[h]; p; p = p->next)
        if (p->gram[0] == g[0] && p->gram[1] == g[1] && p->gram[2] == g[2]) return p;
    return NULL;
}

/* ================= 編集距離 (DP) ================= */
static int edit_distance(const char *s, const char *t) {
    int dp[16][16];
    for (int i = 0; i <= 15; i++) dp[i][0] = i;
    for (int j = 0; j <= 15; j++) dp[0][j] = j;

    for (int i = 1; i <= 15; i++) {
        for (int j = 1; j <= 15; j++) {
            int cost = (s[i-1] == t[j-1]) ? 0 : 1;
            int a = dp[i-1][j] + 1;
            int b = dp[i][j-1] + 1;
            int c = dp[i-1][j-1] + cost;
            int min = (a < b) ? a : b;
            dp[i][j] = (min < c) ? min : c;
        }
    }
    return dp[15][15];
}

/* ================= メイン処理 ================= */
int main(int argc, char **argv) {
    if (argc != 3) return 1;

    FILE *idx_fp = fopen(argv[2], "r");
    if (!idx_fp) return 1;

    char tag[32];
    // 1. #DBセクション
    if (fscanf(idx_fp, "%s", tag) == 1 && strcmp(tag, "#DB") == 0) {
        if (fscanf(idx_fp, "%d", &N_db) != 1) return 1;
        B = malloc(N_db * sizeof(char *));
        for (int i = 0; i < N_db; i++) {
            int id;
            char str[STRLEN + 2];
            if (fscanf(idx_fp, "%d %s", &id, str) == 2) {
                B[id] = strdup(str);
            }
        }
    }

    // 2. #INDEXセクション
    // fscanf(idx_fp, "%s", tag) で "#INDEX" をスキップ
    while (fscanf(idx_fp, "%s", tag) == 1) {
        if (strcmp(tag, "#INDEX") == 0) break;
    }

    // 各行の読み込み: [gram] [id1] [id2] ... [改行]
    // prep_2.c の出力を効率よく読み込むために fscanf を活用
    char current_gram[Q + 1];
    while (fscanf(idx_fp, "%s", current_gram) == 1) {
        // gramは必ず3文字。もしそれ以上なら読み込みエラーか次のセクション
        if (strlen(current_gram) != Q) break; 
        
        while (1) {
            int id;
            int ret = fscanf(idx_fp, "%d", &id);
            if (ret == 1) {
                add_to_index(current_gram, id);
            } else {
                break; // 次のgramへ
            }
        }
    }
    fclose(idx_fp);

    // 3. クエリ処理
    FILE *q_fp = fopen(argv[1], "r");
    if (!q_fp) return 1;

    char query[STRLEN + 2];
    while (fscanf(q_fp, "%s", query) == 1) {
        int found = 0;
        query_tick++;
        
        for (int i = 0; i <= STRLEN - Q; i++) {
            char g[Q + 1];
            memcpy(g, query + i, Q);
            g[Q] = 0;

            Node *n = get_node(g);
            if (n) {
                for (int j = 0; j < n->size; j++) {
                    int db_id = n->ids[j];
                    if (checked[db_id] == query_tick) continue;
                    checked[db_id] = query_tick;

                    if (edit_distance(query, B[db_id]) <= 3) {
                        found = 1;
                        break;
                    }
                }
            }
            if (found) break;
        }
        putchar(found ? '1' : '0');
    }
    putchar('\n');

    fclose(q_fp);
    return 0;
}