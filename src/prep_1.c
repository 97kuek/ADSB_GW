#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#define LINE_LEN 16
#define MAX_BUCKETS 10000 

const int PART_OFFSET[4] = {0, 4, 8, 12};
const int PART_LEN[4]    = {4, 4, 4, 3};

// Columnar Histogram logic:
// We don't pack hist here. We just extract counts.

uint16_t encode_part(const char* str, int offset, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) {
        val = val * 10 + (str[offset + i] - 'A');
    }
    return val;
}

char* read_all(const char* fname, size_t* size) {
    FILE* fp = fopen(fname, "rb");
    if(!fp) { perror("fopen"); exit(1); }
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = malloc(*size);
    if (!data) { perror("malloc"); exit(1); }
    if (fread(data, 1, *size, fp) != *size) { perror("fread"); exit(1); }
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
    
    for (int p = 0; p < 4; p++) {
        int start_s = (p == 0) ? 0 : -1;
        int end_s   = (p == 3) ? 0 : 1;

        memset(counts, 0, sizeof(counts));
        int entries_count = 0;
        
        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            for (int s = start_s; s <= end_s; s++) {
                uint16_t key = encode_part(str, PART_OFFSET[p] + s, PART_LEN[p]);
                counts[key]++;
                entries_count++;
            }
        }

        int current_idx = 0;
        for (int k = 0; k < MAX_BUCKETS; k++) {
            offsets[k] = current_idx;
            current_idx += counts[k];
        }

        fwrite(offsets, sizeof(int), MAX_BUCKETS, stdout);
        fwrite(&entries_count, sizeof(int), 1, stdout); 

        char** sorted_ptrs = malloc(sizeof(char*) * entries_count);

        for (int i = 0; i < num_lines; i++) {
            const char* str = db_data + i * LINE_LEN;
            for (int s = start_s; s <= end_s; s++) {
                uint16_t key = encode_part(str, PART_OFFSET[p] + s, PART_LEN[p]);
                int pos = offsets[key]++; 
                sorted_ptrs[pos] = (char*)str;
            }
        }
        
        // COLUMNAR WRITEOUT
        // Write 10 streams of uint8_t
        // Stream 0: A counts for all entries
        // Stream 1: B counts...
        #define W_BUF_SZ 65536
        uint8_t* val_buf = malloc(W_BUF_SZ);
        
        for(int char_idx=0; char_idx<10; char_idx++) {
            for(int k=0; k<entries_count; k+=W_BUF_SZ) {
                int block = (k + W_BUF_SZ <= entries_count) ? W_BUF_SZ : (entries_count - k);
                for(int j=0; j<block; j++) {
                    // Count specific char in str
                    const char* s = sorted_ptrs[k+j];
                    int c = 0;
                    // Unroll this count manual optimization?
                    // "For i in 0..14 if s[i] == char_idx + 'A' c++"
                    char target = (char)('A' + char_idx);
                    for(int x=0; x<15; x++) if(s[x] == target) c++;
                    val_buf[j] = (uint8_t)c;
                }
                fwrite(val_buf, 1, block, stdout);
            }
        }
        free(val_buf);

        // Then strings
        char* s_w_buf = malloc(LINE_LEN * W_BUF_SZ);
        for(int k=0; k<entries_count; k+=W_BUF_SZ) {
            int block = (k + W_BUF_SZ <= entries_count) ? W_BUF_SZ : (entries_count - k);
            for(int j=0; j<block; j++) {
                memcpy(&s_w_buf[j*LINE_LEN], sorted_ptrs[k+j], LINE_LEN);
            }
            fwrite(s_w_buf, LINE_LEN, block, stdout);
        }
        free(s_w_buf);

        free(sorted_ptrs);
    }

    free(db_data);
    return 0;
}