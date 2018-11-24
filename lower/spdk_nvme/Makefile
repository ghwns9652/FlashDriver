SRCS +=\
	   nvme_inf.c\

OBJS :=\
	$(patsubst %.c,%.o,$(SRCS))\

TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\
#$(patsubst %.c,%.o,$(SRCS))\

#LIBOBJS:=\
#		./spdk/lib/nvme/*.o\
#		./spdk/lib/env_dpdk/*.o\
#		./spdk/lib/log/*.o\
#		./spdk/lib/util/*.o\
#		./spdk/lib/virtio/*.o\
#		./spdk/lib/nvmf/*.o\

DEBUG : all
all: $(TARGETOBJ) #libspdk.a

#libpos.a : $(TARGETOBJ)
#	$(AR) r $(@) $(TARGETOBJ)

#libspdk.a: $(LIBOBJS)
#	$(AR) r $(@) $(^)
#	mv libspdk.a ../../object/

.c.o :
	$(CC) $(CFLAGS_LOWER) $(TARGET_CC) -c $< -o $@

../../object/%.o: %.c
	$(CC) $(CFLAGS_LOWER) $(TARGET_CC) $(INC) -c $< -o $@


clean : 
#@$(RM) libpos.a
	@$(RM) *.o
#	@$(RM) ../../object/libspdk.a
