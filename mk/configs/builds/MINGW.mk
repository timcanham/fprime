COMP ?= DEFAULT

ifeq ($(COMP),DEFAULT)
 include $(BUILD_ROOT)/mk/configs/builds/mingw-gcc-debug.mk
endif

ifeq ($(COMP),gcc-debug)
 include $(BUILD_ROOT)/mk/configs/builds/mingw-gcc-debug.mk
endif

ifeq ($(COMP),comp-ut)
 include $(BUILD_ROOT)/mk/configs/builds/mingw-gcc-ut.mk
endif

ifeq ($(COMP),comp-ut-nocov)
 include $(BUILD_ROOT)/mk/configs/builds/mingw-gcc-ut-nocov.mk
endif