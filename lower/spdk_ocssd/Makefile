SRCS +=\
	   ocssd_inf.c\

OBJS :=\
	$(patsubst %.c,%.o,$(SRCS))\

INC=-I$(CURDIR)/spdk/include

TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\

DEBUG : all
all: $(TARGETOBJ) #libspdk.a

.c.o :
	$(CC) $(CFLAGS_LOWER) $(TARGET_CC) -c $< -o $@

../../object/%.o: %.c
	$(CC) $(CFLAGS_LOWER) $(TARGET_CC) $(INC) -c $< -o $@


clean : 
	@$(RM) *.o
