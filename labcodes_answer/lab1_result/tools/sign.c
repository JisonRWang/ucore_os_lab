#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/*
*   该工具用于检查生成的bootloader 尺寸是否不超过510B，
*   并在末尾填充两个字节0x55AA，从而正式生成合法的bootloader
*   bootloader 文件最大 510B，允许小于510B
*   这部分代码可能有点风险，如果生成的bootloader 少于510B，这里
*   仍是往第510，511 处添加0x55AA 。从目前的编译信息看，确实是
*   少于510B的，也没有什么问题，所以这里还是应该再仔细分析
*   一下。(??????)
*/

int
main(int argc, char *argv[]) {
    struct stat st;
    if (argc != 3) {
        fprintf(stderr, "Usage: <input filename> <output filename>\n");
        return -1;
    }
    /* stat() 函数，用于获取文件属性 */
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

