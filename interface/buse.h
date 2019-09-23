#ifndef BUSE_H_INCLUDED
#define BUSE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "../include/types.h"
#include "../include/settings.h"
#include "interface.h"

    struct buse{
        u_int32_t len;
        u_int64_t offset;
        int type;
        value_set *value;
        char handle[8];
    };

    struct buse_request {
        int type;
        u_int32_t len;
        void *buf;
        u_int64_t offset;
        char handle[8];
    };

	struct buse_operations {
		int (*read)(int sk,void *buf, u_int32_t len, u_int64_t offset, void *userdata);
		int (*write)(int sk,void *buf, u_int32_t len, u_int64_t offset, void *userdata);
		void (*disc)(int sk,void *userdata);
		int (*flush)(int sk,void *userdata);
		int (*trim)(int sk,u_int64_t from, u_int32_t len, void *userdata);

		// either set size, OR set both blksize and size_blocks
		u_int64_t size;
		u_int32_t blksize;
		u_int64_t size_blocks;
	};

    uint32_t lpa2ppa(uint32_t lpa);
    uint32_t getPhysPageAddr(int fd, size_t byteOffset);
	int __buse_main(const char* dev_file, const struct buse_operations *bop, void *userdata);
    int buse_init();
    int buse_free();
    void disconnect_nbd(int signal);

#ifdef __cplusplus
}
#endif

int write_all(int fd, char* buf, size_t count);
#endif /* BUSE_H_INCLUDED */
