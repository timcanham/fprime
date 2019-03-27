# Makefile to run global make.
DEPLOYMENT := R5
BUILD_ROOT ?= $(subst /$(DEPLOYMENT),,$(CURDIR))

export BUILD_ROOT

default: TIR5 dict_install

include $(BUILD_ROOT)/mk/makefiles/deployment_makefile.mk

