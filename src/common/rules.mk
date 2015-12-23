.PHONY: $(CMMN_CLEAN)

include $(wildcard $(CMMN_OBJ_DIR)/*.d)

$(CMMN_CLEAN):
	$(RM) $(RM_FLAGS) $(P_CMMN_OBJ) $(CMMN_OBJ_DIR)/*.d

$(CMMN_OBJ_DIR)/%.o : $(CMMN_DIR)/%.cpp $(CMMN_DIR)/%.h
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_ADD_FLAGS) $(CMMN_CPP_INC_FLAGS) $< -o $@

$(CMMN_OBJ_DIR)/%.o : $(CMMN_DIR)/%.cpp
	$(CPP) $(CPP_FLAGS) $(CXX_SHARED_FLAGS) $(CPP_ADD_FLAGS) $(CMMN_CPP_INC_FLAGS) $< -o $@

