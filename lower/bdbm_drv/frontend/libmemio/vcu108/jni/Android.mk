
include $(CLEAR_VARS)
DTOP?=/home/chamdoo/NDPSql/platform_tests/flash_dual_no_ind_vcu108/vcu108
CONNECTALDIR?=/home/chamdoo/NDPSql/platform/build_tools/connectal
LOCAL_ARM_MODE := arm
include $(CONNECTALDIR)/scripts/Makefile.connectal.application
LOCAL_SRC_FILES := /home/chamdoo/NDPSql/platform_tests/flash_dual_no_ind_vcu108/testflash.cpp /home/chamdoo/NDPSql/platform/build_tools/connectal/cpp/dmaManager.c /home/chamdoo/NDPSql/platform/build_tools/connectal/cpp/platformMemory.cpp $(PORTAL_SRC_FILES)

LOCAL_PATH :=
LOCAL_MODULE := android.exe
LOCAL_MODULE_TAGS := optional
LOCAL_LDLIBS := -llog   
LOCAL_CPPFLAGS := "-march=armv7-a"
LOCAL_CFLAGS := -I$(DTOP)/jni -I$(CONNECTALDIR) -I$(CONNECTALDIR)/cpp -I$(CONNECTALDIR)/lib/cpp   -Werror
LOCAL_CXXFLAGS := -I$(DTOP)/jni -I$(CONNECTALDIR) -I$(CONNECTALDIR)/cpp -I$(CONNECTALDIR)/lib/cpp   -Werror
LOCAL_CFLAGS2 := $(cdefines2)s

include $(BUILD_EXECUTABLE)
