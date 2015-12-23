ifndef COMMON_DIR
include ../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

include $(LM_DIR)/config.mk

CH_BIN_DIR  = $(CH_DIR)/bin
CH_OBJ_DIR  = $(CH_DIR)/obj

CH_TARGET   = $(CH_BIN_DIR)/libch.so \
              $(CH_BIN_DIR)/libch.a

CH_SRC    = channel.cpp          \
            channel_internal.cpp \
            channel_sendrecv.cpp \
            channel_remote.cpp

CPP_INC_FLAGS = -I$(LM_DIR)
CHLD_PFLAGS   = -L$(LM_BIN_DIR)
CHLD_SFLAGS   = -Wl,-Bstatic -llm -Wl,-Bdynamic -lpthread -lrt

P_CH_SRC = $(patsubst %, $(CH_DIR)/%, $(CH_SRC))
P_CH_OBJ = $(patsubst %, $(CH_OBJ_DIR)/%, $(CH_SRC:.cpp=.o))

CH_CLEAN = clean-ch
CH_PURGE = purge-ch

ifeq ($(OPT_DISABLE_LM_RDMA), 1)
CPP_ADD_FLAGS = -D_DISABLE_LM_RDMA 
else
CPP_ADD_FLAGS =
CHLD_SFLAGS  += -libverbs -lrdmacm
endif

