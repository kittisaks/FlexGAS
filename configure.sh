#!/bin/bash

for arg in "$@"
do
    if [ "$arg" == "--disable-lm-rdma" ]
        then
        OPT_DISABLE_LM_RDMA="1"
    elif [ "$arg" == "--enable-trace" ]
        then
        OPT_ENABLE_TRACE="1"
    elif [ "$arg" == "--disable-gpu-support" ]
        then
        OPT_DISABLE_GPU_SUPPORT="1"
    fi
done

ROOT_DIR=`pwd`
if [ "$IBV_INC_DIR" == "" ]
    then
    IBV_INC_DIR="/usr/include/infiniband/"
fi
if [ "$IBV_LIB_DIR" == "" ]
    then
    IBV_LIB_DIR="/usr/lib64/"
fi
if [ "$RDMACM_INC_DIR" == "" ]
    then
    RDMACM_INC_DIR="/usr/include/rdma"
fi
if [ "$RDMACM_LIB_DIR" == "" ]
    then
    RDMACM_LIB_DIR="/usr/lib64"
fi
if [ "$CUDA_DIR" == "" ]
    then
    CUDA_DIR="/usr/local/cuda"
fi

printf "
###### Project directories #####
ROOT_DIR         = $ROOT_DIR
LM_DIR           = \$(ROOT_DIR)/src/lm
CH_DIR           = \$(ROOT_DIR)/src/ch
CMMN_DIR         = \$(ROOT_DIR)/src/common

##### IVM project directories #####

##### vIVM project directories #####
VIVM_DIR         = \$(ROOT_DIR)/src/vivm
AGENT_DIR        = \$(VIVM_DIR)/agent
API_DIR          = \$(VIVM_DIR)/api
SCHED_DIR        = \$(VIVM_DIR)/sched
SCHEDLIB_DIR     = \$(VIVM_DIR)/schedlib

##### Dependencies directory #####
IBV_INC_DIR      = $IBV_INC_DIR
IBV_LIB_DIR      = $IBV_LIB_DIR
RDMACM_INC_DIR   = $RDMACM_INC_DIR
RDMACM_LIB_DIR   = $RDMACM_LIB_DIR
CUDA_DIR         = $CUDA_DIR
CUDA_INC_DIR     = $CUDA_DIR/include
CUDA_LIB_DIR     = $CUDA_DIR/lib64

##### Options #####
OPT_DISABLE_LM_RDMA = $OPT_DISABLE_LM_RDMA
OPT_ENABLE_TRACE    = $OPT_ENABLE_TRACE
OPT_DISABLE_GPU_SUPPORT = $OPT_DISABLE_GPU_SUPPORT

##### Compiler #####
OPTIM_FLAGS      = -O2
ifeq (\$(STRICT_COMPILE), 1)
    STRICT_FLAGS = -Werror
else
    STRICT_FLAGS =
endif
C                = gcc
C_FLAGS          = -c -Wall \$(STRICT_FLAGS) \$(OPTIM_FLAGS) -g -MD
CPP              = g++
CPP_FLAGS        = -c -Wall \$(STRICT_FLAGS) \$(OPTIM_FLAGS) -g -MD
CXX_SHARED_FLAGS = -fPIC

##### Linker #####
LD               = \$(CPP)
LD_FLAGS         =
LD_SHARED_FLAGS  = --shared

##### Utils #####
CP               = cp
CP_FLAGS         =
RM               = rm
RM_FLAGS         = -f
AR               = ar
AR_FLAGS         = rcs

" > $ROOT_DIR/Makefile.common

printf "\nConfigure completed...\n\n"

