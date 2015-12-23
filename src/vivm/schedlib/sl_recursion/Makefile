COMMON_DIR=$(shell pwd)/../../../..
STRICT_COMPILE = 1
include config.mk

.PHONY: all clean purge

all: $(SL_REC_TARGET)
	@echo Build: Scheduler

clean: $(SL_REC_CLEAN)

purge: $(SL_REC_PURGE)

include rules.mk
