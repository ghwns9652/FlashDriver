/*
 * buse - block-device userspace extensions
 * Copyright (C) 2013 Adam Cozzette
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _POSIX_C_SOURCE (200809L)

#include <assert.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <linux/nbd.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "interface.h"
#include "buse.h"
#include "../bench/measurement.h"
#include "../include/FS.h"
#include "../include/settings.h"

#ifndef BUSE_DEBUG
#define BUSE_DEBUG (0)
#endif

#ifdef BUSE_MEASURE
extern MeasureTime buseTime;
#endif
struct nbd_reply reply;
int g_sk;
pthread_mutex_t sk_lock;
pthread_mutex_t req_lock;
extern pthread_mutex_t reply_lock;

#ifdef QPRINT
extern master_processor mp;
extern queue *request_q;
extern queue *reply_q;
#ifdef vcu108
extern queue *vcu_q;
#endif
#ifdef linux_aio
extern int numio;
#endif
#endif

pthread_t srvtid[NUM_SERVE_THRD];

int sp[2];

/*
 * These helper functions were taken from cliserv.h in the nbd distribution.
 */
#ifdef WORDS_BIGENDIAN
u_int64_t ntohll(u_int64_t a) {
    return a;
}
#else
u_int64_t ntohll(u_int64_t a) {
    u_int32_t lo = a & 0xffffffff;
    u_int32_t hi = a >> 32U;
    lo = ntohl(lo);
    hi = ntohl(hi);
    return ((u_int64_t) lo) << 32U | hi;
}
#endif
#define htonll ntohll

int read_all(int fd, char* buf, size_t count)
{
    int bytes_read;

    pthread_mutex_lock(&sk_lock);
    while (count > 0) {
        bytes_read = read(fd, buf, count);
        assert(bytes_read > 0);
        buf += bytes_read;
        count -= bytes_read;
    }
    assert(count == 0);
    pthread_mutex_unlock(&sk_lock);

    return 0;
}

int write_all(int fd, char* buf, size_t count)
{
    int bytes_written;

    pthread_mutex_lock(&sk_lock);
    while (count > 0) {
        bytes_written = write(fd, buf, count);
        assert(bytes_written > 0);
        buf += bytes_written;
        count -= bytes_written;
    }
    assert(count == 0);
    pthread_mutex_unlock(&sk_lock);

    return 0;
}

/* Signal handler to gracefully disconnect from nbd kernel driver. */
int nbd_dev_to_disconnect = -1;
void disconnect_nbd(int signal) {
    (void)signal;
    if (nbd_dev_to_disconnect != -1) {
        if(ioctl(nbd_dev_to_disconnect, NBD_DISCONNECT) == -1) {
            warn("failed to request disconect on nbd device");
        } else {
            nbd_dev_to_disconnect = -1;
            fprintf(stderr, "sucessfuly requested disconnect on nbd device\n");
        }
    }
}

/* Sets signal action like regular sigaction but is suspicious. */
static int set_sigaction(int sig, const struct sigaction * act) {
    struct sigaction oact;
    int r = sigaction(sig, act, &oact);
    if (r == 0 && oact.sa_handler != SIG_DFL) {
        warnx("overriden non-default signal handler (%d: %s)", sig, strsignal(sig));
    }
    return r;
}

static void print_queue_usage(){
    int ret_nread;
#ifdef QPRINT
    ioctl(sk, FIONREAD, &ret_nread);
    printf("--------------------------------\n");
    printf("buffer usage : %d\n", ret_nread);
    printf("request_q usage : %d\n", request_q->size);
    printf("reply_q usage : %d\n", reply_q->size);
    printf("interface_q usage : %d\n", (mp.processors[0].req_q)->size);
#ifdef vcu108
    printf("vcu_q usage : %d\n", vcu_q->size);
#endif
#ifdef linux_aio
    printf("aio_q usage : %d\n", numio);
#endif
    printf("--------------------------------\n");
#endif
}

/* Serve userland side of nbd socket. If everything worked ok, return 0. */
static int __serve_nbd(int sk, const struct buse_operations * aop, void * userdata) {
    u_int64_t from;
    u_int32_t len;
    ssize_t bytes_read;
    struct nbd_request request;
    void *chunk;
    int ret_nread;

    //g_sk = sk;
    reply.magic = htonl(NBD_REPLY_MAGIC);
    reply.error = htonl(0);
#ifdef LPRINT
    struct timeval busestart, buseend;
#endif
    while (1) {
        pthread_mutex_lock(&req_lock);
        bytes_read = read(sk, &request, sizeof(request));
#ifdef LPRINT
        gettimeofday(&busestart, NULL);
#endif
        if(bytes_read <= 0){
            pthread_mutex_unlock(&req_lock);
            break;
        }

        if(ntohl(request.type) != NBD_CMD_WRITE)
            pthread_mutex_unlock(&req_lock);

        print_queue_usage(); 

        assert(bytes_read == sizeof(request));

        len = ntohl(request.len);
        from = ntohll(request.from);
        assert(request.magic == htonl(NBD_REQUEST_MAGIC));

        switch(ntohl(request.type)) {
            /* I may at some point need to deal with the the fact that the
             * official nbd server has a maximum buffer size, and divides up
             * oversized requests into multiple pieces. This applies to reads
             * and writes.
             */
            case NBD_CMD_READ:
#ifdef BUSE_MEASURE
                MS(&buseTime);
#endif
                if (BUSE_DEBUG) fprintf(stderr, "Request for read of size %d\n", len);
                /* Fill with zero in case actual read is not implemented */
                if (aop->read) {
                     aop->read(sk, NULL, len, from, request.handle);
                } 
#ifdef BUSE_MEASURE
                MA(&buseTime);
#endif
                break;
            case NBD_CMD_WRITE:
                if (BUSE_DEBUG) fprintf(stderr, "Request for write of size %d\n", len);
                F_malloc(&chunk, len, FS_SET_T);
                read_all(sk, (char*)chunk, len);
                pthread_mutex_unlock(&req_lock);
                if (aop->write) {
                    aop->write(sk, chunk, len, from, request.handle);
                } 
                break;
            case NBD_CMD_DISC:
                if (BUSE_DEBUG) fprintf(stderr, "Got NBD_CMD_DISC\n");
                /* Handle a disconnect request. */
                if (aop->disc) {
                    aop->disc(sk, userdata);
                }
                return EXIT_SUCCESS;
#ifdef NBD_FLAG_SEND_FLUSH
            case NBD_CMD_FLUSH:
                if (BUSE_DEBUG) fprintf(stderr, "Got NBD_CMD_FLUSH\n");
                if (aop->flush) {
                    aop->flush(sk, userdata);
                }
                memcpy(reply.handle, request.handle, sizeof(reply.handle));
                pthread_mutex_lock(&reply_lock);
                write_all(sk, (char*)&reply, sizeof(struct nbd_reply));
                pthread_mutex_unlock(&reply_lock);
                break;
#endif
#ifdef NBD_FLAG_SEND_TRIM
            case NBD_CMD_TRIM:
                if (BUSE_DEBUG) fprintf(stderr, "Got NBD_CMD_TRIM\n");
                if (aop->trim) {
                    aop->trim(sk, from, len, request.handle);
                }
                break;
#endif
            default:
                assert(0);
        }
#ifdef LPRINT
        gettimeofday(&buseend, NULL);
        printf("buse latency [sec, usec] : [%ld, %ld]\n", buseend.tv_sec-busestart.tv_sec, buseend.tv_usec-busestart.tv_usec);
#endif
    }
    if (bytes_read == -1) {
        warn("error reading userside of nbd socket");
        return EXIT_FAILURE;
    }
    printf("bytes_read after while : %d\n", bytes_read);
    return EXIT_SUCCESS;
}

static void* serve_nbd(void* args)
{
    return (void*)__serve_nbd(sp[0], (struct buse_operations*)args, NULL);
}

int __buse_main(const char* dev_file, const struct buse_operations *aop, void *userdata)
{
    int nbd, sk, err, flags;
    socklen_t argsize;
    int sndsize, rcvsize;

    pthread_mutex_init(&sk_lock, NULL);
    pthread_mutex_init(&req_lock, NULL);

    err = socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
    assert(!err);

    printf("opened device  : %s\n", dev_file);
    nbd = open(dev_file, O_RDWR);
    if (nbd == -1) {
        fprintf(stderr, 
                "Failed to open `%s': %s\n"
                "Is kernel module `nbd' loaded and you have permissions "
                "to access the device?\n", dev_file, strerror(errno));
        return 1;
    }

    if (aop->blksize) {
        err = ioctl(nbd, NBD_SET_BLKSIZE, aop->blksize);
        assert(err != -1);
    }
    if (aop->size) {
        err = ioctl(nbd, NBD_SET_SIZE, aop->size);
        assert(err != -1);
    }
    if (aop->size_blocks) {
        err = ioctl(nbd, NBD_SET_SIZE_BLOCKS, aop->size_blocks);
        assert(err != -1);
    }

    err = ioctl(nbd, NBD_CLEAR_SOCK);
    assert(err != -1);

    pid_t pid = fork();
    if (pid == 0) {
        /* Block all signals to not get interrupted in ioctl(NBD_DO_IT), as
         * it seems there is no good way to handle such interruption.*/
        sigset_t sigset;
        if (
                sigfillset(&sigset) != 0 ||
                sigprocmask(SIG_SETMASK, &sigset, NULL) != 0
           ) {
            warn("failed to block signals in child");
            exit(EXIT_FAILURE);
        }

        /* The child needs to continue setting things up. */
        close(sp[0]);
        sk = sp[1];

        //FIXME: check effect of window size
        argsize = sizeof(int);
        getsockopt(sk, SOL_SOCKET, SO_SNDBUF, &sndsize, &argsize);
        getsockopt(sk, SOL_SOCKET, SO_RCVBUF, &rcvsize, &argsize);

        sndsize*=WDSIZE;
        rcvsize*=WDSIZE;

        setsockopt(sk, SOL_SOCKET, SO_SNDBUF, &sndsize, argsize);
        setsockopt(sk, SOL_SOCKET, SO_RCVBUF, &rcvsize, argsize);

        if(ioctl(nbd, NBD_SET_SOCK, sk) == -1){
            printf("sk ioctl error\n");
            fprintf(stderr, "ioctl(nbd, NBD_SET_SOCK, sk) failed.[%s]\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else{
#if defined NBD_SET_FLAGS
            flags = 0;
#if defined NBD_FLAG_SEND_TRIM
            flags |= NBD_FLAG_SEND_TRIM;
#endif
#if defined NBD_FLAG_SEND_FLUSH
            flags |= NBD_FLAG_SEND_FLUSH;
#endif
            if (flags != 0 && ioctl(nbd, NBD_SET_FLAGS, flags) == -1){
                fprintf(stderr, "ioctl(nbd, NBD_SET_FLAGS, %d) failed.[%s]\n", flags, strerror(errno));
                exit(EXIT_FAILURE);
            }
#endif
            err = ioctl(nbd, NBD_DO_IT);
            if (BUSE_DEBUG) fprintf(stderr, "nbd device terminated with code %d\n", err);
            if (err == -1) {
                warn("NBD_DO_IT terminated with error");
                exit(EXIT_FAILURE);
            }
        }

        if (
                ioctl(nbd, NBD_CLEAR_QUE) == -1 ||
                ioctl(nbd, NBD_CLEAR_SOCK) == -1
           ) {
            warn("failed to perform nbd cleanup actions");
            exit(EXIT_FAILURE);
        }

        exit(0);
    }

    /* Parent handles termination signals by terminating nbd device. */
    assert(nbd_dev_to_disconnect == -1);
    nbd_dev_to_disconnect = nbd;
    struct sigaction act;
    act.sa_handler = disconnect_nbd;
    act.sa_flags = SA_RESTART;
    if (
            sigemptyset(&act.sa_mask) != 0 ||
            sigaddset(&act.sa_mask, SIGINT) != 0 ||
            sigaddset(&act.sa_mask, SIGTERM) != 0
       ) {
        warn("failed to prepare signal mask in parent");
        return EXIT_FAILURE;
    }
    if (
            set_sigaction(SIGINT, &act) != 0 ||
            set_sigaction(SIGTERM, &act) != 0
       ) {
        warn("failed to register signal handlers in parent");
        return EXIT_FAILURE;
    }

    close(sp[1]);
    sk = sp[0];
    g_sk = sk;

    //FIXME: check effect of window size
    argsize = sizeof(int);
    getsockopt(sk, SOL_SOCKET, SO_SNDBUF, &sndsize, &argsize);
    getsockopt(sk, SOL_SOCKET, SO_RCVBUF, &rcvsize, &argsize);

    sndsize*=WDSIZE;
    rcvsize*=WDSIZE;

    setsockopt(sk, SOL_SOCKET, SO_SNDBUF, &sndsize, argsize);
    setsockopt(sk, SOL_SOCKET, SO_RCVBUF, &rcvsize, argsize);

    /* serve NBD socket */
    int status;
    for(int i = 0; i < NUM_SERVE_THRD; i++){
        pthread_create(&srvtid[i], NULL, serve_nbd, (void*)aop);
    }
    //long temp_stat;
    for(int i = 0; i < NUM_SERVE_THRD; i++){
        pthread_join(srvtid[i], NULL);
    }
    //status = serve_nbd(sp[0], aop, userdata);
    if (close(sp[0]) != 0) warn("problem closing server side nbd socket");
    if (status != 0) return status;

    /* wait for subprocess */
    if (waitpid(pid, &status, 0) == -1) {
        warn("waitpid failed");
        return EXIT_FAILURE;
    }
    if (WEXITSTATUS(status) != 0) {
        return WEXITSTATUS(status);
    }

    return EXIT_SUCCESS;
}
