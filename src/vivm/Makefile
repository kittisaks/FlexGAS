COMMON_DIR=$(shell pwd)/../..
include ../../Makefile.common
include $(SCHED_DIR)/config.mk
include $(AGENT_DIR)/config.mk
include $(API_DIR)/config.mk
include $(SCHEDLIB_DIR)/config.mk

SL_SIMPLE_DIR = $(SCHEDLIB_DIR)/sl_simple
SL_REC_DIR    = $(SCHEDLIB_DIR)/sl_recursion
include $(SL_SIMPLE_DIR)/config.mk
include $(SL_REC_DIR)/config.mk

.PHONY: all clean purge


all:
	@$(MAKE) -C $(SCHED_DIR)
	@$(MAKE) -C $(AGENT_DIR)
	@$(MAKE) -C $(API_DIR)
	@$(MAKE) -C $(SCHEDLIB_DIR)
	@$(MAKE) -C $(SL_SIMPLE_DIR)
	@$(MAKE) -C $(SL_REC_DIR)
	@$(CP) $(SCHED_TARGET) $(VIVM_DIR)
	@$(CP) $(AGENT_TARGET) $(VIVM_DIR)
	@$(CP) $(API_TARGET) $(VIVM_DIR)
	@$(CP) $(SCHEDLIB_TARGET) $(VIVM_DIR)
	@$(CP) $(SL_SIMPLE_TARGET) $(VIVM_DIR)
	@$(CP) $(SL_REC_TARGET) $(VIVM_DIR)
	

clean:
	@$(MAKE) -C $(SCHED_DIR) clean
	@$(MAKE) -C $(AGENT_DIR) clean
	@$(MAKE) -C $(API_DIR) clean
	@$(MAKE) -C $(SCHEDLIB_DIR) clean
	@$(MAKE) -C $(SL_SIMPLE_DIR) clean
	@$(MAKE) -C $(SL_REC_DIR) clean
	@$(RM) $(RM_FLAGS) libvivm.so libvivm.a
	@$(RM) $(RM_FLAGS) vagt
	@$(RM) $(RM_FLAGS) vsched
	@$(RM) $(RM_FLAGS) schedlib.so
	@$(RM) $(RM_FLAGS) sl_simple.so
	@$(RM) $(RM_FLAGS) sl_rec.so

purge:
	@$(MAKE) -C $(SCHED_DIR) purge
	@$(MAKE) -C $(AGENT_DIR) purge
	@$(MAKE) -C $(API_DIR) purge
	@$(MAKE) -C $(SCHEDLIB_DIR) clean
	@$(MAKE) -C $(SL_SIMPLE_DIR) clean
	@$(MAKE) -C $(SL_REC_DIR) clean
	@$(RM) $(RM_FLAGS) libvivm.so libvivm.a
	@$(RM) $(RM_FLAGS) vagt
	@$(RM) $(RM_FLAGS) vsched
	@$(RM) $(RM_FLAGS) schedlib.so
	@$(RM) $(RM_FLAGS) sl_simple.so
	@$(RM) $(RM_FLAGS) sl_rec.so

