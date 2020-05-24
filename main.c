#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "extmem.h"

#define BUFFER_SIZE 520
#define BLOCK_SIZE 64
#define MAX_VALUE 9999

#define ATTR_NUM_PER_TUPLE 2
#define TUPLE_NUM_PER_BLOCK 7
#define BLOCK_NUM_PER_SET 6
#define JOIN_TUPLE_NUM_PER_BLOCK 3

#define ATTR_SIZE 4
#define TUPLE_SIZE (ATTR_SIZE * ATTR_NUM_PER_TUPLE)
#define JOIN_TUPLE_SIZE (TUPLE_SIZE * 2)

typedef struct tuple {
    int x;
    int y;
} Tuple;

Buffer buf;

/**
 * @brief 在无序数据块中线性搜索具有指定属性的元组
 * @param first_blk         第一个数据块的编号
 * @param last_blk          最后一个数据块的编号
 * @param first_output_blk  第一个输出数据块的编号
 * @param X                 要查找的属性
 */
void linear_search(int first_blk, int last_blk, int first_output_blk, int X);

/**
 * @brief 两阶段多路归并
 * @param first_blk         第一个数据块的编号
 * @param last_blk          最后一个数据块的编号
 * @param first_output_blk  第一个输出数据块的编号
 * @param last_output_blk   记录最后一个输出数据块编号的变量的地址
 */
void two_phase_multiway_merge_sort(int first_blk, int last_blk, int first_output_blk, int *last_output_blk);

/**
 * @brief 对已排序数据块创建索引
 * @param first_sorted_blk  第一个已排序数据块的编号
 * @param last_sorted_blk   最后一个已排序数据块的编号
 * @param first_index_blk   第一个索引块的编号
 * @param last_index_blk    记录最后一个索引块编号的变量的地址
 */
void create_index(int first_sorted_blk, int last_sorted_blk, int first_index_blk, int *last_index_blk);

/**
 * @brief 通过索引块在已排序数据块中搜索具有指定属性的元组
 * @param first_index_blk   第一个索引块的编号
 * @param last_index_blk    最后一个索引块的编号
 * @param first_output_blk  第一个输出数据块的编号
 * @param X                 要查找的属性
 */
void index_search(int first_index_blk, int last_index_blk, int first_output_blk, int X);

/**
 * @brief 对关系的第一个属性进行投影并去重
 * @param first_sorted_blk  第一个已排序数据块的编号
 * @param last_sorted_blk   最后一个已排序数据块的编号
 * @param first_output_blk  第一个输出数据块的编号
 */
void projection(int first_sorted_blk, int last_sorted_blk, int first_output_blk);

/**
 * @brief 基于排序结果，计算 R JOIN S ON R.A = S.C
 * @param R_first_sorted_blk    R 关系的第一个已排序数据块的编号
 * @param R_last_sorted_blk     R 关系的最后一个已排序数据块的编号
 * @param S_first_sorted_blk    S 关系的第一个已排序数据块的编号
 * @param S_last_sorted_blk     S 关系的最后一个已排序数据块的编号
 * @param first_output_blk      第一个输出数据块的编号
 */
void sort_merge_join(int R_first_sorted_blk, int R_last_sorted_blk, int S_first_sorted_blk, int S_last_sorted_blk,
                     int first_output_blk);

/**
 * @brief 基于排序结果，计算 R 与 S 的交集
 * @param R_first_sorted_blk    R 关系的第一个已排序数据块的编号
 * @param R_last_sorted_blk     R 关系的最后一个已排序数据块的编号
 * @param S_first_sorted_blk    S 关系的第一个已排序数据块的编号
 * @param S_last_sorted_blk     S 关系的最后一个已排序数据块的编号
 * @param first_output_blk      第一个输出数据块的编号
 */
void
sort_merge_intersection(int R_first_sorted_blk, int R_last_sorted_blk, int S_first_sorted_blk, int S_last_sorted_blk,
                        int first_output_blk);

int main() {
    // 初始化 Buffer
    if (!initBuffer(BUFFER_SIZE, BLOCK_SIZE, &buf)) {
        perror("Buffer Initialization Failed!\n");
        exit(-1);
    }

    const int R_first_blk = 1;          // R 的第一个数据块的编号
    const int R_last_blk = 16;          // R 的最后一个数据块的编号
    const int R_first_sorted_blk = 201; // R 的第一个已排序数据块的编号
    int R_last_sorted_blk;              // R 的最后一个已排序数据块的编号
    const int R_first_index_blk = 301;  // R 的第一个索引块的编号
    int R_last_index_blk;               // R 的最后一个索引块的编号

    const int S_first_blk = 17;         // S 的第一个数据块的编号
    const int S_last_blk = 48;          // S 的最后一个数据块的编号
    const int S_first_sorted_blk = 217; // S 的第一个已排序数据块的编号
    int S_last_sorted_blk;              // S 的最后一个已排序数据块的编号
    const int S_first_index_blk = 351;  // S 的第一个索引块的编号
    int S_last_index_blk;               // S 的最后一个索引块的编号

    const int R_A_value = 30;           // 所要搜索的 R.A 的值
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
    projection(R_first_sorted_blk, R_last_sorted_blk, 401);

    // 基于排序的连接操作
    sort_merge_join(R_first_sorted_blk, R_last_sorted_blk, S_first_sorted_blk, S_last_sorted_blk, 501);

    // 基于排序的交操作
    sort_merge_intersection(R_first_sorted_blk, R_last_sorted_blk, S_first_sorted_blk, S_last_sorted_blk, 601);

    // 释放内存
    freeBuffer(&buf);
    return 0;
}

/**
 * @brief 从已装载至内存的块中读取指定元组
 * @param blk       数据块的地址
 * @param offset    偏移量，有效值从 0 到 6，分别对应块中的 7 个元组
 * @return          元组
 */
Tuple get_tuple(const unsigned char *blk, int offset) {
    Tuple t;
    // 属性 X
    char str[ATTR_SIZE + 1] = "";
    for (int i = 0; i < ATTR_SIZE; i++) {
        str[i] = (char) *(blk + offset * TUPLE_SIZE + i);
    }
    t.x = atoi(str);
    
    // 属性 Y
    for (int i = 0; i < ATTR_SIZE; i++) {
        str[i] = (char) *(blk + offset * TUPLE_SIZE + ATTR_SIZE + i);
    }
    t.y = atoi(str);

    return t;
}

/**
 * @brief 从块中读取下一个块的编号
 * @param blk       数据块的地址
 * @return          下一个数据块的编号
 */
int get_next_address(const unsigned char *blk) {
    char addr[ATTR_SIZE + 1] = "";
    for (int i = 0; i < ATTR_SIZE; i++) {
        addr[i] = (char) *(blk + TUPLE_SIZE * TUPLE_NUM_PER_BLOCK + i);
    }

    return atoi(addr);
}

/**
 * @brief 将指定块的下一块编号写为指定编号
 * @param blk       数据块的地址
 * @param address   指定数据块的编号
 */
void write_address_to_block(unsigned char *blk, int address) {
    char str_addr[TUPLE_SIZE + 1] = "";
    sprintf(str_addr, "%d", address);
    memcpy(blk + TUPLE_SIZE * TUPLE_NUM_PER_BLOCK, str_addr, TUPLE_SIZE);
}

/**
 * @brief 将属性值写入块中指定位置
 * @param blk       数据块的地址
 * @param offset    偏移量，有效值从 0 到 13，共 14 个属性
 * @param attr      属性值
 */
void write_attr_to_block(unsigned char *blk, int offset, int attr) {
    char str_attr[ATTR_SIZE + 1] = "";
    sprintf(str_attr, "%d", attr);
    memcpy(blk + offset * ATTR_SIZE, str_attr, ATTR_SIZE);
}

/**
 * @brief 将属性值写入块中指定位置，并在块写满时自动写入磁盘
 * @param blk       记录数据块地址的变量的地址
 * @param addr      记录当前输出数据块编号的变量的地址
 * @param offset    记录偏移量的变量的地址，偏移量有效值从 0 到 13，共 14 个属性
 * @param attr      属性值
 */
void write_attr_to_block_and_disk(unsigned char **blk, int *addr, int *offset, int attr) {
    if (*offset >= ATTR_NUM_PER_TUPLE * TUPLE_NUM_PER_BLOCK) {
        // 当前块已满，写入磁盘
        write_address_to_block(*blk, *addr + 1);

        // 写入磁盘
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", *addr);

        (*addr)++;                          // 更新磁盘块编号
        *blk = getNewBlockInBuffer(&buf);   // 重新申请 block
        *offset = 0;                        // 重置输出位置
    }

    write_attr_to_block(*blk, *offset, attr);
    (*offset)++;
}

/**
 * @brief 将元组写入块中指定位置
 * @param blk       数据块的地址
 * @param offset    从 0 到 6，共 7 个元组
 * @param t         元组
 */
void write_tuple_to_block(unsigned char *blk, int offset, Tuple t) {
    write_attr_to_block(blk, offset * 2, t.x);
    write_attr_to_block(blk, offset * 2 + 1, t.y);
}

/**
 * @brief 将元组写入块中指定位置，并在块写满时自动写入磁盘
 * @param blk       记录数据块地址的变量的地址
 * @param addr      记录当前输出数据块编号的变量的地址
 * @param offset    记录偏移量的变量的地址，偏移量有效值从 0 到 6，共 7 个元组
 * @param t         元组
 */
void write_tuple_to_block_and_disk(unsigned char **blk, int *addr, int *offset, Tuple t) {
    if (*offset >= TUPLE_NUM_PER_BLOCK) {
        // 当前块已满，写入磁盘
        write_address_to_block(*blk, *addr + 1);

        // 写入磁盘
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", *addr);

        (*addr)++;                          // 更新磁盘块编号
        *blk = getNewBlockInBuffer(&buf);   // 重新申请 block
        *offset = 0;                        // 重置输出位置
    }

    write_tuple_to_block(*blk, *offset, t);
    (*offset)++;
}

/**
 * @brief 将连接后的元组写入块中指定位置
 * @param blk       数据块的地址
 * @param offset    偏移量，有效值从 0 到 2，共 3 个连接后的元组
 * @param t1        连接前的元组 1
 * @param t2        连接前的元组 2
 */
void write_join_tuple_to_block(unsigned char *blk, int offset, Tuple t1, Tuple t2) {
    write_tuple_to_block(blk, offset * 2, t1);
    write_tuple_to_block(blk, offset * 2 + 1, t2);
}

/**
 * @brief 将连接后的元组写入块中指定位置，并在块写满时自动写入磁盘
 * @param blk       记录数据块地址的变量的地址
 * @param addr      记录当前输出数据块编号的变量的地址
 * @param offset    记录偏移量的变量的地址，偏移量有效值从 0 到 2，共 3 个连接后的元组
 * @param t1        连接前的元组 1
 * @param t2        连接前的元组 2
 */
void write_join_tuple_to_block_and_disk(unsigned char **blk, int *addr, int *offset, Tuple t1, Tuple t2) {
    if (*offset >= JOIN_TUPLE_NUM_PER_BLOCK) {
        // 当前块已满，写入磁盘
        write_address_to_block(*blk, *addr + 1);

        // 写入磁盘
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", *addr);

        (*addr)++;                          // 更新磁盘块编号
        *blk = getNewBlockInBuffer(&buf);   // 重新申请 block
        memset(*blk, 0, BLOCK_SIZE);        // 清空
        *offset = 0;                        // 重置输出位置
    }

    write_join_tuple_to_block(*blk, *offset, t1, t2);
    (*offset)++;
}

void linear_search(int first_blk, int last_blk, int first_output_blk, int X) {
    size_t old_numIO = buf.numIO;

    char rel[4];
    if (first_blk == 1) {
        strcpy(rel, "R.A");
    } else {
        strcpy(rel, "S.C");
    }
    printf("-------------------------------\n"
           "基于线性搜索的选择算法 %s=%d:\n"
           "-------------------------------\n", rel, X);

    int cur_output_blk = first_output_blk;      // 当前输出数据块地址
    int offset = 0;                             // 输出块中偏移量
    int cnt = 0;                                // 满足条件的元组数

    unsigned char *result_blk;                  // 储存输出结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    // 逐个搜索
    for (int addr = first_blk; addr <= last_blk; addr++) {
        unsigned char *blk;     // 储存元组
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        // 获取数据块中的每个元组
        for (int i = 0; i < TUPLE_NUM_PER_BLOCK; i++) {
            Tuple t = get_tuple(blk, i);

            // 满足条件
            if (t.x == X) {
                write_tuple_to_block_and_disk(&result_blk, &cur_output_blk, &offset, t);
                printf("(X=%d, Y=%d)\n", t.x, t.y);
                cnt++;
            }
        }
        freeBlockInBuffer(blk, &buf);
    }

    if (offset != 0) {
        // 缓冲区中仍有未写回磁盘的数据
        // 写入下一块的地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(result_blk, &buf);
    }

    printf("\n满足选择条件的元组一共 %d 个。\n\n"
           "IO读写一共 %lld 次。\n", cnt, buf.numIO - old_numIO);
}

/**
 * @brief 两阶段多路归并排序的第一阶段
 * @param first_blk         第一个数据块的编号
 * @param last_blk          最后一个数据块的编号
 * @param first_temp_blk    第一个储存中间结果的数据块的编号
 */
void tpmms_p1(int first_blk, int last_blk, int first_temp_blk) {
    printf("-------------------------------\n"
           "第一阶段\n"
           "-------------------------------\n");

    int blk_num = last_blk - first_blk + 1;         // 磁盘块数量
    int set_num = (int) ceil((double) blk_num / BLOCK_NUM_PER_SET);  // 子集合数量
    int cur_addr = first_blk;                       // 当前磁盘块
    int cur_temp_addr = first_temp_blk;             // 当前输出的磁盘块

    // 储存已排序的元组
    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    // 按组遍历
    for (int i = 0; i < set_num; i++) {
        int blk_cnt = 0;    // 记录内存中的块数量
        unsigned char *blk[BLOCK_NUM_PER_SET];
        // 将每个子集合装入内存
        for (; blk_cnt < BLOCK_NUM_PER_SET && cur_addr <= last_blk; cur_addr++, blk_cnt++) {
            if ((blk[blk_cnt] = readBlockFromDisk(cur_addr, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
        }

        // 交换排序
        for (int j = 0; j < blk_cnt * TUPLE_NUM_PER_BLOCK - 1; j++) {
            int tuple_j_index = j / 7;
            int tuple_j_offset = j % 7;
            for (int k = j + 1; k < blk_cnt * TUPLE_NUM_PER_BLOCK; k++) {
                int tuple_k_index = k / 7;
                int tuple_k_offset = k % 7;
                Tuple t1 = get_tuple(blk[tuple_j_index], tuple_j_offset);
                Tuple t2 = get_tuple(blk[tuple_k_index], tuple_k_offset);
                if (t1.x > t2.x) {
                    write_tuple_to_block(blk[tuple_j_index], tuple_j_offset, t2);
                    write_tuple_to_block(blk[tuple_k_index], tuple_k_offset, t1);
                }
            }

        }

        // 保存中间结果
        int offset = 0;
        for (int j = 0; j < blk_cnt; j++) {
            for (int k = 0; k < TUPLE_NUM_PER_BLOCK; k++) {
                Tuple t = get_tuple(blk[j], k);
                write_tuple_to_block_and_disk(&result_blk, &cur_temp_addr, &offset, t);
            }
        }

        // 缓冲区中仍有未写回磁盘的数据
        if (offset != 0) {
            // 写入下一块的地址
            write_address_to_block(result_blk, cur_temp_addr + 1);

            // 写入磁盘
            if (writeBlockToDisk(result_blk, cur_temp_addr, &buf) != 0) {
                perror("Writing Block Failed!\n");
                exit(-1);
            }

            printf("注：结果写入磁盘：%d\n", cur_temp_addr);

            // 申请新块
            result_blk = getNewBlockInBuffer(&buf);

            cur_temp_addr++;    // 更新输出块编号
        }

        // 释放
        for (int j = 0; j < blk_cnt; j++) {
            freeBlockInBuffer(blk[j], &buf);
        }

    }

    // 释放
    freeBlockInBuffer(result_blk, &buf);
}

/**
 * @brief 两阶段多路归并排序的第二阶段
 * @param first_temp_blk    第一个储存中间结果的数据块的编号
 * @param last_temp_blk     最后一个储存中间结果的数据块的编号
 * @param first_output_blk  第一个输出数据块的编号
 * @param last_output_blk   记录最后一个输出数据块编号的变量的地址
 */
void tpmms_p2(int first_temp_blk, int last_temp_blk, int first_output_blk, int *last_output_blk) {
    printf("-------------------------------\n"
           "第二阶段\n"
           "-------------------------------\n");

    int blk_num = last_temp_blk - first_temp_blk + 1;   // 磁盘块数量
    int set_num = (int) ceil((double) blk_num / BLOCK_NUM_PER_SET);  // 子集合数量

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
        if ((blk[i] = readBlockFromDisk(first_temp_blk + i * BLOCK_NUM_PER_SET, &buf)) == NULL) {
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
        bool set_finish[set_num];   // 记录子集是否已排序完成

        // 搜索最小值所在位置
        for (int i = 0; i < set_num; i++) {
            Tuple t = get_tuple(m_compare, i);
            if (t.x == MAX_VALUE) {
                // 该子集合已完成排序
                set_finish[i] = true;
            } else {
                set_finish[i] = false;
            }
            if (t.x < min) {
                min = t.x;
                set_index = i;
            }
        }

        bool finish = true;     // 记录排序是否已完全完成
        for (int i = 0; i < set_num; i++) {
            if (set_finish[i] == 0) {
                // 仍有子集合未完成排序
                finish = false;
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
                // 释放
                freeBlockInBuffer(result_blk, &buf);
            }

            // 记录最后一块的编号
            *last_output_blk = cur_output_addr;

            // 释放 block
            freeBlockInBuffer(m_compare, &buf);
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
    char rel[2];
    if (first_blk == 1) {
        strcpy(rel, "R");
    } else {
        strcpy(rel, "S");
    }
    printf("-------------------------------\n"
           "两阶段多路归并排序关系 %s\n", rel);

    int first_temp_blk = first_output_blk * 10;                 // 第一个临时块的编号
    int last_temp_blk = first_temp_blk + last_blk - first_blk;  // 最后一个临时块的编号

    // 第一阶段
    tpmms_p1(first_blk, last_blk, first_temp_blk);
    // 第二阶段
    tpmms_p2(first_temp_blk, last_temp_blk, first_output_blk, last_output_blk);
}

void create_index(int first_sorted_blk, int last_sorted_blk, int first_index_blk, int *last_index_blk) {
    char rel[2];
    if (first_sorted_blk == 201) {
        strcpy(rel, "R");
    } else {
        strcpy(rel, "S");
    }

    printf("-------------------------------\n"
           "对关系 %s 创建索引\n"
           "-------------------------------\n", rel);

    int cur_output_blk = first_index_blk;   // 当前输出块的编号
    int offset = 0;                         // 输出块中偏移量

    // 输出用的索引块
    unsigned char *index_blk;
    if ((index_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    // 逐个遍历
    for (int addr = first_sorted_blk; addr <= last_sorted_blk; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        // 记录每个块第一个元组的第一个属性
        Tuple index = get_tuple(blk, 0);
        index.y = addr;
        write_tuple_to_block_and_disk(&index_blk, &cur_output_blk, &offset, index);

        freeBlockInBuffer(blk, &buf);
    }

    *last_index_blk = cur_output_blk;   // 记录最后一个索引块的编号

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 写入下一块的地址
        write_address_to_block(index_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(index_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(index_blk, &buf);
    }
}

void index_search(int first_index_blk, int last_index_blk, int first_output_blk, int X) {
    size_t old_numIO = buf.numIO;

    char rel[4];
    if (first_index_blk == 301) {
        strcpy(rel, "R.A");
    } else {
        strcpy(rel, "S.C");
    }

    printf("-------------------------------\n"
           "基于索引的选择算法 %s=%d:\n"
           "-------------------------------\n", rel, X);

    int cur_output_blk = first_output_blk;  // 当前输出块的编号
    int offset = 0;                         // 输出块中的偏移量
    int cnt = 0;                            // 满足条件的元组数

    bool index_find = false;    // 记录是否找到所需的索引
    int blk_index = 0;          // 所需读取的首个数据块
    // 遍历索引块
    for (int addr = first_index_blk; addr <= last_index_blk && !index_find; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入索引块 %d\n", addr);

        // 遍历块中每个索引
        for (int i = 0; i < 7; i++) {
            Tuple index = get_tuple(blk, i);
            if (index.x >= X) {
                // 不小于所需值，需要从前一个索引开始搜索
                index_find = true;
                break;
            }
            blk_index = index.y;
        }

        if (!index_find) {
            printf("没有满足条件的元组。\n");
            return;
        }

        // 释放
        freeBlockInBuffer(blk, &buf);
    }

    unsigned char *result_blk;      // 储存输出结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    bool finish_flag = false;       // 记录是否搜索完毕
    // 从指定位置开始遍历数据块
    for (int addr = blk_index; addr <= last_index_blk && !finish_flag; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        // 遍历块中每个元组
        for (int i = 0; i < 7; i++) {
            Tuple t = get_tuple(blk, i);
            if (t.x == X) {
                // 符合条件
                write_tuple_to_block_and_disk(&result_blk, &cur_output_blk, &offset, t);
                printf("(X=%d, Y=%d)\n", t.x, t.y);
                cnt++;
            } else if (t.x > X) {
                // 搜索完毕
                finish_flag = true;
                break;
            }
        }

        // 释放
        freeBlockInBuffer(blk, &buf);
    }

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 写入下一块的地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(result_blk, &buf);
    }

    printf("\n满足选择条件的元组一共 %d 个。\n\n"
           "IO读写一共 %lld 次。\n", cnt, buf.numIO - old_numIO);
}

void projection(int first_sorted_blk, int last_sorted_blk, int first_output_blk) {
    printf("-------------------------------\n"
           "基于排序的投影算法（并去重）\n"
           "-------------------------------\n");

    int cur_output_blk = first_output_blk;  // 当前输出块的编号
    int offset = 0;                         // 输出块中的偏移量
    int attr_cnt = 0;                       // 不同的属性的数量

    unsigned char *result_blk;              // 记录结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    int last_attr = MAX_VALUE;              // 上一个符合条件的属性
    // 遍历所有数据块
    for (int addr = first_sorted_blk; addr <= last_sorted_blk; addr++) {
        unsigned char *blk;
        if ((blk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("读入数据块 %d\n", addr);

        // 遍历块中元组
        for (int i = 0; i < 7; i++) {
            Tuple t = get_tuple(blk, i);
            if (last_attr != t.x) {
                // 新属性
                last_attr = t.x;
                write_attr_to_block_and_disk(&result_blk, &cur_output_blk, &offset, last_attr);
                attr_cnt++;
                printf("(X=%d)\n", last_attr);
            }
        }

        // 释放
        freeBlockInBuffer(blk, &buf);
    }

    // 缓冲区中仍有未写回磁盘的数据
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

void sort_merge_join(int R_first_sorted_blk, int R_last_sorted_blk, int S_first_sorted_blk, int S_last_sorted_blk,
                     int first_output_blk) {
    printf("-------------------------------\n"
           "基于排序的连接算法\n"
           "-------------------------------\n");

    int join_cnt = 0;                       // 连接次数

    int r_cur_blk = R_first_sorted_blk;     // 当前读取的 R 关系的块
    int s_cur_blk = S_first_sorted_blk;     // 当前读取的 S 关系的块

    int cur_output_blk = first_output_blk;  // 当前输出块的编号
    int offset = 0;                         // 输出块中的偏移量

    // 初始化
    unsigned char *result_blk;              // 记录输出结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    memset(result_blk, 0, BLOCK_SIZE);        // 清空

    unsigned char *r_blk;                   // R
    if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    unsigned char *s_blk;                   // S
    if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    int r_blk_offset = 0;                   // 记录当前正在读取的 R 的元组在块中的偏移量
    int s_blk_offset = 0;                   // 记录当前正在读取的 S 的元组在块中的偏移量

    while (r_cur_blk <= R_last_sorted_blk && s_cur_blk <= S_last_sorted_blk) {
        Tuple t_r = get_tuple(r_blk, r_blk_offset);
        Tuple t_s = get_tuple(s_blk, s_blk_offset);

        if (t_r.x < t_s.x) {
            r_blk_offset++;
        } else if (t_r.x > t_s.x) {
            s_blk_offset++;
        } else {
            // t_r.x == t_s.x
            // 遍历从此处开始的 R 关系的元组，直到 t_r.x > t_s.x
            int r_temp_blk = r_cur_blk;
            int r_temp_offset = r_blk_offset;

            unsigned char *r_temp;
            if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }

            // 从当前位置开始遍历，记录所有连接后的元组，直到 t_r.x > t_s.x 或遍历完所有块
            while (r_temp_blk <= R_last_sorted_blk) {
                Tuple t_r_temp = get_tuple(r_temp, r_temp_offset);

                if (t_r_temp.x > t_s.x) {
                    freeBlockInBuffer(r_temp, &buf);
                    break;
                }

                write_join_tuple_to_block_and_disk(&result_blk, &cur_output_blk, &offset, t_r_temp, t_s);
                join_cnt++;

                r_temp_offset++;    // 下一个元组
                if (r_temp_offset >= 7) {
                    // 当前块已读完
                    // 释放块
                    freeBlockInBuffer(r_temp, &buf);
                    r_temp_blk++;   // 下一块
                    if (r_temp_blk > R_last_sorted_blk) {
                        // 遍历完
                        break;
                    }
                    if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                        perror("Reading Block Failed!\n");
                        exit(-1);
                    }
                    r_temp_offset = 0;  // 重置偏移量
                }
            }

            s_blk_offset++;     // 下一个元组
        }

        if (r_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(r_blk, &buf);
            r_cur_blk++;
            if (r_cur_blk > R_last_sorted_blk) {
                // 遍历完
                break;
            }
            if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            r_blk_offset = 0;
        }

        if (s_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(s_blk, &buf);
            s_cur_blk++;
            if (s_cur_blk > S_last_sorted_blk) {
                // 遍历完
                break;
            }
            if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            s_blk_offset = 0;
        }
    }

    // 缓冲区中仍有未写回磁盘的数据
    if (offset != 0) {
        // 当前块已满，写入磁盘
        // 写地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(result_blk, &buf);
    }

    // 释放
    freeBlockInBuffer(r_blk, &buf);
    freeBlockInBuffer(s_blk, &buf);

    printf("\n总共连接 %d 次。\n", join_cnt);
}

void
sort_merge_intersection(int R_first_sorted_blk, int R_last_sorted_blk, int S_first_sorted_blk, int S_last_sorted_blk,
                        int first_output_blk) {
    printf("-------------------------------\n"
           "基于排序的交算法\n"
           "-------------------------------\n");

    int cnt = 0;                            // 符合条件的元组数

    int r_cur_blk = R_first_sorted_blk;     // 当前读取的 R 关系的块
    int s_cur_blk = S_first_sorted_blk;     // 当前读取的 S 关系的块

    int cur_output_blk = first_output_blk;  // 当前输出块的编号
    int offset = 0;                         // 输出块中的偏移量

    // 初始化
    unsigned char *result_blk;              // 记录输出结果
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    memset(result_blk, 0, BLOCK_SIZE);        // 清空

    unsigned char *r_blk;                   // R
    if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    unsigned char *s_blk;                   // S
    if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }

    int r_blk_offset = 0;                   // 记录当前正在读取的 R 的元组在块中的偏移量
    int s_blk_offset = 0;                   // 记录当前正在读取的 S 的元组在块中的偏移量

    while (r_cur_blk <= R_last_sorted_blk && s_cur_blk <= S_last_sorted_blk) {
        Tuple t_r = get_tuple(r_blk, r_blk_offset);
        Tuple t_s = get_tuple(s_blk, s_blk_offset);

        if (t_r.x < t_s.x) {
            r_blk_offset++;
        } else if (t_r.x > t_s.x) {
            s_blk_offset++;
        } else {
            // t_r.x == t_s.x
            // 遍历从此处开始的 R 关系的元组，直到 t_r.x > t_s.x
            int r_temp_blk = r_cur_blk;
            int r_temp_offset = r_blk_offset;

            unsigned char *r_temp;
            if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }

            // 从当前位置开始遍历，记录所有相同的元组，直到 t_r.x > t_s.x 或遍历完所有块
            while (r_temp_blk <= R_last_sorted_blk) {
                Tuple t_r_temp = get_tuple(r_temp, r_temp_offset);

                if (t_r_temp.x > t_s.x) {
                    freeBlockInBuffer(r_temp, &buf);
                    break;
                }

                if (t_r_temp.y == t_s.y) {
                    // 元组相同
                    write_tuple_to_block_and_disk(&result_blk, &cur_output_blk, &offset, t_s);
                    printf("(X=%d, Y=%d)\n", t_s.x, t_s.y);
                    cnt++;
                }

                r_temp_offset++;    // 下一个元组
                if (r_temp_offset >= 7) {
                    // 当前块已读完
                    // 释放块
                    freeBlockInBuffer(r_temp, &buf);
                    r_temp_blk++;   // 下一块
                    if (r_temp_blk > R_last_sorted_blk) {
                        // 遍历完
                        break;
                    }
                    if ((r_temp = readBlockFromDisk(r_temp_blk, &buf)) == NULL) {
                        perror("Reading Block Failed!\n");
                        exit(-1);
                    }
                    r_temp_offset = 0;  // 重置
                }
            }

            s_blk_offset++;     // 下一个元组
        }

        if (r_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(r_blk, &buf);
            r_cur_blk++;    // 下一块
            if (r_cur_blk > R_last_sorted_blk) {
                // 遍历完
                break;
            }
            if ((r_blk = readBlockFromDisk(r_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            r_blk_offset = 0;
        }

        if (s_blk_offset >= 7) {
            // 当前块已读完
            freeBlockInBuffer(s_blk, &buf);
            s_cur_blk++;
            if (s_cur_blk > S_last_sorted_blk) {
                // 遍历完
                break;
            }
            if ((s_blk = readBlockFromDisk(s_cur_blk, &buf)) == NULL) {
                perror("Reading Block Failed!\n");
                exit(-1);
            }
            s_blk_offset = 0;
        }
    }

    if (offset != 0) {
        // 当前块已满，写入磁盘
        // 写地址
        write_address_to_block(result_blk, cur_output_blk + 1);

        // 写入磁盘
        if (writeBlockToDisk(result_blk, cur_output_blk, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        printf("注：结果写入磁盘：%d\n", cur_output_blk);
    } else {
        // 释放
        freeBlockInBuffer(result_blk, &buf);
    }

    // 释放
    freeBlockInBuffer(r_blk, &buf);
    freeBlockInBuffer(s_blk, &buf);

    printf("\nS 和 R 的交集有 %d 个元组。\n", cnt);
}
