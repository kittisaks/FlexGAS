ifndef COMMON_DIR
include ../../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

include $(AGENT_DIR)/config.mk

API_BIN_DIR = $(API_DIR)/bin
API_OBJ_DIR = $(API_DIR)/obj

API_TARGET  = $(API_BIN_DIR)/libvivm.so \
              $(API_BIN_DIR)/libvivm.a

API_SRC     = ivm_comp.cpp     \
              ivm_init.cpp     \
              ivm_memory.cpp   \
              ivm_pes.cpp      \
              api_settings.cpp 

API_CPP_INC_FLAGS = -I$(LM_DIR) -I$(CH_DIR) -I$(CMMN_DIR) -I$(AGENT_DIR) -I$(SCHED_DIR) -I./
API_LD_PFLAGS     =
API_LD_SFLAGS     =

P_API_SRC = $(addprefix $(API_DIR)/, $(API_SRC))
P_API_OBJ = $(addprefix $(API_OBJ_DIR)/, $(API_SRC:.cpp=.o))

API_CLEAN = clean-api
API_PURGE = api-purge

