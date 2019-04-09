
MKDIR := mkdir -p
export RM := rm -f
RM_DIR = $(RM) -rf 
LS := ls
SIZE := size
LS_SIZE := -hs
export CAT := cat
TIME := 
CP := cp
CP_RECURSE := $(CP) -r
TEE := tee
CHMOD_WRITE_USER := chmod -f u+w
TOUCH := touch
NM := nm
MAKE := make

SLOC_COUNTER := ${BUILD_ROOT}/mk/bin/ncsl -v -i
PROCESS_SLOC := ${BUILD_ROOT}/mk/bin/process_ncsl_sloc.py
FILE_HASH := $(PYTHON_BIN) ${BUILD_ROOT}/mk/bin/run_file_hash.py
GEN_VERSION := $(PYTHON_BIN) $(BUILD_ROOT)/mk/bin/gen_git_version.py

export PYTHON_BASE ?= c:\Python27
export PYTHON_BIN := $(PYTHON_BASE)\python.exe
export CHEETAH_COMPILE ?= $(PYTHON_BASE)\Scripts\cheetah-compile.exe

HOST_BIN_SUFFIX := .exe
JOBS = -j 4

TI_CCS_DIR := c:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.5.LTS

SYMBOL_CHECK := echo

FILE_HASH := $(PYTHON_BIN) ${BUILD_ROOT}/mk/bin/run_file_hash.py

export HOME := $(HOMEPATH)
export USER := $(USERNAME)
