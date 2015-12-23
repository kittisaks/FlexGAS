ifndef COMMON_DIR
include ../../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

include $(CH_DIR)/config.mk
include $(CMMN_DIR)/config.mk

AGENT_BIN_DIR = $(AGENT_DIR)/bin
AGENT_OBJ_DIR = $(AGENT_DIR)/obj

AGENT_TARGET  = $(AGENT_BIN_DIR)/vagt

AGENT_SRC     = agent_threads.cpp        \
                agent_local_handler.cpp  \
                agent_remote_handler.cpp \
                agent_settings.cpp       \
                agent_trace.cpp          \
                main.cpp

AGENT_CPP_INC_FLAGS = -I$(LM_DIR) -I$(CH_DIR) -I$(CMMN_DIR) -I$(API_DIR) -I$(SCHED_DIR)
AGENT_LD_PFLAGS = -L$(CH_DIR)/bin -L$(LM_DIR)/bin
AGENT_LD_SFLAGS = -lpthread -lch -llm

P_AGENT_SRC = $(addprefix $(AGENT_DIR)/, $(AGENT_SRC))
P_AGENT_OBJ = $(addprefix $(AGENT_OBJ_DIR)/, $(AGENT_SRC:.cpp=.o))

AGENT_CLEAN = clean-agent
AGENT_PURGE = purge-agent

CPP_OPT_FLAGS :=
ifeq ($(OPT_ENABLE_TRACE), 1)
AGENT_CPP_OPT += -D_ENABLE_TRACE
endif

ifeq ($(OPT_DISABLE_LM_RDMA), 1)
AGENT_CPP_OPT += -D_DISABLE_LM_RDMA
endif

