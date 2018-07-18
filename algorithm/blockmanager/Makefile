CC = g++

SRCS +=\
		BM_Block.c\
		BM_Heap.c\

OBJS :=\
	$(patsubst %.c,%.o,$(SRCS))\

TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\

LEAK: all
all: $(TARGETOBJ) 

libbm.a : $(TARGETOBJ)
	$(AR) r $(@) $(TARGETOBJ)

.c.o :
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

../../object/%.o: %.c
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

clean : 
	@$(RM) libbm.a
	@$(RM) *.o
