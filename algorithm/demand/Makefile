CC = g++

CFLAGS_ALGO +=\

SRCS +=\
	   demand.c\
	   demand_internal.c\
	   cache.c\
	   block.c\
	   gc.c\
	   lru.c

OBJS :=\
	$(patsubst %.c,%.o,$(SRCS))\

OBJS+=skiplist.o

TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\

LEAK: all
all: $(TARGETOBJ) 

../../object/skiplist.o: ../Lsmtree/skiplist.c
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

libdemand.a : $(TARGETOBJ)
	$(AR) r $(@) $(TARGETOBJ)

.c.o :
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

../../object/%.o: %.c
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

clean : 
	@$(RM) libdemand.a
	@$(RM) *.o
