ifndef COMMON_DIR
include ../../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

include $(CH_DIR)/config.mk
include $(CMMN_DIR)/config.mk

SCHED_BIN_DIR = $(SCHED_DIR)/bin
SCHED_OBJ_DIR = $(SCHED_DIR)/obj

SCHED_TARGET  = $(SCHED_BIN_DIR)/vsched

SCHED_SRC     = sched_settings.cpp       \
                sched_threads.cpp        \
                sched_local_handler.cpp  \
                sched_remote_handler.cpp \
                sched_api.cpp            \
                sched_trace.cpp          \
                main.cpp

SCHED_CPP_INC_FLAGS = -I$(LM_DIR) -I$(CH_DIR) -I$(CMMN_DIR) -I$(API_DIR) -I$(SCHED_DIR)
SCHED_LD_PFLAGS = -L$(CH_DIR)/bin -L$(LM_DIR)/bin
SCHED_LD_SFLAGS = -lpthread -ldl -lch -llm

P_SCHED_SRC = $(addprefix $(SCHED_DIR)/, $(SCHED_SRC))
P_SCHED_OBJ = $(addprefix $(SCHED_OBJ_DIR)/, $(SCHED_SRC:.cpp=.o))

SCHED_CLEAN = clean-sched
SCHED_PURGE = purge-sched

CPP_OPT_FLAGS :=
ifeq ($(OPT_ENABLE_TRACE), 1)
SCHED_CPP_OPT += -D_ENABLE_TRACE
endif

ifeq ($(OPT_DISABLE_LM_RDMA), 1)
SCHED_CPP_OPT += -D_DISABLE_LM_RDMA
endif

