CC=g++

TARGET_CACHE_UNIT=UNIT_T

CFLAGS_ALGO +=\
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
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

../../object/%.o: %.c
	$(CC) $(CFLAGS_ALGO) -c $< -o $@

clean : 
	@$(RM) libnom.a
	@$(RM) *.o
