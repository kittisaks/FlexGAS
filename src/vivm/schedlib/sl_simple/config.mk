ifndef COMMON_DIR
include ../../../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

include $(CH_DIR)/config.mk
include $(CMMN_DIR)/config.mk

SL_SIMPLE_DIR     = $(SCHEDLIB_DIR)/sl_simple
SL_SIMPLE_BIN_DIR = $(SL_SIMPLE_DIR)/bin
SL_SIMPLE_OBJ_DIR = $(SL_SIMPLE_DIR)/obj

SL_SIMPLE_TARGET  = $(SL_SIMPLE_BIN_DIR)/sl_simple.so

SL_SIMPLE_SRC     = simple.c simp_job.c simp_group.c

SL_SIMPLE_CPP_INC_FLAGS = -I$(LM_DIR) -I$(CH_DIR) -I$(CMMN_DIR) -I$(API_DIR) -I$(SCHED_DIR) -I$(SCHEDLIB_DIR) -I$(SL_SIMPLE_DIR)
SL_SIMPLE_LD_PFLAGS = -L$(CH_DIR)/bin -L$(LM_DIR)/bin
SL_SIMPLE_LD_SFLAGS = -lpthread -lch -llm

P_SL_SIMPLE_SRC = $(addprefix $(SL_SIMPLE_DIR)/, $(SL_SIMPLE_SRC))
P_SL_SIMPLE_OBJ = $(addprefix $(SL_SIMPLE_OBJ_DIR)/, $(SL_SIMPLE_SRC:.c=.o))

SL_SIMPLE_CLEAN = clean-sl_simple
SL_SIMPLE_PURGE = purge-sl_simple

CPP_OPT_FLAGS :=
ifeq ($(OPT_ENABLE_TRACE), 1)
SL_SIMPLE_CPP_OPT += -D_ENABLE_TRACE
endif

ifeq ($(OPT_DISABLE_LM_RDMA), 1)
SL_SIMPLE_CPP_OPT += -D_DISABLE_LM_RDMA
endif

