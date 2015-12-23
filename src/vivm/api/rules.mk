.PHONY: $(API_CLEAN)

include $(wildcard $(API_OBJ_DIR)/*.d)

$(API_BIN_DIR)/%.so: $(CH_TARGET) $(P_API_OBJ)
	$(CPP) $(LD_FLAGS) $(LD_SHARED_FLAGS) $(API_LD_PFLAGS) $(P_API_OBJ) -o $@ $(API_LD_SFLAGS)

$(API_BIN_DIR)/%.a: $(P_LM_OBJ) $(P_CH_OBJ) $(P_API_OBJ)
	$(AR) $(AR_FLAGS) $@ $^

$(API_CLEAN):
	$(RM) $(RM_FLAGS) $(API_TARGET)
	$(RM) $(RM_FLAGS) $(P_API_OBJ) $(API_OBJ_DIR)/*.d

$(API_PURGE): $(SCHED_CLEAN) $(AGENT_CLEAN) $(CH_CLEAN) $(LM_CLEAN)
	$(RM) $(RM_FLAGS) $(API_TARGET)
	$(RM) $(RM_FLAGS) $(P_API_OBJ) $(API_OBJ_DIR)/*.d

$(API_OBJ_DIR)/%.o : $(API_DIR)/%.cpp $(API_DIR)/%.h
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_ADD_FLAGS) $(API_CPP_INC_FLAGS) $< -o $@

$(API_OBJ_DIR)/%.o : $(API_DIR)/%.cpp
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_ADD_FLAGS) $(API_CPP_INC_FLAGS) $< -o $@

include $(AGENT_DIR)/rules.mk
