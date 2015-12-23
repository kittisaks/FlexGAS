ifndef COMMON_DIR
include ../../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

include $(CH_DIR)/config.mk
include $(CMMN_DIR)/config.mk

SCHEDLIB_BIN_DIR = $(SCHEDLIB_DIR)/bin
SCHEDLIB_OBJ_DIR = $(SCHEDLIB_DIR)/obj

SCHEDLIB_TARGET  = $(SCHEDLIB_BIN_DIR)/schedlib.so

SCHEDLIB_SRC     = sl_main.cpp

SCHEDLIB_CPP_INC_FLAGS = -I$(LM_DIR) -I$(CH_DIR) -I$(CMMN_DIR) -I$(API_DIR) -I$(SCHED_DIR) -I$(SCHEDLIB_DIR)
SCHEDLIB_LD_PFLAGS = -L$(CH_DIR)/bin -L$(LM_DIR)/bin
SCHEDLIB_LD_SFLAGS = -lpthread -lch -llm

P_SCHEDLIB_SRC = $(addprefix $(SCHEDLIB_DIR)/, $(SCHEDLIB_SRC))
P_SCHEDLIB_OBJ = $(addprefix $(SCHEDLIB_OBJ_DIR)/, $(SCHEDLIB_SRC:.cpp=.o))

SCHEDLIB_CLEAN = clean-sched
SCHEDLIB_PURGE = purge-sched

CPP_OPT_FLAGS :=
ifeq ($(OPT_ENABLE_TRACE), 1)
SCHEDLIB_CPP_OPT += -D_ENABLE_TRACE
endif

ifeq ($(OPT_DISABLE_LM_RDMA), 1)
SCHEDLIB_CPP_OPT += -D_DISABLE_LM_RDMA
endif

