COMMON_DIR=$(shell pwd)/../../..
STRICT_COMPILE = 1
include config.mk

.PHONY: all clean purge

all: $(SCHEDLIB_TARGET)
	@echo Build: Scheduler

clean: $(SCHEDLIB_CLEAN)

purge: $(SCHEDLIB_PURGE)

include rules.mk
