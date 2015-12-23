STRICT_COMPILE=0
include ../../../Makefile.common

TARGET  = basic
SRC     = basic.cpp
OBJ     = $(addprefix obj/, $(SRC:.cpp=.o))

CPP_INC = -I$(CH_DIR) -I$(LM_DIR)
CPP_DBG = -g
LD_INC  = -L$(CH_DIR)/bin -L$(LM_DIR)/bin
LD_DLNK = -llm -lch
LD_SLNK = -lpthread -lrt

.PHONY: all clean

all: $(TARGET)
	@printf "Build complete.\n"

clean:
	$(RM) $(RM_FLAGS) $(TARGET)
	$(RM) $(RM_FLAGS) $(OBJ)
	$(RM) $(RM_FLAGS) obj/*.d
	@printf "Clean complete.\n"

$(TARGET) : $(OBJ) $(CH_DIR)/bin/$(wildcard *.so) $(LM_DIR)/bin/$(wildcard *.a)
	$(LD) $(LD_INC) $(OBJ) -o $@ -Wl,-Bstatic $(LD_DLNK) -Wl,-Bdynamic -libverbs $(LD_SLNK)

obj/%.o : %.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_INC) $< -o $@

