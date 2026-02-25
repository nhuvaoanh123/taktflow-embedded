# =============================================================================
# Integration Test Makefile — ASPICE SWE.5
#
# Builds and runs integration tests that link multiple BSW modules together.
# Tests verify inter-module interfaces, safety paths, and communication chains.
#
# Usage:
#   make test                           Run all integration tests
#   make test-e2e_chain                 Run a single integration test
#   make coverage                       Run with coverage instrumentation
#   make clean                          Remove build artifacts
#
# @aspice SWE.5 — Software Component Verification & Integration
# @iso    ISO 26262 Part 6, Section 10
# =============================================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -std=c99 -pedantic -g
CFLAGS += -DUNIT_TEST -DINTEGRATION_TEST

# Coverage flags (set by coverage target)
COV_CFLAGS  =
COV_LDFLAGS =

# --- Directories ---
FW_DIR      = ../../firmware
BSW_DIR     = $(FW_DIR)/shared/bsw
MCAL_DIR    = $(BSW_DIR)/mcal
ECUAL_DIR   = $(BSW_DIR)/ecual
SERVICES_DIR = $(BSW_DIR)/services
RTE_DIR     = $(BSW_DIR)/rte
UNITY_DIR   = $(BSW_DIR)/test/unity
BUILD_DIR   = build

# --- Include paths ---
INCLUDES  = -I$(BSW_DIR)/include
INCLUDES += -I$(MCAL_DIR)
INCLUDES += -I$(ECUAL_DIR)
INCLUDES += -I$(SERVICES_DIR)
INCLUDES += -I$(RTE_DIR)
INCLUDES += -I$(UNITY_DIR)
INCLUDES += -I$(FW_DIR)/cvc/include
INCLUDES += -I$(FW_DIR)/fzc/include
INCLUDES += -I$(FW_DIR)/rzc/include
INCLUDES += -I.

# --- Unity source ---
UNITY_SRC = $(UNITY_DIR)/unity.c

# --- BSW Source Groups ---
# Communication stack: Can -> CanIf -> PduR -> Com
BSW_COM_STACK = $(ECUAL_DIR)/CanIf.c $(ECUAL_DIR)/PduR.c $(SERVICES_DIR)/Com.c

# Safety services
BSW_SAFETY = $(SERVICES_DIR)/WdgM.c $(SERVICES_DIR)/BswM.c $(SERVICES_DIR)/Dem.c

# Diagnostics
BSW_DIAG = $(SERVICES_DIR)/Dcm.c $(SERVICES_DIR)/Dem.c

# E2E protection
BSW_E2E = $(SERVICES_DIR)/E2E.c

# RTE
BSW_RTE = $(RTE_DIR)/Rte.c

# --- Test source files ---
TEST_SRCS = $(wildcard test_int_*.c)
TEST_BINS = $(patsubst %.c,$(BUILD_DIR)/%,$(TEST_SRCS))

# =============================================================================
# Targets
# =============================================================================

.PHONY: test clean coverage

test: $(TEST_BINS)
	@echo "=== Running all integration tests ==="
	@passed=0; failed=0; total=0; \
	for t in $(TEST_BINS); do \
		total=$$((total + 1)); \
		echo "--- Running $$t ---"; \
		if ./$$t; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "=== Results: $$passed/$$total passed, $$failed failed ==="; \
	if [ $$failed -ne 0 ]; then exit 1; fi

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) coverage

# --- Individual test targets (convenience) ---
test-%: $(BUILD_DIR)/test_int_%
	./$(BUILD_DIR)/test_int_$*

# =============================================================================
# Integration Test Build Rules
#
# Each test links a specific set of BSW modules together. Only the
# hardware layer (Can_Hw_*, Dio_*, Adc_*) is mocked in the test file.
# =============================================================================

# INT-003: E2E protect -> transmit -> receive -> check
$(BUILD_DIR)/test_int_e2e_chain: test_int_e2e_chain.c $(BSW_E2E) $(BSW_COM_STACK) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-004: DEM error report -> DCM DTC readout
$(BUILD_DIR)/test_int_dem_to_dcm: test_int_dem_to_dcm.c $(BSW_DIAG) $(ECUAL_DIR)/PduR.c $(ECUAL_DIR)/CanIf.c $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-005: WdgM checkpoint -> deadline violation -> BswM safe state
$(BUILD_DIR)/test_int_wdgm_supervision: test_int_wdgm_supervision.c $(BSW_SAFETY) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-006: BswM mode transitions propagate to all dependent modules
$(BUILD_DIR)/test_int_bswm_mode: test_int_bswm_mode.c $(SERVICES_DIR)/BswM.c $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-007: Critical fault -> all actuators to safe state
$(BUILD_DIR)/test_int_safe_state: test_int_safe_state.c $(BSW_SAFETY) $(BSW_RTE) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-008: ECU heartbeat timeout -> degraded mode
$(BUILD_DIR)/test_int_heartbeat_loss: test_int_heartbeat_loss.c $(BSW_SAFETY) $(BSW_RTE) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-010: Current sensor -> threshold -> motor shutdown -> DEM
$(BUILD_DIR)/test_int_overcurrent_chain: test_int_overcurrent_chain.c $(BSW_SAFETY) $(BSW_RTE) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-011: CAN message matrix verification
$(BUILD_DIR)/test_int_can_matrix: test_int_can_matrix.c $(BSW_COM_STACK) $(BSW_E2E) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-012: Signal routing full stack
$(BUILD_DIR)/test_int_signal_routing: test_int_signal_routing.c $(BSW_COM_STACK) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-013/014: E2E CRC corruption and sequence gap detection
$(BUILD_DIR)/test_int_e2e_faults: test_int_e2e_faults.c $(BSW_E2E) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# INT-015: CAN bus-off recovery
$(BUILD_DIR)/test_int_can_busoff: test_int_can_busoff.c $(BSW_COM_STACK) $(UNITY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COV_CFLAGS) $(INCLUDES) $^ -o $@ $(COV_LDFLAGS)

# =============================================================================
# Coverage
# =============================================================================

coverage: COV_CFLAGS  = -fprofile-arcs -ftest-coverage
coverage: COV_LDFLAGS = -lgcov --coverage
coverage: clean $(TEST_BINS)
	@echo "=== Running integration tests with coverage ==="
	@passed=0; failed=0; total=0; \
	for t in $(TEST_BINS); do \
		total=$$((total + 1)); \
		echo "--- Running $$t ---"; \
		if ./$$t; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "=== Results: $$passed/$$total passed, $$failed failed ==="; \
	if [ $$failed -ne 0 ]; then exit 1; fi
	@echo "=== Collecting integration test coverage ==="
	mkdir -p coverage
	lcov --capture --directory $(BUILD_DIR) --output-file coverage/int.info \
		--rc branch_coverage=1 --ignore-errors mismatch
	lcov --remove coverage/int.info '*/test/*' \
		--output-file coverage/int-filtered.info --rc branch_coverage=1 \
		--ignore-errors unused
	genhtml coverage/int-filtered.info --output-directory coverage/html \
		--branch-coverage --title "Integration Test Coverage"
	@echo "=== Integration Coverage Summary ==="
	lcov --summary coverage/int-filtered.info --rc branch_coverage=1
