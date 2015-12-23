COMMON_DIR=$(shell pwd)/../../..
STRICT_COMPILE = 1
include config.mk

.PHONY: all clean purge

all: $(AGENT_TARGET)
	@echo Build: Agent

clean: $(AGENT_CLEAN)

purge: $(AGENT_PURGE)

include rules.mk
