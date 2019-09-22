#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "buse.h"

int main(int argc, char *argv[]) {
    int fd, ret;
    void *buf;
    buse_init(); 

    printf("in to the sleep\n");
    sleep(60);

    fd = open("/mnt/nbd0/gppa_test.txt", O_RDWR|O_CREAT|O_DIRECT, 0666);
    printf("fd : %d\n", fd);
    posix_memalign(&buf, 512, 4096);
    memset(buf, 'A', 4096);
    ret = write(fd, buf, 4096);
    printf("ret : %d\n", ret);
    printf("ppa : %u\n", getPhysPageAddr(fd, 0));

    return 0;
}
