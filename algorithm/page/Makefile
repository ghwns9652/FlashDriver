CC=gcc

CFLAGS +=\
		 -g\
		 -std=c99\
		 -Wno-discarded-qualifiers\


SRCS +=\
		PM_blockqueue.c\
		PM_operation.c\
	   page_PM.c\

OBJS :=\
	$(patsubst %.c,%.o,$(SRCS))\

TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\


all: $(TARGETOBJ) 

libnom.a : $(TARGETOBJ)
	$(AR) r $(@) $(TARGETOBJ)

.c.o :
	$(CC) $(CFLAGS) -c $< -o $@

../../object/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean : 
	@$(RM) libnom.a
	@$(RM) *.o
