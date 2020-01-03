COMP ?= DEFAULT

ifeq ($(COMP),DEFAULT)
 include $(BUILD_ROOT)/mk/configs/builds/raspian-gcc-cross.mk
endif

