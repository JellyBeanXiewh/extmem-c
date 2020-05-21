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
    // ��ʼ�� Buffer
    if(!initBuffer(BUFFER_SIZE, BLOCK_SIZE, &buf)) {
        perror("Buffer Initialization Failed!\n");
        exit(-1);
    }

//    printf("��ѡ�������\n"
//           "1. �������������Ĺ�ϵѡ��\n"
//           "2. ���׶ζ�·�鲢���� (TPMMS)\n"
//           "3. ���������Ĺ�ϵѡ��\n"
//           "4. ��ϵͶӰ\n"
//           "5. ������������Ӳ���\n"
//           "6. ���������ɢ�е�����ɨ��\n");
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
//                printf("������������������\n");
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

int get_next_address(const unsigned char *blk) {
    char addr[5] = "";
    for (int i = 0; i < 4; i++) {
        addr[i] = (char)*(blk + 56 + i);
    }

    return atoi(addr);
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
        // ��ǰ��������д�����
        char next_addr[9] = "";     // ��һ�����̿�ĵ�ַ
        sprintf(next_addr, "%d", *addr + 1);
        memcpy(*blk + 56, next_addr, 8);

        // д�����
        if (writeBlockToDisk(*blk, *addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }

        (*addr)++;                          // ���´��̿��ַ
        *blk = getNewBlockInBuffer(&buf);   // �������� block
        *offset = 0;                        // �������λ��
    }

    write_tuple_to_block(*blk, *offset, t);
    (*offset)++;
}

void linear_search() {
    buf.numIO = 0;      // reset

    printf("----------------------------\n"
           "��������������ѡ���㷨 R.A=30:\n"
           "----------------------------\n");

    unsigned char *rblk;                // ���� R ��ϵ
    unsigned char *result_blk;          // ����������
    int start_output_addr = 101;        // ��ʼ������ݿ��ַ
    int cur_output_addr = 101;          // ��ǰ������ݿ��ַ
    int offset = 0;                     // ����ƫ����
    int cnt = 0;                        // ����������Ԫ����

    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    for (int addr = 1; addr <= 16; addr++) {
        if ((rblk = readBlockFromDisk(addr, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }

        printf("�������ݿ� %d\n", addr);

        // ��ȡ���ݿ��е�ÿ��Ԫ��
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

    // ������������δд�ش��̵�����
    if (offset != 0) {
        // д����һ��ĵ�ַ
        char next_addr[9] = "";
        sprintf(next_addr, "%d", cur_output_addr + 1);
        memcpy(result_blk + 56, next_addr, 8);

        if (writeBlockToDisk(result_blk, cur_output_addr, &buf) != 0) {
            perror("Writing Block Failed!\n");
            exit(-1);
        }
    }

    printf("ע�����д����̣�%d\n\n"
           "����ѡ��������Ԫ��һ�� %d ����\n\n"
           "IO��дһ�� %d �Ρ�\n", start_output_addr, cnt, (int)buf.numIO);
}

int cmp(const void *a, const void *b) {
    return ((Tuple*)a)->x - ((Tuple*)b)->x;
}

void tpmms_p1(int first_blk, int last_blk, int first_temp_blk) {
    int blk_num = last_blk - first_blk + 1;         // ���̿�����
    int set_num = (int)ceil((double) blk_num / 6);  // �Ӽ�������
    int cur_addr = first_blk;                       // ��ǰ���̿�
    int cur_temp_addr = first_temp_blk;             // ��ǰ����Ĵ��̿�

    // �����������Ԫ��
    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    for (int i = 0; i < set_num; i++) {
        unsigned char *blk[6];
        Tuple tuples[6 * 7];
        int tuple_cnt = 0;
        // ��ÿ���Ӽ���װ���ڴ�
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

        // TODO: �޸�����ʽ
        qsort(tuples, tuple_cnt, sizeof(Tuple), cmp);

        // �����м���
        int offset = 0;
        for (int j = 0; j < tuple_cnt; j++) {
            write_tuple_to_block_and_disk(&result_blk, &cur_temp_addr, &offset, tuples[j]);
        }

        // ������������δд�ش��̵�����
        if (offset != 0) {
            // д����һ��ĵ�ַ
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
    int blk_num = last_temp_blk - first_temp_blk + 1;   // ���̿�����
    int set_num = (int)ceil((double) blk_num / 6);  // �Ӽ�������

    int cur_output_addr = first_output_blk;             // ָ��ǰ����Ĵ��̿�
    int offset = 0;                                     // ָ����һ�����λ��

    // ��¼���
    unsigned char *result_blk;
    if ((result_blk = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    // ���ڱȽ�
    unsigned char *m_compare;
    if ((m_compare = getNewBlockInBuffer(&buf)) == NULL) {
        perror("Get new Block Failed!\n");
        exit(-1);
    }

    unsigned char *blk[set_num];    // ����ÿ�������׸�δ�������Ŀ�
    int tuple_ptr[set_num];         // ָ��ÿ������׸�δ�������Ԫ��
    int blk_sorting_ptr[set_num];   // ��¼��ǰ��������Ŀ�

    // ��ʼ��
    for (int i = 0; i < set_num; i++) {
        if ((blk[i] = readBlockFromDisk(first_temp_blk + i * 6, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            exit(-1);
        }
        Tuple t = get_tuple(blk[i], 0);             // ��ȡ���еĵ�һ��Ԫ��
        write_tuple_to_block(m_compare, i, t);      // ��Ԫ�������Ƚϼ���
        tuple_ptr[i] = 1;           // ָ����һ�����Ƚϵ�Ԫ��
        blk_sorting_ptr[i] = 0;     // ָ��ǰ��������Ŀ�
    }

    while (1) {
        int min = MAX_VALUE;        // ��Сֵ
        int set_index = 0;          // ��Сֵ����λ��
        int set_finish[6] = {0};    // ��¼�Ӽ��Ƿ����������

        // �ҵ���Сֵ����λ��
        for (int i = 0; i < set_num; i++) {
            Tuple t = get_tuple(m_compare, i);
            if (t.x == MAX_VALUE) {
                // ���Ӽ������������
                set_finish[i] = 1;
            }
            if (t.x < min) {
                min = t.x;
                set_index = i;
            }
        }

        int finish = 1;     // ��¼�����Ƿ�����ȫ���
        for (int i = 0; i < set_num; i++) {
            if (set_finish[i] == 0) {
                // �����Ӽ���δ�������
                finish = 0;
                break;
            }
        }

        // ���������
        if (finish) {
            // ������������δд�ش��̵�����
            if (offset != 0) {
                // д����һ��ĵ�ַ
                char next_addr[9] = "";
                sprintf(next_addr, "%d", cur_output_addr + 1);
                memcpy(result_blk + 56, next_addr, 8);

                if (writeBlockToDisk(result_blk, cur_output_addr, &buf) != 0) {
                    perror("Writing Block Failed!\n");
                    exit(-1);
                }
            }

            // �ͷ� block
            for (int i = 0; i < set_num; i++) {
                freeBlockInBuffer(blk[i], &buf);
            }

            // ɾ����ʱ�ļ�
            for (int i = first_temp_blk; i <= last_temp_blk; i++) {
                if (dropBlockOnDisk(i) != 0) {
                    perror("Dropping Block Fails!\n");
                    exit(-1);
                }
            }

            freeBlockInBuffer(m_compare, &buf);
            freeBlockInBuffer(result_blk, &buf);
            return;
        }

        // ����δ���
        // ��ȡ��Сֵ����Ԫ��
        Tuple t = get_tuple(m_compare, set_index);
        // ����Ԫ��д����������
        write_tuple_to_block_and_disk(&result_blk, &cur_output_addr, &offset, t);

        // ����һ��Ԫ��д����Ƚϼ���
        int blk_index = tuple_ptr[set_index] / 7;       // ��һ�����������Ԫ�����ڵĿ����Ӽ����е�λ��
        int tuple_offset = tuple_ptr[set_index] % 7;    // ��һ�����������Ԫ���ڿ��е�λ��

        if (blk_index == blk_sorting_ptr[set_index]) {
            // Ŀǰ��������Ŀ黹��δ�����Ԫ��
            Tuple temp = get_tuple(blk[set_index], tuple_offset);
            write_tuple_to_block(m_compare, set_index, temp);
            tuple_ptr[set_index]++;
        } else {
            // Ŀǰ��������Ŀ��е�Ԫ������ȫ����
            int next_addr = get_next_address(blk[set_index]);   // ��ȡ��һ��ĵ�ַ
            if (blk_index < 6 && next_addr <= last_temp_blk) {
                // ��ǰ�Ӽ�������δ����Ŀ�
                // �ͷ�������Ŀ�
                freeBlockInBuffer(blk[set_index], &buf);

                // װ��δ����Ŀ�
                if ((blk[set_index] = readBlockFromDisk(first_temp_blk + set_index * 6 + blk_index, &buf)) == NULL) {
                    perror("Reading Block Failed!\n");
                    exit(-1);
                }

                // ���µ�ǰ����������ָ��
                blk_sorting_ptr[set_index] = blk_index;

                // �����е�һ��Ԫ�������Ƚϼ���
                Tuple temp = get_tuple(blk[set_index], tuple_offset);
                write_tuple_to_block(m_compare, set_index, temp);

                // ָ����һ��Ԫ��
                tuple_ptr[set_index]++;
            } else {
                // ��ǰ�Ӽ�������ȫ����
                // ����Ƚϼ�����д���ض�Ԫ�飬��ʶ���Ӽ������������
                Tuple temp = {MAX_VALUE, MAX_VALUE};
                write_tuple_to_block(m_compare, set_index, temp);
            }
        }
    }
}

void two_phase_multiway_merge_sort(int first_blk, int last_blk, int first_output_blk) {
    buf.numIO = 0;      // reset

    int first_temp_blk = first_output_blk * 10;
    int last_temp_blk = first_temp_blk + last_blk - first_blk;

    // ��һ�׶�
    tpmms_p1(first_blk, last_blk, first_temp_blk);
    // �ڶ��׶�
    tpmms_p2(first_temp_blk, last_temp_blk, first_output_blk);
}
