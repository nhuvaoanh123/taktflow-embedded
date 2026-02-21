# =============================================================================
# BSW Unit Test Makefile -- runs on host (x86/x64)
#
# Usage:
#   make test           - Build and run all unit tests
#   make test-Can       - Build and run a specific test (e.g., test_Can.c)
#   make clean          - Remove build artifacts
#
# Convention:
#   Test file:   test/<test_name>.c     (e.g., test/test_Can.c)
#   Source file:  <layer>/<module>.c     (e.g., mcal/Can.c)
#   The Makefile auto-discovers source files across BSW layers.
# =============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic -g
CFLAGS += -DUNIT_TEST
CFLAGS += -Iinclude -Itest/unity

# Source directories
MCAL_DIR = mcal
ECUAL_DIR = ecual
SERVICES_DIR = services
RTE_DIR = rte
TEST_DIR = test
UNITY_DIR = test/unity

# Unity source
UNITY_SRC = $(UNITY_DIR)/unity.c

# Find all test files
TEST_SRCS = $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(TEST_DIR)/build/%)

# Default: run all tests
.PHONY: test clean

test: $(TEST_BINS)
	@echo "=== Running all BSW unit tests ==="
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

# Build directory
$(TEST_DIR)/build:
	mkdir -p $(TEST_DIR)/build

# Pattern rule: compile test + matching source + unity
# Each test_<module>.c links with <module>.c from the appropriate BSW layer
$(TEST_DIR)/build/test_%: $(TEST_DIR)/test_%.c $(UNITY_SRC) | $(TEST_DIR)/build
	$(CC) $(CFLAGS) -I$(MCAL_DIR) -I$(ECUAL_DIR) -I$(SERVICES_DIR) -I$(RTE_DIR) -I$(TEST_DIR)/mocks \
		$< $(call find_source,$*) $(UNITY_SRC) -o $@

# Helper: find the source file for a module across BSW layers
define find_source
$(or $(wildcard $(MCAL_DIR)/$(1).c),$(wildcard $(ECUAL_DIR)/$(1).c),$(wildcard $(SERVICES_DIR)/$(1).c),$(wildcard $(RTE_DIR)/$(1).c))
endef

clean:
	rm -rf $(TEST_DIR)/build

# Individual test targets (convenience)
test-%: $(TEST_DIR)/build/test_%
	./$(TEST_DIR)/build/test_$*
