# =============================================================================
# BSW Unit Test Makefile -- runs on host (x86/x64)
#
# Usage:
#   make test           - Build and run all unit tests
#   make test-Can       - Build and run a specific test (e.g., test_Can.c)
#   make coverage       - Build with coverage, run tests, generate lcov report
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

# Coverage flags (appended when building with coverage target)
COV_CFLAGS  =
COV_LDFLAGS =

# Default: run all tests
.PHONY: test clean coverage coverage-report

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
	$(CC) $(CFLAGS) $(COV_CFLAGS) -I$(MCAL_DIR) -I$(ECUAL_DIR) -I$(SERVICES_DIR) -I$(RTE_DIR) -I$(TEST_DIR)/mocks \
		$< $(call find_source,$*) $(UNITY_SRC) -o $@ $(COV_LDFLAGS)

# Helper: find the source file for a module across BSW layers
define find_source
$(or $(wildcard $(MCAL_DIR)/$(1).c),$(wildcard $(ECUAL_DIR)/$(1).c),$(wildcard $(SERVICES_DIR)/$(1).c),$(wildcard $(RTE_DIR)/$(1).c))
endef

clean:
	rm -rf $(TEST_DIR)/build
	rm -rf coverage

# Individual test targets (convenience)
test-%: $(TEST_DIR)/build/test_%
	./$(TEST_DIR)/build/test_$*

# =============================================================================
# Coverage targets — gcov + lcov
#
# Builds all tests with --coverage, runs them, collects coverage data with
# lcov, and generates an HTML report. Measures statement, branch, and
# function coverage per IEC 61508 / ISO 26262 requirements.
#
# Prerequisites: gcc, gcov, lcov, genhtml
# =============================================================================

# Coverage build: rebuild tests with --coverage, run, collect
coverage: COV_CFLAGS  = -fprofile-arcs -ftest-coverage
coverage: COV_LDFLAGS = -lgcov --coverage
coverage: clean $(TEST_BINS)
	@echo "=== Running BSW tests with coverage ==="
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
	@echo "=== Collecting coverage data ==="
	mkdir -p coverage
	lcov --capture --directory $(TEST_DIR)/build --output-file coverage/bsw.info \
		--rc branch_coverage=1 --ignore-errors mismatch
	lcov --remove coverage/bsw.info '*/test/*' '/usr/*' \
		--output-file coverage/bsw-filtered.info --rc branch_coverage=1
	@echo "=== Generating HTML report ==="
	genhtml coverage/bsw-filtered.info --output-directory coverage/html \
		--branch-coverage --title "BSW Unit Test Coverage"
	@echo "=== Coverage Summary ==="
	lcov --summary coverage/bsw-filtered.info --rc branch_coverage=1
	@echo "=== HTML report: coverage/html/index.html ==="

# Text-only summary (for CI without genhtml)
coverage-report: COV_CFLAGS  = -fprofile-arcs -ftest-coverage
coverage-report: COV_LDFLAGS = -lgcov --coverage
coverage-report: clean $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || true; done
	mkdir -p coverage
	lcov --capture --directory $(TEST_DIR)/build --output-file coverage/bsw.info \
		--rc branch_coverage=1 --ignore-errors mismatch
	lcov --remove coverage/bsw.info '*/test/*' '/usr/*' \
		--output-file coverage/bsw-filtered.info --rc branch_coverage=1
	lcov --summary coverage/bsw-filtered.info --rc branch_coverage=1
