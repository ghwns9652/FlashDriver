export CC=g++

TARGET_INF=interface
TARGET_LOWER=posix_memory
TARGET_ALGO=sftl

PPWD=$(pwd)

DEBUGFLAGS=\
			-rdynamic\
			-Wno-pointer-arith\
#	-DBUSE_DEBUG

COMMONFLAGS=\
			-Wno-write-strings\
			-DLARGEFILE64_SOURCE\
			-DSLC\
#			-O2\
#			-DWRITESYNC\

COMMONFLAGS+=$(DEBUGFLAGS)\

export CFLAGS_ALGO=\
			 -g\
			 -Wall\
			 -D$(TARGET_LOWER)\
#-DDVALUE\


export CFLAGS_LOWER=\
			-g\
			 -lpthread\
			 -Wall\
			 -D_FILE_OFFSET_BITS=64\

export priority="false"
export ORIGINAL_PATH=$(PPWD)

#CFLAGS_ALGO+=-DCOMPACTIONLOG\
	
CFLAGS_ALGO+=$(COMMONFLAGS)\
			 -D$(TARGET_ALGO)\

CFLAGS_LOWER+=$(COMMONFLAGS)\
			  -D$(TARGET_ALGO)\

ifeq ($(TARGET_ALGO), lsmtree)
 CFLAGS_ALGO+=-DLSM_SKIP
endif

ifeq ($(CC), gcc)
 CFLAGS_ALGO+=-Wno-discarded-qualifiers -std=c99
 CFLAGS_LOWER+=-Wno-discarded-qualifiers -std=c99
else
 CFLAGS_ALGO+= -std=c++11
 CFLAGS_LOWER+= -std=c++11
endif

CFLAGS +=\
		 $(CFLAGS_ALGO)\
		 -D$(TARGET_LOWER)\
		 -D$(TARGET_ALGO)\
		 -D$(TARGET_INF)\
		 -D_DEFAULT_SOURCE\
		 -D_BSD_SOURCE\
-DBENCH\
-DCDF\

SRCS +=\
	./interface/queue.c\
	./interface/interface.c\
	./interface/bb_checker.c\
	./interface/buse.c\
	./include/FS.c\
	./include/slab.c\
	./include/utils/debug_tools.c\
	./include/utils/dl_sync.c\
	./include/utils/rwlock.c\
	./include/utils/cond_lock.c\
	./include/data_struct/hash.c\
	./include/data_struct/list.c\
	./include/data_struct/redblack.c\
	./bench/measurement.c\
	./bench/bench.c\
	./include/utils/thpool.c\

TARGETOBJ =\
			$(patsubst %.c,%.o,$(SRCS))\

MEMORYOBJ =\
		   	$(patsubst %.c,%_mem.o,$(SRCS))\

DEBUGOBJ =\
		   	$(patsubst %.c,%_d.o,$(SRCS))\


ifeq ($(TARGET_LOWER),bdbm_drv)
	ARCH +=./object/libmemio.a
endif

LIBS +=\
		-lpthread\
		-lm\
		-laio\
-ljemalloc\

all: range_driver

DEBUG: debug_driver

duma_sim: duma_driver

debug_driver: ./interface/main.c libdriver_d.a
	$(CC) $(CFLAGS) -DDEBUG -o $@ $^ $(LIBS)

driver: ./interface/main.c libdriver.a
	$(CC) $(CFLAGS) -o $@ $^ $(ARCH) $(LIBS)

range_driver: ./interface/range_test_main.c libdriver.a
	$(CC) $(CFLAGS) -o $@ $^ $(ARCH) $(LIBS)

duma_driver: ./interface/main.c libdriver.a
	$(CC) $(CFLAGS) -o $@ $^ -lduma $(ARCH) $(LIBS)
	

libdriver.a: $(TARGETOBJ)
	mkdir -p object && mkdir -p data
	cd ./algorithm/$(TARGET_ALGO) && $(MAKE) clean && $(MAKE) && cd ../../
	cd ./lower/$(TARGET_LOWER) && $(MAKE) && cd ../../ 
	cd ./algorithm/blockmanager && $(MAKE) && cd ../../
#cd ./include/kuk_socket_lib/ && $(MAKE) && mv ./*.o ../../object/ && cd ../../
	mv ./include/data_struct/*.o ./object/
	mv ./include/utils/*.o ./object/
	mv ./interface/*.o ./object/ && mv ./bench/*.o ./object/ && mv ./include/*.o ./object/
	$(AR) r $(@) ./object/*

%_mem.o: %.c
	$(CC) $(CFLAGS) -DLEAKCHECK -c $< -o $@ $(LIBS)

%_d.o: %.c
	$(CC) $(CFLAGS) -DDEBUG -c $< -o $@ $(LIBS)

.c.o :
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)


clean :
	cd ./algorithm/$(TARGET_ALGO) && $(MAKE) clean && cd ../../
	cd ./lower/$(TARGET_LOWER) && $(MAKE) clean && cd ../../
	@$(RM) ./data/*
	@$(RM) ./object/*.o
	@$(RM) *.a
	@$(RM) driver
	@$(RM) driver_memory_check
	@$(RM) debug_driver
	@$(RM) duma_driver
