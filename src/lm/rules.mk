.PHONY: $(LM_CLEAN)

.SECONDARY:

include $(wildcard $(LM_OBJ_DIR)/*.d)

$(LM_CLEAN):
	$(RM) $(RM_FLAGS) $(LM_TARGET)
	$(RM) $(RM_FLAGS) $(P_LM_OBJ) $(LM_OBJ_DIR)/*.d

$(LM_BIN_DIR)/%.so : $(P_LM_OBJ)
	$(CPP) $(LD_FLAGS) $(LMLD_PFLAGS) $(LD_SHARED_FLAGS) $^ -o $@ $(LMLD_SFLAGS)

$(LM_BIN_DIR)/%.a : $(P_LM_OBJ)
	$(AR) $(AR_FLAGS) $@ $^

$(LM_OBJ_DIR)/%.o : $(LM_DIR)/%.cpp $(LM_DIR)/%.h
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_INC_FLAGS) $< -o $@

$(LM_OBJ_DIR)/%.o : $(LM_DIR)/%.cpp
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_INC_FLAGS) $< -o $@

