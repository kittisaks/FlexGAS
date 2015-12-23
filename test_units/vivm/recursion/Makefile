STRICT_COMPILE=0
include ../../../Makefile.common

TARGET  = recursion
SRC     = recursion.cpp
OBJ     = $(addprefix obj/, $(SRC:.cpp=.o))

CPP_INC = -I$(CH_DIR) -I$(LM_DIR) -I$(VIVM_DIR)/api
CPP_DBG = -g
LD_INC  = -L$(CH_DIR)/bin -L$(LM_DIR)/bin -L$(VIVM_DIR)/api/bin
LD_DLNK = -llm -lch -lvivm
LD_SLNK = -lpthread -lrt

RECURSION_OPT :=
ifneq ($(OPT_DISABLE_LM_RDMA), 1)
RECURSION_OPT = -libverbs
endif

.PHONY: all clean

all: $(TARGET)
	@printf "Build complete.\n"

clean:
	$(RM) $(RM_FLAGS) $(TARGET)
	$(RM) $(RM_FLAGS) $(OBJ)
	$(RM) $(RM_FLAGS) obj/*.d
	@printf "Clean complete.\n"

$(TARGET) : $(OBJ) $(CH_DIR)/bin/$(wildcard *.so) $(LM_DIR)/bin/$(wildcard *.a)
	$(LD) $(LD_INC) $(OBJ) -o $@ -Wl,-Bstatic $(LD_DLNK) -Wl,-Bdynamic $(RECURSION_OPT) $(LD_SLNK)

obj/%.o : %.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_INC) $< -o $@

