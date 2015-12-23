ifndef COMMON_DIR
include ../../Makefile.common
else
include $(COMMON_DIR)/Makefile.common
endif

CMMN_OBJ_DIR = $(CMMN_DIR)/obj

CMMN_SRC     = thread.cpp \
               trace.cpp

CMMN_CPP_INC_FLAGS =
CMMN_LD_PFLAGS     =
CMMN_LD_SFLAGS     =

P_CMMN_SRC = $(addprefix $(CMMN_DIR)/, $(CMMN_SRC))
P_CMMN_OBJ = $(addprefix $(CMMN_OBJ_DIR)/, $(CMMN_SRC:.cpp=.o))

CMMN_CLEAN = clean-cmmn

