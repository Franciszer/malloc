# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: francisco <francisco@student.42.fr>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/22 12:49:06 by francisco         #+#    #+#              #
#    Updated: 2025/09/29 03:42:04 by francisco        ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Makefile — portable build for libft_malloc on Linux & macOS
# Builds a shared library: libft_malloc_$(HOSTTYPE).so
# Creates a symlink:      libft_malloc.so -> libft_malloc_$(HOSTTYPE).so
# Objects & deps mirrored under build/ (e.g., lib/x/y.c -> build/x/y.o, build/x/y.d)

# ------------------------------- host & toolchain -------------------------------

CC       ?= cc
UNAME_S   := $(shell uname -s)

# Subject requirement: define HOSTTYPE if empty
ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

TARGET    := libft_malloc_$(HOSTTYPE).so
SYMLINK   := libft_malloc.so

# ------------------------------- sources & objects ------------------------------

# All C under lib/
ALL_LIB_SRCS := $(shell find lib -name '*.c')
# Unit tests live next to sources and end with "_test.c"
TEST_SRCS    := $(shell find lib -name '*_test.c')
# Library sources exclude tests
SRCS         := $(filter-out $(TEST_SRCS), $(ALL_LIB_SRCS))

# Map lib/%.c -> build/%.o
OBJS         := $(patsubst lib/%.c,build/%.o,$(SRCS))
DEPS         := $(OBJS:.o=.d)

# Exported API sources (exclude from unit-test link to avoid symbol conflicts)
EXPORTED_SRCS ?= lib/malloc.c
# Objects to link into tests (library code minus exported API)
TEST_CORE_SRCS := $(filter-out $(EXPORTED_SRCS), $(SRCS))
TEST_CORE_OBJS := $(patsubst lib/%.c,build/%.o,$(TEST_CORE_SRCS))

# Unit test executables: lib/foo/bar_test.c -> build/tests/foo/bar_test.test
TEST_BINS := $(patsubst lib/%.c,build/tests/%.test,$(TEST_SRCS))

# Third-party (munit)
THIRD_PARTY_SRCS := third_party/munit.c
THIRD_PARTY_OBJS := $(patsubst %.c,build/%.o,$(THIRD_PARTY_SRCS))

# ------------------------------- flags -----------------------------------------


CFLAGS := -std=c11 -fPIC -Wall -Wextra -Ilib -MMD -MP
LDFLAGS ?=
LDLIBS  ?=

ifeq ($(UNAME_S),Linux)
  CFLAGS += -D_DEFAULT_SOURCE -D_GNU_SOURCE
endif

ifeq ($(UNAME_S),Darwin)
  CFLAGS += -D_DARWIN_C_SOURCE
endif

# Hide everything by default; export only API via __attribute__((visibility("default")))
CFLAGS += -fvisibility=hidden

# ------------------------------- platform link mode -----------------------------

ifeq ($(UNAME_S),Linux)
	SHARED_FLAG := -shared
	SONAME_FLAG := -Wl,-soname,libft_malloc.so
else ifeq ($(UNAME_S),Darwin)
  SHARED_FLAG       := -dynamiclib
  INSTALL_NAME_FLAG := -Wl,-install_name,@rpath/$(TARGET)
else
  SHARED_FLAG       := -shared
  SONAME_FLAG       :=
endif

# ------------------------------- targets ---------------------------------------

.PHONY: all clean fclean symlink re test unit_test

all: $(TARGET) symlink

-include $(DEPS)


$(TARGET): $(OBJS)
	$(CC) $(SHARED_FLAG) $(SONAME_FLAG) $(INSTALL_NAME_FLAG) \
	  $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)


# Symlink that the subject expects
symlink: $(TARGET)
	@ln -sf $(TARGET) $(SYMLINK)

# Pattern rule: compile lib/%.c -> build/%.o (with deps alongside)
build/%.o: lib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Third-party objects (munit)
build/third_party/%.o: third_party/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Ithird_party -c $< -o $@

# ------------------------------- unit tests ------------------------------------

# Build each *_test.c into its own executable, link with: munit + core objects
build/tests/%.test: lib/%.c $(TEST_CORE_OBJS) $(THIRD_PARTY_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Ithird_party $< $(TEST_CORE_OBJS) $(THIRD_PARTY_OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# Run all unit tests
test unit_test: $(TEST_BINS)
	@set -e; \
	if [ -z "$(TEST_BINS)" ]; then \
	  echo "No unit tests found (looking for *_test.c under lib/)"; \
	else \
	  for t in $(TEST_BINS); do echo "🏃 $$t"; "$$t"; done; \
	fi

# ---- integration tests (itests): all tests link to our lib + run with LD_PRELOAD

ABS_TARGET := $(abspath $(TARGET))

ifeq ($(UNAME_S),Darwin)
  PRELOAD_ENV := DYLD_INSERT_LIBRARIES="$(ABS_TARGET)" DYLD_FORCE_FLAT_NAMESPACE=1
  RPATH_FLAG  := -Wl,-rpath,@loader_path
else
  PRELOAD_ENV := LD_PRELOAD="$(ABS_TARGET)"
  RPATH_FLAG  := -Wl,-rpath,'$$ORIGIN'
endif

ITEST_DIR  := build/itests
ITEST_SRCS := $(wildcard itests/*.c)
ITEST_BINS := $(patsubst itests/%.c,$(ITEST_DIR)/%.custom,$(ITEST_SRCS))

$(ITEST_DIR):
	@mkdir -p $(ITEST_DIR)

# Normal itests: link to our lib and bake in an rpath that finds it in:
#   build/itests (ORIGIN), build (..), and repo root (../..)
$(ITEST_DIR)/%.custom: itests/%.c | $(ITEST_DIR) $(TARGET) symlink
	$(CC) $(CFLAGS) -Ilib -I. $< -o $@ \
	  -L. -lft_malloc -Wl,-rpath,'$$ORIGIN:$$ORIGIN/..:$$ORIGIN/../..' \
	  $(LDFLAGS) $(LDLIBS)
	  
.PHONY: itest
itest: $(TARGET) symlink $(ITEST_BINS)
	@set -e; \
	if [ -z "$(ITEST_SRCS)" ]; then \
	  echo "No itests found (itests/*.c)."; exit 0; \
	fi; \
	for t in $(ITEST_BINS); do \
	  echo "▶︎ $$t (custom malloc via LD_PRELOAD)"; \
	  env $(PRELOAD_ENV) "$$t"; \
	done

clean_itests:
	$(RM) -r $(ITEST_DIR)


##### Bench (subject) tests — isolated #####

BENCH_CC              ?= $(CC)
BENCH_CFLAGS          ?= -Wall -Wextra
BENCH_TIME            ?= /usr/bin/time -v
BENCH_PYTHON          ?= python3
BENCH_RUNNER          ?= scripts/run_linux.sh
BENCH_SCRIPT          ?= scripts/bench_malloc.py
BENCH_OUT_CSV         ?= bench.csv

BENCH_DIR             ?= bench_tests
BENCH_SRC_DIR         ?= $(BENCH_DIR)/src
BENCH_BIN_DIR         ?= $(BENCH_DIR)/bin

BENCH_SRCS            := $(wildcard $(BENCH_SRC_DIR)/test[0-5].c)
BENCH_BINS            := $(patsubst $(BENCH_SRC_DIR)/%.c,$(BENCH_BIN_DIR)/%,$(BENCH_SRCS))

.PHONY: bench_tests
bench_tests: libft_malloc_build $(BENCH_BIN_DIR) $(BENCH_BINS) $(BENCH_RUNNER) $(BENCH_SCRIPT)
	@ln -sf libft_malloc_$(HOSTTYPE).so libft_malloc.so || true
	@rm -f $(BENCH_OUT_CSV)
	@chmod +x $(BENCH_RUNNER)
	$(BENCH_PYTHON) $(BENCH_SCRIPT) \
	  --time "$(BENCH_TIME)" \
	  --runner-custom "$(BENCH_RUNNER)" \
	  --out "$(BENCH_OUT_CSV)" \
	  $(BENCH_BINS)
	@echo "Wrote $(BENCH_OUT_CSV)"

.PHONY: bench_tests_quick
bench_tests_quick: libft_malloc_build $(BENCH_BIN_DIR) $(BENCH_RUNNER) $(BENCH_SCRIPT)
	@bins=""
	@for t in test0 test1 test2; do \
	  if [ -f "$(BENCH_SRC_DIR)/$$t.c" ]; then \
	    $(MAKE) -s $(BENCH_BIN_DIR)/$$t; \
	    bins="$$bins $(BENCH_BIN_DIR)/$$t"; \
	  fi; \
	done; \
	if [ -z "$$bins" ]; then \
	  echo "❌ No test0..2 sources found in $(BENCH_SRC_DIR)"; exit 1; \
	fi; \
	chmod +x $(BENCH_RUNNER); \
	echo "➡️  Running: $$bins"; \
	$(BENCH_PYTHON) $(BENCH_SCRIPT) --time "$(BENCH_TIME)" --runner-custom "$(BENCH_RUNNER)" --out "$(BENCH_OUT_CSV)" $$bins; \
	echo "✅ Wrote $(BENCH_OUT_CSV)"

.PHONY: libft_malloc_build
libft_malloc_build:
	@$(MAKE) -s re

$(BENCH_BIN_DIR):
	@mkdir -p $(BENCH_BIN_DIR)

# Compile exactly like the subject
$(BENCH_BIN_DIR)/test0: $(BENCH_SRC_DIR)/test0.c | $(BENCH_BIN_DIR)
	$(BENCH_CC) $(BENCH_CFLAGS) -o $@ $<
$(BENCH_BIN_DIR)/test1: $(BENCH_SRC_DIR)/test1.c | $(BENCH_BIN_DIR)
	$(BENCH_CC) $(BENCH_CFLAGS) -o $@ $<
$(BENCH_BIN_DIR)/test2: $(BENCH_SRC_DIR)/test2.c | $(BENCH_BIN_DIR)
	$(BENCH_CC) $(BENCH_CFLAGS) -o $@ $<

$(BENCH_BIN_DIR)/test3: $(BENCH_SRC_DIR)/test3.c | $(BENCH_BIN_DIR)
	$(BENCH_CC) $(BENCH_CFLAGS) -o $@ $< -L. -lft_malloc
$(BENCH_BIN_DIR)/test4: $(BENCH_SRC_DIR)/test4.c | $(BENCH_BIN_DIR)
	$(BENCH_CC) $(BENCH_CFLAGS) -o $@ $< -L. -lft_malloc
$(BENCH_BIN_DIR)/test5: $(BENCH_SRC_DIR)/test5.c | $(BENCH_BIN_DIR)
	$(BENCH_CC) $(BENCH_CFLAGS) -o $@ $< -L. -lft_malloc

.PHONY: bench_tests_clean

bench_tests_clean:
	@rm -f $(BENCH_BIN_DIR)/test0 $(BENCH_BIN_DIR)/test1 $(BENCH_BIN_DIR)/test2 \
	        $(BENCH_BIN_DIR)/test3 $(BENCH_BIN_DIR)/test4 $(BENCH_BIN_DIR)/test5 \
	        $(BENCH_OUT_CSV)

$(BENCH_RUNNER):
	@echo "❌ Missing $(BENCH_RUNNER). Put the subject's run_linux.sh in scripts/ and chmod +x." >&2
	@false
$(BENCH_SCRIPT):
	@echo "❌ Missing $(BENCH_SCRIPT). Add your Python bench script there." >&2
	@false

# ---- clean ----
.PHONY: clean_bench_tests
clean_bench_tests:
	@rm -f $(BIN_DIR)/test0 $(BIN_DIR)/test1 $(BIN_DIR)/test2 \
	       $(BIN_DIR)/test3 $(BIN_DIR)/test4 $(BIN_DIR)/test5 \
	       $(OUT_CSV)

# Friendly error if the wrapper is missing
$(RUN_CUSTOM):
	@echo "ERROR: $(RUN_CUSTOM) not found. Put the subject's run_linux.sh in scripts/ and chmod +x." >&2
	@false
# ------------------------------- cleaning --------------------------------------

clean: clean_itests clean_bench_tests
	$(RM) $(TARGET) $(SYMLINK)

fclean: clean clean-bootstrap
	$(RM) -r build

re: fclean all

# ------------------------------- formatting ------------------------------------

CLANG_FORMAT ?= clang-format
TOOLS_DIR ?= tools
LOCAL_CLANG_FORMAT := $(TOOLS_DIR)/clang-format
# Prefer repo-local formatter if present
ifneq ("$(wildcard $(LOCAL_CLANG_FORMAT))","")
  CLANG_FORMAT := $(LOCAL_CLANG_FORMAT)
endif

# Files to format: all C/H under lib/ (skip third_party)
FMT_FILES := $(shell find lib -type f \( -name '*.c' -o -name '*.h' \))
FMT_VERBOSE ?= 1  # set to 1 for a file-by-file list

.PHONY: fmt format fmt-check format-check bootstrap-clang-format

define _fmt_header_write
@echo "== clang-format (write) =="
@echo "Files: $(words $(FMT_FILES))"
endef

define _fmt_header_check
@echo "== clang-format (check) =="
@echo "Files: $(words $(FMT_FILES))"
endef

# Write changes in-place
fmt format:
	@{ command -v $(CLANG_FORMAT) >/dev/null 2>&1 || [ -x "$(LOCAL_CLANG_FORMAT)" ]; } || { \
	  echo "error: clang-format not found. Run 'make bootstrap-clang-format' to install a local copy."; exit 127; }
	$(_fmt_header_write)
ifneq ($(FMT_VERBOSE),0)
	@printf '  %s\n' $(FMT_FILES)
endif
	@$(CLANG_FORMAT) -i $(FMT_FILES)

# Check only (CI-friendly): nonzero exit if formatting is needed
fmt-check format-check:
	@{ command -v $(CLANG_FORMAT) >/dev/null 2>&1 || [ -x "$(LOCAL_CLANG_FORMAT)" ]; } || { \
	  echo "error: clang-format not found. Run 'make bootstrap-clang-format' to install a local copy."; exit 127; }
	$(_fmt_header_check)
ifneq ($(FMT_VERBOSE),0)
	@printf '  %s\n' $(FMT_FILES)
endif
	@$(CLANG_FORMAT) -n --Werror $(FMT_FILES) \
	  || { echo "✗ Formatting needed. Run 'make fmt'."; exit 1; }
	@echo "Formatting OK ✓"

# Bootstrap a local clang-format with npm (no sudo)
bootstrap-clang-format:
	@mkdir -p $(TOOLS_DIR)
	@if command -v $(CLANG_FORMAT) >/dev/null 2>&1; then \
	  echo "clang-format already available: $$($(CLANG_FORMAT) --version)"; \
	else \
	  echo "Installing clang-format locally via npm…"; \
	  npm init -y >/dev/null 2>&1 || true; \
	  npm install --no-save clang-format >/dev/null || { echo "npm install failed"; exit 1; }; \
	  BIN="node_modules/.bin/clang-format"; \
	  if [ -x "$$BIN" ]; then \
	    ln -sf ../$$BIN $(TOOLS_DIR)/clang-format; \
	    echo "Installed local formatter at $(TOOLS_DIR)/clang-format"; \
	    $(TOOLS_DIR)/clang-format --version; \
	  else \
	    echo "npm installed but $$BIN not found (check npm logs)."; \
	    exit 1; \
	  fi; \
	fi

clean-clang-format:
	rm -rf package.json tools
# ---- IntelliSense bootstrap (VS Code) ----
.PHONY: bootstrap-intellisense bootstrap clean-intellisense clean-bootstrap

# Detect IntelliSense mode + compiler
ifeq ($(UNAME_S),Darwin)
  INTELLI_MODE := $(if $(findstring arm64,$(HOSTTYPE)),macos-clang-arm64,macos-clang-x64)
  INTELLI_COMP := /usr/bin/clang
else ifeq ($(UNAME_S),Linux)
  INTELLI_MODE := $(if $(findstring aarch64,$(HOSTTYPE)),linux-clang-arm64,linux-clang-x64)
  INTELLI_COMP := $(shell command -v clang 2>/dev/null || echo /usr/bin/clang)
else
  INTELLI_MODE := linux-clang-x64
  INTELLI_COMP := $(shell command -v clang 2>/dev/null || echo /usr/bin/clang)
endif

# Intellisense
bootstrap-intellisense:
	@mkdir -p .vscode
	@echo "Writing .vscode/c_cpp_properties.json for $(UNAME_S) ($(HOSTTYPE))"
	@{ \
	  printf '{\n'; \
	  printf '  "configurations": [\n'; \
	  printf '    {\n'; \
	  printf '      "name": "%s-%s",\n' "$(UNAME_S)" "$(HOSTTYPE)"; \
	  printf '      "compilerPath": "%s",\n' "$(INTELLI_COMP)"; \
	  printf '      "intelliSenseMode": "%s",\n' "$(INTELLI_MODE)"; \
	  printf '      "cStandard": "c11",\n'; \
	  printf '      "cppStandard": "c++17",\n'; \
	  printf '      "includePath": [\n'; \
	  printf '        "$${workspaceFolder}/lib",\n'; \
	  printf '        "$${workspaceFolder}/lib/**",\n'; \
	  printf '        "$${workspaceFolder}/third_party"\n'; \
	  printf '      ],\n'; \
	  printf '      "defines": []\n'; \
	  printf '    }\n'; \
	  printf '  ],\n'; \
	  printf '  "version": 4\n'; \
	  printf '}\n'; \
	} > .vscode/c_cpp_properties.json
	@# Optionally prefer Makefile Tools to supply flags (works even without it installed)
	@printf '{\n  "C_Cpp.default.configurationProvider": "ms-vscode.makefile-tools",\n  "C_Cpp.default.intelliSenseMode": "%s"\n}\n' "$(INTELLI_MODE)" > .vscode/settings.json
	@$(call _emit_compile_commands_json)
	@echo "VS Code IntelliSense bootstrapped ✓"

# Aggregate bootstrap
bootstrap: bootstrap-clang-format bootstrap-intellisense
	@echo "Bootstrap complete ✓"

# Clean only the IntelliSense artifacts produced by bootstrap-intellisense
clean-intellisense:
	@rm -f .vscode/c_cpp_properties.json .vscode/settings.json
	@rmdir .vscode 2>/dev/null || true

# Clean all bootstrap artifacts (extend as needed)
clean-bootstrap: clean-intellisense
	@rm -f tools/clang-format
