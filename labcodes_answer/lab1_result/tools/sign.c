#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/*
*   �ù������ڼ�����ɵ�bootloader �ߴ��Ƿ񲻳���510B��
*   ����ĩβ��������ֽ�0x55AA���Ӷ���ʽ���ɺϷ���bootloader
*   bootloader �ļ���� 510B������С��510B
*   �ⲿ�ִ�������е���գ�������ɵ�bootloader ����510B������
*   ��������510��511 �����0x55AA ����Ŀǰ�ı�����Ϣ����ȷʵ��
*   ����510B�ģ�Ҳû��ʲô���⣬�������ﻹ��Ӧ������ϸ����
*   һ�¡�(??????)
*/

int
main(int argc, char *argv[]) {
    struct stat st;
    if (argc != 3) {
        fprintf(stderr, "Usage: <input filename> <output filename>\n");
        return -1;
    }
    /* stat() ���������ڻ�ȡ�ļ����� */
    if (stat(argv[1], &st) != 0) {
        fprintf(stderr, "Error opening file '%s': %s\n", argv[1], strerror(errno));
        return -1;
    }
    printf("'%s' size: %lld bytes\n", argv[1], (long long)st.st_size);
    if (st.st_size > 510) {
        fprintf(stderr, "%lld >> 510!!\n", (long long)st.st_size);
        return -1;
    }
    char buf[512];
    memset(buf, 0, sizeof(buf));
    FILE *ifp = fopen(argv[1], "rb");
    int size = fread(buf, 1, st.st_size, ifp);
    if (size != st.st_size) {
        fprintf(stderr, "read '%s' error, size is %d.\n", argv[1], size);
        return -1;
    }
    fclose(ifp);
    buf[510] = 0x55;
    buf[511] = 0xAA;
    FILE *ofp = fopen(argv[2], "wb+");
    size = fwrite(buf, 1, 512, ofp);
    if (size != 512) {
        fprintf(stderr, "write '%s' error, size is %d.\n", argv[2], size);
        return -1;
    }
    fclose(ofp);
    printf("build 512 bytes boot sector: '%s' success!\n", argv[2]);
    return 0;
}

