STRICT_COMPILE=0
include ../../../../Makefile.common

TARGET  = event
SRC     = event.cpp
OBJ     = $(addprefix obj/, $(SRC:.cpp=.o))

CPP_INC = -I$(LM_DIR)
CPP_DBG = -g
LD_INC  = -L$(LM_DIR)/bin
LD_LNK  = -llm

.PHONY: all clean

all: $(TARGET)
	@printf "Build complete.\n"

clean:
	$(RM) $(RM_FLAGS) $(TARGET)
	$(RM) $(RM_FLAGS) $(OBJ)
	$(RM) $(RM_FLAGS) obj/*.d
	@printf "Clean complete.\n"

$(TARGET) : $(OBJ) $(CH_DIR)/bin/$(wildcard *.so) $(CH_DIR)/bin/$(wildcard *.a)
	$(LD) $(LD_INC) $(OBJ) -o $@ -Wl,-Bstatic $(LD_LNK) -Wl,-Bdynamic -libverbs

obj/%.o : %.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_INC) $< -o $@

