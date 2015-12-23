COMMON_DIR = $(shell pwd)/../../..
STRICT_COMPILE = 1
include config.mk

.PHONY: all clean purge

all: $(API_TARGET)
	@echo Build: API

clean: $(API_CLEAN)

purge: $(API_PURGE)

include rules.mk
