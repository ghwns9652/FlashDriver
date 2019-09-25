#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "buse.h"

int main(int argc, char *argv[]) {
    int fd, ret;
    void *buf;

    buse_init(); //start nbd device, buse, vcu108
    run_exec("mkfs -t ext4 /dev/nbd0");
    run_exec("mount /dev/nbd0 /mnt/nbd0");

    fd = open("/mnt/nbd0/test.txt", O_RDWR|O_CREAT|O_DIRECT, 0666);
    printf("fd : %d\n", fd);
    posix_memalign(&buf, 512, 16384);
    memset(buf, 'A', 16384);
    ret = write(fd, buf, 16384);
    printf("ret : %d\n", ret);
    printf("ppa : %u\n", getPhysPageAddr(fd, 4096));
    close(fd);

    run_exec("umount /dev/nbd0"); //must unmount nbd before buse exit
    buse_wait();
    buse_free(); 

    return 0;
}
