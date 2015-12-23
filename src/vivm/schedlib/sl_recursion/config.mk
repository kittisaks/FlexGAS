ifndef COMMON_DIR
include ../../../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

include $(CH_DIR)/config.mk
include $(CMMN_DIR)/config.mk

SL_REC_DIR     = $(SCHEDLIB_DIR)/sl_recursion
SL_REC_BIN_DIR = $(SL_REC_DIR)/bin
SL_REC_OBJ_DIR = $(SL_REC_DIR)/obj

SL_REC_TARGET  = $(SL_REC_BIN_DIR)/sl_rec.so

SL_REC_SRC     = rec.cpp rec_sched.cpp

SL_REC_CPP_INC_FLAGS = -I$(LM_DIR) -I$(CH_DIR) -I$(CMMN_DIR) -I$(API_DIR) -I$(SCHED_DIR) -I$(SCHEDLIB_DIR) -I$(SL_REC_DIR)
SL_REC_LD_PFLAGS = -L$(CH_DIR)/bin -L$(LM_DIR)/bin
SL_REC_LD_SFLAGS = -lpthread -lch -llm

P_SL_REC_SRC = $(addprefix $(SL_REC_DIR)/, $(SL_REC_SRC))
P_SL_REC_OBJ = $(addprefix $(SL_REC_OBJ_DIR)/, $(SL_REC_SRC:.cpp=.o))

SL_REC_CLEAN = clean-sl_rec
SL_REC_PURGE = purge-sl_rec

CPP_OPT_FLAGS :=
ifeq ($(OPT_ENABLE_TRACE), 1)
SL_REC_CPP_OPT += -D_ENABLE_TRACE
endif

ifeq ($(OPT_DISABLE_LM_RDMA), 1)
SL_REC_CPP_OPT += -D_DISABLE_LM_RDMA
endif

