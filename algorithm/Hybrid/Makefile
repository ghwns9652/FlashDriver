SRCS +=\
		FAST_Create.c\
		FAST_Destroy.c\
		FAST_EndRequest.c\
		FAST_Get.c\
		FAST_Set.c\
		FAST_Remove.c\
		fast_WritePage.c\
		fast_ReadPage.c\
		fast_AllocDataBlockEntry.c\
		fast_AllocRWLogBlockEntry.c\
		fast_AllocSWLogBlockEntry.c\
		fast_SearchDataBlock.c\
		fast_SearchRWLogBlock.c\
		fast_SearchSWLogBlock.c\
		fast_Util.c\
		fast_SwitchSWLogBlock.c\
		fast_MergeSWLogBlock.c\
		fast_MergeRWLogBlock.c\


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
