COMMON_DIR=$(shell pwd)/../../..
STRICT_COMPILE = 1
include config.mk

.PHONY: all clean purge

all: $(SCHED_TARGET)
	@echo Build: Scheduler

clean: $(SCHED_CLEAN)

purge: $(SCHED_PURGE)

include rules.mk
