TARGET    = ibqp
SRC       = ibqp.cpp     \
            ibqp_ops.cpp

OBJ       = $(addprefix obj/, $(SRC:.cpp=.o))

CPP       = g++
CPP_FLAGS = -c -g -O2 -MD
LD        = g++
LD_PFLAGS = 
LD_SFLAGS = -libverbs

.PHONY: all clean

all: $(TARGET)
	@echo Build complete

$(TARGET) : $(OBJ)
	$(LD) $(LD_PFLAGS) $^ -o $@ $(LD_SFLAGS)

obj/%.o : %.cpp
	$(CPP) $(CPP_FLAGS) $< -o $@

include $(wildcard obj/*.d)

clean:
	rm -f $(TARGET)
	rm -f $(OBJ)
	rm -f obj/*.d
	@echo Clean complete

