CC = g++

CFLAGS_ALGO +=\

SRCS +=\
	   hashftl.c\
	   hashftl_gc.c\
	   hashftl_util.c\
	   md5.c\

OBJS :=\
	$(patsubst %.c,%.o,$(SRCS))\


TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\

LEAK: all
all: $(TARGETOBJ)

#../../object/skiplist.o: ../Lsmtree/skiplist.c
#	$(CC) $(CFLAGS_ALGO) -c $< -o $@

libnom.a : $(TARGETOBJ)
	$(AR) r $(@) $(TARGETOBJ)

.c.o :
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

../../object/%.o: %.c
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

clean :
	@$(RM) libnom.a
	@$(RM) *.o
