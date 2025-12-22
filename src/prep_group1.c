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

/* ================= ハッシュ ================= */

static unsigned hash(const char *s) {
    unsigned h = 0;
    for (int i = 0; i < Q; i++)
        h = h * 31 + (unsigned)s[i];
    return h % HASH_SIZE;
}

static Node *find_or_create(const char *g) {
    unsigned h = hash(g);
    for (Node *p = table[h]; p; p = p->next)
        if (strcmp(p->gram, g) == 0)
            return p;

    Node *n = calloc(1, sizeof(Node));
    strcpy(n->gram, g);
    n->cap = 8;
    n->ids = malloc(n->cap * sizeof(int));

    n->next = table[h];
    table[h] = n;
    return n;
}

/* ================= main ================= */

int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }

    FILE *db = fopen(argv[1], "r");
    if (!db) {
        return 1;
    }

    char **B = malloc(MAXN * sizeof(char *));
    if (!B) {
        fclose(db);
        return 1;
    }
    
    int N = 0;
    char buf[STRLEN+2];

    while (N < MAXN && fscanf(db, "%15s", buf) == 1) {
        B[N] = strdup(buf);
        if (!B[N]) {
            fclose(db);
            return 1;
        }

        int len = strlen(buf);
        for (int i = 0; i <= len - Q; i++) {
            char g[Q+1];
            memcpy(g, buf + i, Q);
            g[Q] = 0;

            Node *n = find_or_create(g);
            if (n->size == n->cap) {
                n->cap *= 2;
                int *new_ids = realloc(n->ids, n->cap * sizeof(int));
                if (!new_ids) {
                    fclose(db);
                    return 1;
                }
                n->ids = new_ids;
            }
            n->ids[n->size++] = N;
        }
        N++;
    }
    fclose(db);

    /* ================= 出力 ================= */
    
    printf("#DB\n");
    printf("%d\n", N);
    for (int i = 0; i < N; i++)
        printf("%d %s\n", i, B[i]);

    printf("#INDEX\n");
    for (int h = 0; h < HASH_SIZE; h++) {
        for (Node *p = table[h]; p; p = p->next) {
            printf("%s", p->gram);
            for (int i = 0; i < p->size; i++)
                printf(" %d", p->ids[i]);
            printf("\n");
        }
    }

    /* ================= メモリ解放 ================= */
    
    for (int i = 0; i < N; i++) {
        free(B[i]);
    }
    free(B);
    
    for (int h = 0; h < HASH_SIZE; h++) {
        Node *p = table[h];
        while (p) {
            Node *next = p->next;
            free(p->ids);
            free(p);
            p = next;
        }
    }

    return 0;
}