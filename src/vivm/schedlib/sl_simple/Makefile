COMMON_DIR=$(shell pwd)/../../../..
STRICT_COMPILE = 1
include config.mk

.PHONY: all clean purge

all: $(SL_SIMPLE_TARGET)
	@echo Build: Scheduler

clean: $(SL_SIMPLE_CLEAN)

purge: $(SL_SIMPLE_PURGE)

include rules.mk
