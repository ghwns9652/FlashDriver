
SRCS +=\
	   vcu108_inf.cpp\

OBJS :=\
	$(patsubst %.cpp,%.o,$(SRCS))\

TARGETOBJ:=\
	$(addprefix ../../object/,$(OBJS))\

CONNECTALDIR?=../../../../platform/build_tools/connectal
DTOP?=../../../../platform_tests/flash_dual_no_ind_vcu108/vcu108

CFLAGS_LOWER += -O\
                -g\
                -I$(DTOP)/jni\
                -I$(CONNECTALDIR)\
                -I$(CONNECTALDIR)/cpp\
                -I$(CONNECTALDIR)/lib/cpp\
                -Wall\
                -Werror\
                -I$(DTOP)/jni\
                -I$(CONNECTALDIR)\
                -I$(CONNECTALDIR)/cpp\
                -I$(CONNECTALDIR)/lib/cpp\

LDLIBS := -lpthread

DEBUG : all
all: $(TARGETOBJ)

libpos.a : $(TARGETOBJ)
	$(AR) r $(@) $(TARGETOBJ)
	

.cpp.o :
	$(CC) $(CFLAGS_LOWER) -c $< -o $@ $(LDLIBS)

../../object/%.o: %.cpp
	$(CC) $(CFLAGS_LOWER) -c $< -o $@ $(LDLIBS)

clean : 
	@$(RM) libpos.a
	@$(RM) *.o
