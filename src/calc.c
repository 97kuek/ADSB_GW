#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ground_truth.txt> <my_output.txt>\n", argv[0]);
        return 1;
    }

    FILE *f_gt = fopen(argv[1], "r");  // 模範解答（改行あり想定）
    FILE *f_my = fopen(argv[2], "r");  // 自分の出力（改行なし想定）

    if (!f_gt || !f_my) {
        perror("File open error");
        return 1;
    }

    long hamming_distance = 0;
    long total_compared = 0;
    int c_gt, c_my;

    while (1) {
        // 模範解答から改行(LF/CR)をスキップして1文字読む
        do {
            c_gt = fgetc(f_gt);
        } while (c_gt == '\n' || c_gt == '\r');

        // 自分の出力から1文字読む（改行は無視しない）
        c_my = fgetc(f_my);

        // 両方終端に達したら終了
        if (c_gt == EOF || c_my == EOF) {
            break;
        }

        // 比較
        if (c_gt != c_my) {
            hamming_distance++;
        }
        total_compared++;
    }

    printf("--- Result ---\n");
    printf("Total Queries Compared: %ld\n", total_compared);
    printf("Hamming Distance      : %ld\n", hamming_distance);
    
    if (hamming_distance == 0) {
        printf("PERFECT! Accuracy is 100%%.\n");
    } else {
        double accuracy = (double)(total_compared - hamming_distance) / total_compared * 100.0;
        printf("Accuracy Rate         : %.4f%%\n", accuracy);
    }

    fclose(f_gt);
    fclose(f_my);
    return 0;
}