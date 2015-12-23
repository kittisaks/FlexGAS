#!/bin/bash

ls $VIVM_AGENT_SHM_PATH -al | grep vivm_agent_ | awk '{sub(/vivm_agent_/, "", $9)} {print $9}' > nodes.lst

