ifndef COMMON_DIR
include ../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

LM_BIN_DIR = $(LM_DIR)/bin
LM_OBJ_DIR = $(LM_DIR)/obj

LM_TARGET = $(LM_BIN_DIR)/liblm.so \
            $(LM_BIN_DIR)/liblm.a

#POSIX shared memory support
LM_SRC    = lmshm_connection.cpp   \
            lmshm_sendrecv.cpp     \
            lmshm_shm.cpp
CPP_INC_FLAGS = -I$(LM_DIR)
LMLD_PFLAGS   =
LMLD_SFLAGS   = -Wl,-Bdynamic -lrt

#Infiniband support
ifneq ($(OPT_DISABLE_LM_RDMA), 1)
LM_SRC   += lmib_connection.cpp    \
            lmib_rdma.cpp          \
            lmib_sendrecv.cpp
CPP_INC_FLAGS += -I$(IBV_INC_DIR) -I$(RDMACM_INC_DIR)
LMLD_PFLAGS   += -L$(IBV_LIB_DIR) -L$(RDMACM_LIB_DIR)
LMLD_SFLAGS   += -libverbs -lrdmacm
endif

#Inet socket support
LM_SRC   += lminet_connection.cpp  \
            lminet_sendrecv.cpp    \
            lminet_handler.cpp
CPP_INC_FLAGS +=
LMLD_PFLAGS   +=
LMLD_SFLAGS   += -lpthread


P_LM_SRC = $(patsubst %, $(LM_DIR)/%, $(LM_SRC))
P_LM_OBJ = $(patsubst %, $(LM_OBJ_DIR)/%, $(LM_SRC:.cpp=.o))

LM_CLEAN = clean-lm

