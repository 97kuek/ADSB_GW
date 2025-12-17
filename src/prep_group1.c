// prep_group1.c

//test comment(hoshina)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define LINE_LEN 16
#define MAX_BUCKETS 10000

const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

typedef struct {
    char str[16];
} DataEntry;

uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) {
        val = val * 10 + (str[offset + i] - 'A');
    }
    return val;
}

char* read_all(const char* fname, size_t* size) {
    FILE* fp = fopen(fname, "rb");
    if(!fp) exit(1);
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = malloc(*size);
    if(!data) exit(1);
    if (fread(data, 1, *size, fp) != *size) exit(1);
    fclose(fp);
    return data;
}

int main(int argc, char* argv[]) {
    if (argc != 2) return 1;

    size_t db_size;
    char* db_data = read_all(argv[1], &db_size);
    int num_lines = db_size / LINE_LEN;

    fwrite(&num_lines, sizeof(int), 1, stdout);

    static int counts[MAX_BUCKETS];
    static int offsets[MAX_BUCKETS];
    
    DataEntry* sorted_data = malloc(sizeof(DataEntry) * num_lines);
    if(!sorted_data) return 1;

    for (int p = 0; p < 4; p++) {
        memset(counts, 0, sizeof(counts));
        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            uint16_t key = encode_part(str, PART_OFFSET[p], PART_LEN[p]);
            counts[key]++;
        }

        int current_idx = 0;
        for (int k = 0; k < MAX_BUCKETS; k++) {
            offsets[k] = current_idx;
            current_idx += counts[k];
        }

        fwrite(offsets, sizeof(int), MAX_BUCKETS, stdout);
        int total_count = num_lines; 
        fwrite(&total_count, sizeof(int), 1, stdout); 

        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            uint16_t key = encode_part(str, PART_OFFSET[p], PART_LEN[p]);
            int pos = offsets[key]++; 
            memcpy(sorted_data[pos].str, str, LINE_LEN);
        }
        
        fwrite(sorted_data, sizeof(DataEntry), num_lines, stdout);
    }
    free(sorted_data);
    free(db_data);
    return 0;
}
