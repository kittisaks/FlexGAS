.PHONY: $(CH_CLEAN)

include $(wildcard $(CH_OBJ_DIR)/*.d)

$(CH_BIN_DIR)/%.so: $(LM_TARGET) $(P_CH_OBJ)
	$(CPP) $(LD_FLAGS) $(LD_SHARED_FLAGS) $(CHLD_PFLAGS) $(P_CH_OBJ) -o $@ $(CHLD_SFLAGS)

$(CH_BIN_DIR)/%.a : $(P_LM_OBJ) $(P_CH_OBJ)
	$(AR) $(AR_FLAGS) $@ $^

$(CH_CLEAN):
	$(RM) $(RM_FLAGS) $(CH_TARGET)
	$(RM) $(RM_FLAGS) $(P_CH_OBJ) $(CH_OBJ_DIR)/*.d

$(CH_PURGE): $(LM_CLEAN)
	$(RM) $(RM_FLAGS) $(CH_TARGET)
	$(RM) $(RM_FLAGS) $(P_CH_OBJ) $(CH_OBJ_DIR)/*.d

$(CH_OBJ_DIR)/%.o : $(CH_DIR)/%.cpp $(LM_DIR)/%.h
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_ADD_FLAGS) $(CPP_INC_FLAGS) $< -o $@

$(CH_OBJ_DIR)/%.o : $(CH_DIR)/%.cpp
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_ADD_FLAGS) $(CPP_INC_FLAGS) $< -o $@

include $(LM_DIR)/rules.mk
