include $(BUILD_ROOT)/mk/configs/compiler/include_common.mk
include $(BUILD_ROOT)/mk/configs/compiler/defines_common.mk
include $(BUILD_ROOT)/mk/configs/compiler/mingw_common.mk
include $(BUILD_ROOT)/mk/configs/compiler/gnu-common.mk

CC :=  /mingw/bin/gcc
CXX := /mingw/bin/g++
GCOV := /mingw/bin/gcov
AR := /mingw/bin/ar

BUILD_32BIT := -m32

CC_MUNCH := $(BUILD_ROOT)/mk/bin/empty.sh

LINK_LIB := $(AR)
LINK_LIB_FLAGS := rcs
LIBRARY_TO := 
POST_LINK_LIB := ranlib

LINK_BIN := $(CXX)
LINK_BIN_FLAGS := $(LIBS)

FILE_SIZE := $(LS) $(LS_SIZE)
LOAD_SIZE := $(SIZE)



LINK_LIBS :=  -lm 

OPT_SPEED := -Os
DEBUG := -g3

LINUX_GNU_CFLAGS := $(LINUX_FLAGS_COMMON) \
					$(COMMON_DEFINES) \
					$(GNU_CFLAGS_COMMON)

LINUX_GNU_CXXFLAGS :=	$(LINUX_FLAGS_COMMON) \
						$(COMMON_DEFINES) \
						$(GNU_CXXFLAGS_COMMON)

COVERAGE := -fprofile-arcs -ftest-coverage

LINUX_GNU_INCLUDES := $(LINUX_INCLUDES_COMMON) $(COMMON_INCLUDES)

DUMP = $(BUILD_ROOT)/mk/bin/empty.sh

MUNCH := $(BUILD_ROOT)/mk/bin/empty.sh
