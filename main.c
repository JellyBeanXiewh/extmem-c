#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "extmem.h"
#define BUFFER_SIZE 520
#define BLOCK_SIZE 64
#define MAX_VALUE 9999

void linear_search(int first_blk, int last_blk, int first_output_blk, int X);
void two_phase_multiway_merge_sort(int first_blk, int last_blk, int first_output_blk, int *last_output_blk);
void create_index(int first_sorted_blk, int last_sorted_blk, int first_index_blk, int *last_index_blk);
void index_search(int first_index_blk, int last_index_blk, int first_output_blk, int X);
void projection(int first_sorted_blk, int last_sorted_blk, int first_output_blk);
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

    const int R_first_blk = 1;
    const int R_last_blk = 16;
    const int R_first_sorted_blk = 201;
    int R_last_sorted_blk;
    const int R_first_index_blk = 301;
    int R_last_index_blk;

    const int S_first_blk = 17;
    const int S_last_blk = 48;
    const int S_first_sorted_blk = 217;
    int S_last_sorted_blk;
    const int S_first_index_blk = 351;
    int S_last_index_blk;

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
//                index_search();
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

    const int R_A_value = 30;
    // 线性搜索 R.A = 30 的元组
    linear_search(R_first_blk, R_last_blk, 101, R_A_value);

    // 关系 R 排序
    two_phase_multiway_merge_sort(R_first_blk, R_last_blk, R_first_sorted_blk, &R_last_sorted_blk);
    // 关系 S 排序
    two_phase_multiway_merge_sort(S_first_blk, S_last_blk, S_first_sorted_blk, &S_last_sorted_blk);

    // 对关系 R 建立索引块
    create_index(R_first_sorted_blk, R_last_sorted_blk, R_first_index_blk, &R_last_index_blk);
    // 对关系 S 建立索引块
    create_index(S_first_sorted_blk, S_last_sorted_blk, S_first_index_blk, &S_last_index_blk);

    // 基于索引搜索 R.A = 30 的元组
    index_search(R_first_index_blk, R_last_index_blk, 121, R_A_value);

    // 对关系 R 上的 A 属性（非主码）进行投影并去重
    projection(R_first_sorted_blk, R_last_sorted_blk, 301);

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

int get_next_address(const unsigned char *blk) {
    char addr[5] = "";
    for (int i = 0; i < 4; i++) {
        addr[i] = (char)*(blk + 56 + i);
    }

    return atoi(addr);
}

void write_address_to_block(unsigned char *blk, int address) {
    char str_addr[9] = "";
    sprintf(str_addr, "%d", address);
    memcpy(blk + 56, str_addr, 8);
}

void write_attr_to_block(unsigned char *blk, int offset, int attr) {
    char str_attr[5] = "";
    sprintf(str_attr, "%d", attr);
    memcpy(blk + offset * 4, str_attr, 4);
}

void write_attr_to_block_and_disk(unsigned char **blk, int *addr, int *offset, int attr) {
    if (*offset >= 14) {
        // 当前块已满，写入磁盘
        write_address_to_block(*blk, *addr + 1);

        // 写入磁盘
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", *addr);

        (*addr)++;                          // 更新磁盘块地址
        *blk = getNewBlockInBuffer(&buf);   // 重新申请 block
        *offset = 0;                        // 更新输出位置
    }

    write_attr_to_block(*blk, *offset, attr);
    (*offset)++;
}

void write_tuple_to_block(unsigned char *blk, int offset, Tuple t) {
    char str_X[5] = "", str_Y[5] = "";
    sprintf(str_X, "%d", t.x);
    sprintf(str_Y, "%d", t.y);
    memcpy(blk + offset * 8, str_X, 4);
    memcpy(blk + offset * 8 + 4, str_Y, 4);
}

void write_tuple_to_block_and_disk(unsigned char **blk, int *addr, int *offset, Tuple t) {
    if (*offset >= 7) {
        // 当前块已满，写入磁盘
        write_address_to_block(*blk, *addr + 1);

        // 写入磁盘
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", *addr);

        (*addr)++;                          // 更新磁盘块地址
        *blk = getNewBlockInBuffer(&buf);   // 重新申请 block
        *offset = 0;                        // 更新输出位置
    }

    write_tuple_to_block(*blk, *offset, t);
    (*offset)++;
}

void linear_search(int first_blk, int last_blk, int first_output_blk, int X) {
    buf.numIO = 0;      // reset

    char rel[4];
    if (first_blk == 1) {
        strcpy(rel, "R.A");
    } else {
        strcpy(rel, "S.C");
    }
    printf("----------------------------\n"
           "基于线性搜索的选择算法 %s=%d:\n"
           "----------------------------\n", rel, X);

    unsigned char *result_blk;                  // 储存输出结果
    int cur_output_blk = first_output_blk;      // 当前输出数据块地址
    int offset = 0;                             // 块中偏移量
    int cnt = 0;                                // 满足条件的元组数

    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    for (int addr = first_blk; addr <= last_blk; addr++) {
        unsigned char *blk;     // 储存元组
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        // 获取数据块中的每个元组
        for (int i = 0; i < 7; i++) {
            Tuple t = get_tuple(blk, i);

            // R.A = 30
            if (t.x == X) {
                write_tuple_to_block_and_disk(&result_blk, &cur_output_blk, &offset, t);
                printf("(X=%d, Y=%d)\n", t.x, t.y);
                cnt++;
            }
        }
        freeBlockInBuffer(blk, &buf);
    }

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 写入下一块的地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        freeBlockInBuffer(result_blk, &buf);
    }

    printf("\n满足选择条件的元组一共 %d 个。\n\n"
           "IO读写一共 %d 次。\n", cnt, (int)buf.numIO);
}

int cmp(const void *a, const void *b) {
    return ((Tuple*)a)->x - ((Tuple*)b)->x;
}

void tpmms_p1(int first_blk, int last_blk, int first_temp_blk) {
    printf("----------------------------\n"
           "第一阶段\n"
           "----------------------------\n");

    int blk_num = last_blk - first_blk + 1;         // 磁盘块数量
    int set_num = (int)ceil((double) blk_num / 6);  // 子集合数量
    int cur_addr = first_blk;                       // 当前磁盘块
    int cur_temp_addr = first_temp_blk;             // 当前输出的磁盘块

    // 储存已排序的元组
    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    for (int i = 0; i < set_num; i++) {
        int blk_cnt = 0;
        unsigned char *blk[6];
        Tuple tuples[6 * 7];
        int tuple_cnt = 0;
        // 将每个子集合装入内存
        for (int j = 0; j < 6 && cur_addr <= last_blk; j++, cur_addr++, blk_cnt++) {
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

        // 保存中间结果
        int offset = 0;
        for (int j = 0; j < tuple_cnt; j++) {
            write_tuple_to_block_and_disk(&result_blk, &cur_temp_addr, &offset, tuples[j]);
        }

        // 缓冲区中仍有未写回磁盘的数据
        if (offset != 0) {
            // 写入下一块的地址
            write_address_to_block(result_blk, cur_temp_addr + 1);

            if (writeBlockToDisk(result_blk, cur_temp_addr, &buf) != 0) {
                perror("Writing Block Failed!\n");
                exit(-1);
            }

            printf("注：结果写入磁盘：%d\n", cur_temp_addr);

            result_blk = getNewBlockInBuffer(&buf);

            cur_temp_addr++;
        }

        for (int j = 0; j < blk_cnt; j++) {
            freeBlockInBuffer(blk[j], &buf);
        }

    }

    freeBlockInBuffer(result_blk, &buf);
}

void tpmms_p2(int first_temp_blk, int last_temp_blk, int first_output_blk, int *last_output_blk) {
    printf("----------------------------\n"
           "第二阶段\n"
           "----------------------------\n");

    int blk_num = last_temp_blk - first_temp_blk + 1;   // 磁盘块数量
    int set_num = (int)ceil((double) blk_num / 6);  // 子集合数量

    int cur_output_addr = first_output_blk;             // 指向当前输出的磁盘块
    int offset = 0;                                     // 指向下一个输出位置

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

    unsigned char *blk[set_num];    // 储存每个分组首个未完成排序的块
    int tuple_ptr[set_num];         // 指向每个块的首个未完成排序元组
    int blk_sorting_ptr[set_num];   // 记录当前正在排序的块

    // 初始化
    for (int i = 0; i < set_num; i++) {
        if ((blk[i] = readBlockFromDisk(first_temp_blk + i * 6, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }
        Tuple t = get_tuple(blk[i], 0);             // 读取块中的第一个元组
        write_tuple_to_block(m_compare, i, t);      // 将元组加入待比较集合
        tuple_ptr[i] = 1;           // 指向下一个待比较的元组
        blk_sorting_ptr[i] = 0;     // 指向当前正在排序的块
    }

    while (1) {
        int min = MAX_VALUE;        // 最小值
        int set_index = 0;          // 最小值所在位置
        int set_finish[6] = {0};    // 记录子集是否已排序完成

        // 找到最小值所在位置
        for (int i = 0; i < set_num; i++) {
            Tuple t = get_tuple(m_compare, i);
            if (t.x == MAX_VALUE) {
                // 该子集合已完成排序
                set_finish[i] = 1;
            }
            if (t.x < min) {
                min = t.x;
                set_index = i;
            }
        }

        int finish = 1;     // 记录排序是否已完全完成
        for (int i = 0; i < set_num; i++) {
            if (set_finish[i] == 0) {
                // 仍有子集合未完成排序
                finish = 0;
                break;
            }
        }

        // 排序已完成
        if (finish) {
            // 缓冲区中仍有未写回磁盘的数据
            if (offset != 0) {
                // 写入下一块的地址
                write_address_to_block(result_blk, cur_output_addr + 1);

                if (writeBlockToDisk(result_blk, cur_output_addr, &buf) != 0) {
                    perror("Writing Block Failed!\n");
                    exit(-1);
                }

                printf("注：结果写入磁盘：%d\n", cur_output_addr);
            } else {
                freeBlockInBuffer(result_blk, &buf);
            }

            *last_output_blk = cur_output_addr;

            freeBlockInBuffer(m_compare, &buf);

            // 释放 block
            for (int i = 0; i < set_num; i++) {
                freeBlockInBuffer(blk[i], &buf);
            }

            // 删除临时文件
            for (int i = first_temp_blk; i <= last_temp_blk; i++) {
                if (dropBlockOnDisk(i) != 0) {
                    perror("Dropping Block Fails!\n");
                    exit(-1);
                }
            }
            return;
        }

        // 排序未完成
        // 获取最小值所在元组
        Tuple t = get_tuple(m_compare, set_index);
        // 将该元组写入已排序结果
        write_tuple_to_block_and_disk(&result_blk, &cur_output_addr, &offset, t);

        // 将下一个元组写入待比较集合
        int blk_index = tuple_ptr[set_index] / 7;       // 下一个所需排序的元组所在的块在子集合中的位置
        int tuple_offset = tuple_ptr[set_index] % 7;    // 下一个所需排序的元组在块中的位置

        if (blk_index == blk_sorting_ptr[set_index]) {
            // 目前正在排序的块还有未排序的元组
            Tuple temp = get_tuple(blk[set_index], tuple_offset);
            write_tuple_to_block(m_compare, set_index, temp);
            tuple_ptr[set_index]++;
        } else {
            // 目前正在排序的块中的元组已完全排序
            int next_addr = get_next_address(blk[set_index]);   // 获取下一块的地址
            if (blk_index < 6 && next_addr <= last_temp_blk) {
                // 当前子集合仍有未排序的块
                // 释放已排序的块
                freeBlockInBuffer(blk[set_index], &buf);

                // 装载未排序的块
                if ((blk[set_index] = readBlockFromDisk(first_temp_blk + set_index * 6 + blk_index, &buf)) == NULL) {
                    perror("Reading Block Failed!\n");
                    exit(-1);
                }

                // 更新当前正在排序块的指针
                blk_sorting_ptr[set_index] = blk_index;

                // 将块中第一个元组加入待比较集合
                Tuple temp = get_tuple(blk[set_index], tuple_offset);
                write_tuple_to_block(m_compare, set_index, temp);

                // 指向下一个元组
                tuple_ptr[set_index]++;
            } else {
                // 当前子集合已完全排序
                // 向待比较集合中写入特定元组，标识该子集合已完成排序
                Tuple temp = {MAX_VALUE, MAX_VALUE};
                write_tuple_to_block(m_compare, set_index, temp);
            }
        }
    }
}

void two_phase_multiway_merge_sort(int first_blk, int last_blk, int first_output_blk, int *last_output_blk) {
    buf.numIO = 0;      // reset

    char rel[2];
    if (first_blk == 1) {
        strcpy(rel, "R");
    } else {
        strcpy(rel, "S");
    }
    printf("----------------------------\n"
           "两阶段多路归并排序关系 %s\n", rel);

    int first_temp_blk = first_output_blk * 10;
    int last_temp_blk = first_temp_blk + last_blk - first_blk;

    // 第一阶段
    tpmms_p1(first_blk, last_blk, first_temp_blk);
    // 第二阶段
    tpmms_p2(first_temp_blk, last_temp_blk, first_output_blk, last_output_blk);
}

void create_index(int first_sorted_blk, int last_sorted_blk, int first_index_blk, int *last_index_blk) {
    buf.numIO = 0;      // reset

    int cur_output_blk = first_index_blk;

    int offset = 0;

    unsigned char *index_blk;
    if ((index_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    for (int addr = first_sorted_blk; addr <= last_sorted_blk; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        Tuple index = get_tuple(blk, 0);
        index.y = addr;
        write_tuple_to_block_and_disk(&index_blk, &cur_output_blk, &offset, index);

        freeBlockInBuffer(blk, &buf);
    }

    *last_index_blk = cur_output_blk;

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 写入下一块的地址
        write_address_to_block(index_blk, cur_output_blk + 1);

        if (writeBlockToDisk(index_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        freeBlockInBuffer(index_blk, &buf);
    }
}

void index_search(int first_index_blk, int last_index_blk, int first_output_blk, int X) {
    buf.numIO = 0;      // reset

    char rel[4];
    if (first_index_blk == 301) {
        strcpy(rel, "R.A");
    } else {
        strcpy(rel, "S.C");
    }

    printf("----------------------------\n"
           "基于索引的选择算法 %s=%d:\n"
           "----------------------------\n", rel, X);

    int cur_output_blk = first_output_blk;
    int offset = 0;
    int cnt = 0;

    int index_find = 0;
    int blk_index = 0;
    for (int addr = first_index_blk; addr <= last_index_blk && !index_find; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入索引块 %d\n", addr);

        for (int i = 0; i < 7; i++) {
            Tuple index = get_tuple(blk, i);
            if (index.x >= X) {
                index_find = 1;
                break;
            }
            blk_index = index.y;
        }

        if (!index_find) {
            printf("没有满足条件的元组。\n");
        }

        freeBlockInBuffer(blk, &buf);
    }

    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    int finish_flag = 0;
    for (int addr = blk_index; addr <= last_index_blk && !finish_flag; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        for (int i = 0; i < 7; i++) {
            Tuple t = get_tuple(blk, i);
            if (t.x == X) {
                write_tuple_to_block_and_disk(&result_blk, &cur_output_blk, &offset, t);
                printf("(X=%d, Y=%d)\n", t.x, t.y);
                cnt++;
            } else if (t.x > X) {
                finish_flag = 1;
                break;
            }
        }

        freeBlockInBuffer(blk, &buf);
    }

    if (offset != 0) {
        // 写入下一块的地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        freeBlockInBuffer(result_blk, &buf);
    }

    printf("\n满足选择条件的元组一共 %d 个。\n\n"
           "IO读写一共 %d 次。\n", cnt, (int)buf.numIO);
}

void projection(int first_sorted_blk, int last_sorted_blk, int first_output_blk) {
    buf.numIO = 0;      // reset

    printf("----------------------------\n"
           "基于排序的投影算法（并去重）"
           "----------------------------\n");

    int cur_output_blk = first_output_blk;
    int offset = 0;
    int attr_cnt = 0;

    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    int last_attr = MAX_VALUE;
    for (int addr = first_sorted_blk; addr <= last_sorted_blk; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        for (int i = 0; i < 7; i++) {
            Tuple t = get_tuple(blk, i);
            if (last_attr != t.x) {
                last_attr = t.x;
                write_attr_to_block_and_disk(&result_blk, &cur_output_blk, &offset, last_attr);
                attr_cnt++;
                printf("(X=%d)\n", last_attr);
            }
        }

        freeBlockInBuffer(blk, &buf);
    }

    if (offset != 0) {
        // 当前块已满，写入磁盘
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        freeBlockInBuffer(result_blk, &buf);
    }

    printf("\n关系 R 上的 A 属性满足投影（去重）的属性值一共 %d 个。\n", attr_cnt);
}
