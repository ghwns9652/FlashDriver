#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "buse.h"

//pthread_mutex_t cmd_lock;

/*
void sig_hdlr(int signal){
    pthread_mutex_unlock(&cmd_lock);
}
*/

int main(int argc, char *argv[]) {
    int fd, ret;
    pid_t pid;
    void *buf;

    //if(signal(SIGCHLD, sig_hdlr) == SIG_ERR)
        //fprintf(stderr, "signal handler error\n");

    //pthread_mutex_init(&cmd_lock, NULL);
    //pthread_mutex_lock(&cmd_lock);

    buse_init(); 
    run_exec("mkfs -t ext4 /dev/nbd0");
    run_exec("mount /dev/nbd0 /mnt/nbd0");

    fd = open("/mnt/nbd0/gppa_test.txt", O_RDWR|O_CREAT|O_DIRECT, 0666);
    printf("fd : %d\n", fd);
    posix_memalign(&buf, 512, 4096);
    memset(buf, 'A', 4096);
    ret = write(fd, buf, 4096);
    printf("ret : %d\n", ret);
    printf("ppa : %u\n", getPhysPageAddr(fd, 0));
    close(fd);

    pid = run_exec("umount /dev/nbd0");
    while(1){}
    //buse_free(); 

    return 0;
}
