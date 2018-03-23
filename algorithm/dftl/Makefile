CC=gcc

TARGET_CACHE_UNIT=UNIT_T

CFLAGS +=\
		 -g\
		 -std=c99\
		 -Wno-discarded-qualifiers\
		 -D$(TARGET_CACHE_UNIT)\

SRCS +=\
	   dftl_init.c\
	   dftl_main.c\
	   tpage_dftl_main.c\
	   dftl_gc.c\
	   tpage_dftl_gc.c\
	   dftl_queue.c\

OBJS :=\
	$(patsubst %.c,%.o,$(SRCS))\

TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\

LEAK: all
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
