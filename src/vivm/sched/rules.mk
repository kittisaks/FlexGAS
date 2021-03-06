.PHONY: $(SCHED_CLEAN) $(SCHED_PURGE)

include $(wildcard $(SCHED_OBJ_DIR)/*.d)

$(SCHED_TARGET): $(CH_TARGET) $(P_SCHED_OBJ) $(P_CMMN_OBJ)
	$(CPP) $(LD_FLAGS) $(SCHED_LD_PFLAGS) $(P_SCHED_OBJ) $(P_CMMN_OBJ) -o $@ $(SCHED_LD_SFLAGS)

$(SCHED_CLEAN):
	$(RM) $(RM_FLAGS) $(SCHED_TARGET)
	$(RM) $(RM_FLAGS) $(P_SCHED_OBJ) $(SCHED_OBJ_DIR)/*.d

$(SCHED_PURGE): $(SCHED_CLEAN) $(CMMN_CLEAN) $(CH_CLEAN) $(LM_CLEAN)
	$(RM) $(RM_FLAGS) $(SCHED_TARGET)
	$(RM) $(RM_FLAGS) $(P_SCHED_OBJ) $(SCHED_OBJ_DIR)/*.d

$(SCHED_OBJ_DIR)/%.o : $(SCHED_DIR)/%.cpp $(SCHED_DIR)/%.h
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(SCHED_CPP_OPT) $(SCHED_CPP_INC_FLAGS) $< -o $@

$(SCHED_OBJ_DIR)/%.o : $(SCHED_DIR)/%.cpp
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(SCHED_CPP_OPT) $(SCHED_CPP_INC_FLAGS) $< -o $@

include $(CH_DIR)/rules.mk
include $(CMMN_DIR)/rules.mk

