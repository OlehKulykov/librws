LOCAL_PATH:= $(call my-dir)


ALL_SOURCES := \
	../../../src/common.c \
	../../../src/error.c \
	../../../src/frame.c \
	../../../src/librws.c \
	../../../src/list.c \
	../../../src/memory.c \
	../../../src/socketpriv.c \
	../../../src/socketpub.c \
	../../../src/string.c \
	../../../src/thread.c


ALL_INCLUDES := $(LOCAL_PATH)/../../../

ALL_CFLAGS := -w

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(ALL_SOURCES)
LOCAL_C_INCLUDES += $(ALL_INCLUDES)
LOCAL_CFLAGS += $(ALL_CFLAGS)
LOCAL_MODULE := librws
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)

