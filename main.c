#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "extmem.h"
#define BUFFER_SIZE 520
#define BLOCK_SIZE 64
#define MAX_VALUE 9999

void linear_search();
void two_phase_multiway_merge_sort(int first_blk, int last_blk, int first_output_blk);
void selection_based_on_index();
void projection();
void sort_merge_join();
void sort_scan();

typedef struct tuple {
    int x;
    int y;
} Tuple;

Buffer buf;

int main() {
    // 初始化 Buffer
    if(!initBuffer(BUFFER_SIZE, BLOCK_SIZE, &buf)) {
        perror("Buffer Initialization Failed!\n");
        exit(-1);
    }

//    printf("请选择操作：\n"
//           "1. 基于线性搜索的关系选择\n"
//           "2. 两阶段多路归并排序 (TPMMS)\n"
//           "3. 基于索引的关系选择\n"
//           "4. 关系投影\n"
//           "5. 基于排序的连接操作\n"
//           "6. 基于排序或散列的两趟扫描\n");
//    int choice;
//    do {
//        scanf("%d", &choice);
//        switch (choice) {
//            case 1:
//                linear_search();
//                break;
//            case 2:
//                two_phase_multiway_merge_sort();
//                break;
//            case 3:
//                selection_based_on_index();
//                break;
//            case 4:
//                projection();
//                break;
//            case 5:
//                sort_merge_join();
//                break;
//            case 6:
//                sort_scan();
//                break;
//            case 0:
//                break;
//            default:
//                printf("输入有误，请重新输入\n");
//        }
//    } while (choice != 0);

    linear_search();

    two_phase_multiway_merge_sort(1, 16, 201);
    two_phase_multiway_merge_sort(17, 48, 217);

    freeBuffer(&buf);
    return 0;
}

Tuple get_tuple(const unsigned char *blk, int offset) {
    Tuple t;
    char str[5] = "";
    for (int i = 0; i < 4; i++)
    {
        str[i] = (char)*(blk + offset * 8 + i);
    }
    t.x = atoi(str);
    for (int i = 0; i < 4; i++)
    {
        str[i] = (char)*(blk + offset * 8 + 4 + i);
    }
    t.y = atoi(str);

    return t;
}

void write_tuple_to_block(unsigned char *blk, int offset, Tuple t) {
    char str_A[5] = "", str_B[5] = "";
    sprintf(str_A, "%d", t.x);
    sprintf(str_B, "%d", t.y);
    memcpy(blk + offset, str_A, 4);
    memcpy(blk + offset + 4, str_B, 4);
}

void write_tuple_to_block_and_disk(unsigned char **blk, int *addr, int *offset, Tuple t) {
    if (*offset >= 56) {
        char next_addr[9] = "";
        sprintf(next_addr, "%d", *addr + 1);
        memcpy(*blk + 56, next_addr, 8);
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }
        (*addr)++;
        *blk = getNewBlockInBuffer(&buf);
        *offset = 0;
    }

//    char str_A[5] = "", str_B[5] = "";
//    sprintf(str_A, "%d", t.x);
//    sprintf(str_B, "%d", t.y);
//    memcpy(*blk + *offset, str_A, 4);
//    memcpy(*blk + *offset + 4, str_B, 4);
    write_tuple_to_block(*blk, *offset, t);
    *offset += 8;
}

void linear_search() {
    buf.numIO = 0;      // reset

    printf("----------------------------\n"
           "基于线性搜索的选择算法 R.A=30:\n"
           "----------------------------\n");

    unsigned char *rblk;                // 储存 R 关系
    unsigned char *result_blk;          // 储存输出结果
    int start_output_addr = 101;        // 起始输出数据块地址
    int cur_output_addr = 101;          // 当前输出数据块地址
    int offset = 0;                     // 块中偏移量
    int cnt = 0;                        // 满足条件的元组数

    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    for (int addr = 1; addr <= 16; addr++) {
        if ((rblk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        // 获取数据块中的每个元组
        for (int i = 0; i < 7; i++) {
            Tuple t;
            t = get_tuple(rblk, i);

            // R.A = 30
            if (t.x == 30) {
                write_tuple_to_block_and_disk(&result_blk, &cur_output_addr, &offset, t);
                printf("(X=%d, Y=%d)\n", t.x, t.y);
                cnt++;
            }
        }
        freeBlockInBuffer(rblk, &buf);
    }

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 写入下一块的地址
        char next_addr[9] = "";
        sprintf(next_addr, "%d", cur_output_addr + 1);
        memcpy(result_blk + 56, next_addr, 8);

        if (writeBlockToDisk(result_blk, cur_output_addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }
    }

    printf("注：结果写入磁盘：%d\n\n"
           "满足选择条件的元组一共 %d 个。\n\n"
           "IO读写一共 %d 次。\n", start_output_addr, cnt, (int)buf.numIO);
}

int cmp(const void *a, const void *b) {
    return ((Tuple*)a)->x - ((Tuple*)b)->x;
}

void tpmms_p1(int first_blk, int last_blk, int first_temp_blk) {
    int blk_num = last_blk - first_blk + 1;
    int set_num = (int)ceil((double) blk_num / 6);
    int cur_addr = first_blk;
    int cur_temp_addr = first_temp_blk;

    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    for (int i = 0; i < set_num; i++) {
        unsigned char *blk[6];
        Tuple tuples[6 * 7];
        int tuple_cnt = 0;
        // 将每个子集合装入内存
        for (int j = 0; j < 6 && cur_addr <= last_blk; j++, cur_addr++) {
            if ((blk[j] = readBlockFromDisk(cur_addr, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }

            for (int offset = 0; offset < 7; offset++) {
                tuples[j * 7 + offset] = get_tuple(blk[j], offset);
                tuple_cnt++;
            }
        }

        // TODO: 修改排序方式
        qsort(tuples, tuple_cnt, sizeof(Tuple), cmp);

        int offset = 0;
        for (int j = 0; j < tuple_cnt; j++) {
            write_tuple_to_block_and_disk(&result_blk, &cur_temp_addr, &offset, tuples[j]);
        }

        // 缓冲区中仍有未写回磁盘的数据
        if (offset != 0) {
            // 写入下一块的地址
            char next_addr[9] = "";
            sprintf(next_addr, "%d", cur_temp_addr + 1);
            memcpy(result_blk + 56, next_addr, 8);

            if (writeBlockToDisk(result_blk, cur_temp_addr, &buf) != 0) {
                perror("Writing Block Failed!\n");
                exit(-1);
            }

            cur_temp_addr++;
        }

        for (int j = 0; j < 6; j++) {
            freeBlockInBuffer(blk[j], &buf);
        }
    }

    freeBlockInBuffer(result_blk, &buf);
}

void tpmms_p2(int first_temp_blk, int last_temp_blk, int first_output_blk) {
    int blk_num = last_temp_blk - first_temp_blk + 1;
    int set_num = (int)ceil((double) blk_num / 6);

    // 记录结果
    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    // 用于比较
    unsigned char *m_compare;
    if ((m_compare = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    unsigned char *blk[6];
    int tuple_ptr[set_num];
    int m_ptr[set_num];

    for (int i = 0; i < set_num; i++) {
        if ((blk[i] = readBlockFromDisk(first_temp_blk + i * 6, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }
        Tuple t = get_tuple(blk[i], 0);
        tuple_ptr[i] = 1;
        write_tuple_to_block(m_compare, i * 8, t);
        m_ptr[i] = 0;
    }
}

void two_phase_multiway_merge_sort(int first_blk, int last_blk, int first_output_blk) {
    buf.numIO = 0;      // reset

    int first_temp_blk = first_output_blk * 10;
    int last_temp_blk = first_temp_blk + last_blk - first_blk;

    // 第一阶段
    tpmms_p1(first_blk, last_blk, first_temp_blk);
    // 第二阶段
    tpmms_p2(first_temp_blk, last_temp_blk, first_output_blk);
}
