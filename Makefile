# Makefile for libjcccol - COL Standard Library
#
# Builds libjcccol.a static library for use with the JCC compiler
# Supports macOS, Linux, and Windows (using Clang on all platforms)

# Compiler and tools. We use LLVM Clang by default on all platforms.
# Override from the command line (e.g., `make CC=gcc`) or environment.
#
# The `origin` check is required for CC because make pre-defines CC=cc as a
# built-in implicit variable; a plain `CC ?= clang` would be a no-op and we'd
# silently end up with gcc on Linux. The same applies to AR.
ifeq ($(origin CC),default)
    CC := clang
endif
ifeq ($(origin AR),default)
    AR := ar
endif
RANLIB ?= ranlib

# Directories
SRC_DIR := src
INCLUDE_DIR := include
OBJ_DIR := obj
BUILD_DIR := build
TEST_DIR := tests

# Output
LIB_PATH := $(BUILD_DIR)/libjcccol.a

# Version (single source of truth: the VERSION file at the repo root).
# Bumped by scripts/release.sh as part of `make release`.
VERSION := $(shell cat VERSION 2>/dev/null || echo 0.0.0)

# Source files
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Compiler flags. -MMD -MP emits .d files alongside each .o as a side-effect
# of compilation, so dependency tracking stays in lock-step with the build
# without a separate rule.
CFLAGS := -Wall -Wextra -Werror -std=c11 -O2 -I$(INCLUDE_DIR) -MMD -MP
LDFLAGS :=
ARFLAGS := rcs

# Executable suffix. Empty on POSIX; .exe on Windows (MSYS2/MinGW or native
# OS=Windows_NT). Baked into the test-binary pattern rule below so make
# matches `build/test_core.exe` against `tests/test_core.c` without
# auto-appending a second .exe.
EXE :=
UNAME_S := $(shell uname -s)
ifneq (,$(findstring MINGW,$(UNAME_S)))
    EXE := .exe
endif
ifeq ($(OS),Windows_NT)
    EXE := .exe
endif

# Test files. Test objects live under obj/tests/ so a hypothetical
# tests/core.c would not collide with src/core.c at obj/core.o.
TEST_OBJ_DIR := $(OBJ_DIR)/tests
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.c,$(TEST_OBJ_DIR)/%.o,$(TEST_SOURCES))
TEST_BINARIES := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%$(EXE),$(TEST_SOURCES))

# Dist target inputs (override from command line):
#   PLATFORM=<name>   — used in archive filename and stage directory,
#                       e.g. macos-arm64, linux-x86_64, windows-x86_64.
#   ARCHIVE=<format>  — "tar.gz" (default) or "zip".
DIST_DIR := dist
# Normalize uname -s to the names the GitHub Actions matrix uses
# (macos/linux/windows), so a local `make dist` matches the artifact names
# CI produces.
DIST_OS := $(shell uname -s | tr A-Z a-z)
ifeq ($(DIST_OS),darwin)
    DIST_OS := macos
endif
ifneq (,$(findstring mingw,$(DIST_OS)))
    DIST_OS := windows
endif
ifeq ($(OS),Windows_NT)
    DIST_OS := windows
endif
PLATFORM ?= $(DIST_OS)-$(shell uname -m)
ARCHIVE ?= tar.gz
# Archives are named libjcccol-<version>-<platform>.<ext> to match the
# convention JCC's Maven build expects (see docs/ARCHITECTURE.md).
DIST_NAME := libjcccol-$(VERSION)-$(PLATFORM)
DIST_STAGE := $(DIST_DIR)/$(DIST_NAME)

# Phony targets
.PHONY: all clean test help dirs release version dist lint

# Default target
all: dirs $(LIB_PATH)

# Help target
help:
	@echo "libjcccol - COL Standard Library Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build the library (default)"
	@echo "  test       - Build and run all tests"
	@echo "  lint       - Lint workflows (actionlint) and scripts (shellcheck) if installed"
	@echo "  dist       - Stage and archive a release bundle (PLATFORM=… ARCHIVE=tar.gz|zip)"
	@echo "  clean      - Remove all build artifacts"
	@echo "  version    - Print the current library version"
	@echo "  release    - Cut a release (requires NEW_VERSION=X.Y.Z; macOS/Linux only)"
	@echo "  help       - Show this help message"

# Create necessary directories
dirs:
	@mkdir -p $(OBJ_DIR) $(TEST_OBJ_DIR) $(BUILD_DIR)

# Build the static library
$(LIB_PATH): $(OBJECTS)
	@echo "Creating static library: $@"
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test source files to object files. Kept separate from the link
# step so a test can grow to multiple translation units (just add the extra
# .o to the test binary's prerequisites).
$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Link test executables. $(filter %.o,$^) lets additional helper objects
# join the link without picking up $(LIB_PATH) as an input file.
$(BUILD_DIR)/%$(EXE): $(TEST_OBJ_DIR)/%.o $(LIB_PATH)
	@echo "Linking test: $@"
	$(CC) $(CFLAGS) $(filter %.o,$^) -L$(BUILD_DIR) -ljcccol -o $@ $(LDFLAGS)

# Build and run tests
test: dirs $(LIB_PATH) $(TEST_BINARIES)
	@echo ""
	@echo "Running tests..."
	@echo "================"
	@for test in $(TEST_BINARIES); do \
		echo ""; \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR) $(DIST_DIR)

# Stage and archive a release bundle. Used by .github/workflows to package
# per-platform tarballs/zips, so the layout is defined here (one place)
# rather than duplicated across workflow YAML.
#
# Layout produced under $(DIST_STAGE):
#   lib/libjcccol.a
#   include/jcccol.h
#   include/jcccol/*.h
#   README.md
#   LICENSE
dist: dirs lint $(LIB_PATH)
	@echo "Staging dist bundle: $(DIST_STAGE) (ARCHIVE=$(ARCHIVE))"
	rm -rf $(DIST_STAGE) $(DIST_STAGE).tar.gz $(DIST_STAGE).zip
	mkdir -p $(DIST_STAGE)/lib $(DIST_STAGE)/include/jcccol
	cp $(LIB_PATH) $(DIST_STAGE)/lib/
	cp $(INCLUDE_DIR)/jcccol.h $(DIST_STAGE)/include/
	cp $(INCLUDE_DIR)/jcccol/*.h $(DIST_STAGE)/include/jcccol/
	cp README.md LICENSE $(DIST_STAGE)/
ifeq ($(ARCHIVE),zip)
	cd $(DIST_DIR) && zip -r $(DIST_NAME).zip $(DIST_NAME)
else
	tar -czf $(DIST_STAGE).tar.gz -C $(DIST_DIR) $(DIST_NAME)
endif

# Optional lint pass. actionlint covers .github/workflows/ (and transitively
# runs shellcheck on workflow `run:` blocks if shellcheck is on PATH).
# shellcheck handles standalone scripts under scripts/. Both are gated on
# `command -v` so a contributor without the tools installed still gets a
# usable `make dist`; install with `brew install actionlint shellcheck`.
lint:
	@if command -v actionlint >/dev/null 2>&1; then \
		echo "Running actionlint..."; \
		actionlint; \
	else \
		echo "actionlint not installed — skipping (brew install actionlint)"; \
	fi
	@if command -v shellcheck >/dev/null 2>&1; then \
		echo "Running shellcheck..."; \
		shellcheck scripts/*.sh; \
	else \
		echo "shellcheck not installed — skipping (brew install shellcheck)"; \
	fi

# Print the current version (from the VERSION file)
version:
	@echo $(VERSION)

# Cut a release: run tests, bump VERSION, commit, and tag.
# Usage: make release NEW_VERSION=0.1.0
#
# The push (which triggers the CI release workflow) is left to the user; the
# script prints the exact `git push` invocation after creating the local
# commit and tag.
#
# Implementation note: the release script is bash and targets macOS/Linux only.
# Windows users should run the equivalent git steps manually — see the comment
# at the top of scripts/release.sh.
release:
	@if [ -z "$(NEW_VERSION)" ]; then \
		echo "Usage: make release NEW_VERSION=X.Y.Z"; \
		exit 1; \
	fi
	./scripts/release.sh $(NEW_VERSION)

# Dependency tracking. .d files are produced as a side-effect of compilation
# via -MMD -MP in CFLAGS, so no dedicated rule is needed.
-include $(OBJECTS:.o=.d)
-include $(TEST_OBJECTS:.o=.d)
