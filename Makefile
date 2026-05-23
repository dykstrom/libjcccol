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
LIB_NAME := libjcccol.a
LIB_PATH := $(BUILD_DIR)/$(LIB_NAME)

# Version (single source of truth: the VERSION file at the repo root).
# Bumped by scripts/release.sh as part of `make release`.
VERSION := $(shell cat VERSION 2>/dev/null || echo 0.0.0)

# Source files
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Compiler flags
CFLAGS := -Wall -Wextra -Werror -std=c11 -O2 -I$(INCLUDE_DIR)
LDFLAGS :=
ARFLAGS := rcs

# Executable suffix. Empty on POSIX; .exe on Windows (MSYS2/MinGW/Cygwin or
# native nmake-style OS=Windows_NT). Baked into the test-binary pattern rule
# below so make matches `build/test_core.exe` against `tests/test_core.c`
# without auto-appending a second .exe.
EXE :=
UNAME_S := $(shell uname -s)
ifneq (,$(findstring MINGW,$(UNAME_S)))
    EXE := .exe
endif
ifneq (,$(findstring CYGWIN,$(UNAME_S)))
    EXE := .exe
endif
ifeq ($(OS),Windows_NT)
    EXE := .exe
endif

# Test files
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.c)
TEST_BINARIES := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%$(EXE),$(TEST_SOURCES))

# Phony targets
.PHONY: all clean test install help dirs release version

# Default target
all: dirs $(LIB_PATH)

# Help target
help:
	@echo "libjcccol - COL Standard Library Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build the library (default)"
	@echo "  test       - Build and run all tests"
	@echo "  clean      - Remove all build artifacts"
	@echo "  install    - Install library to system (requires PREFIX)"
	@echo "  version    - Print the current library version"
	@echo "  release    - Cut a release (requires NEW_VERSION=X.Y.Z; macOS/Linux only)"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Example usage:"
	@echo "  make              # Build the library"
	@echo "  make test         # Run tests"
	@echo "  make clean        # Clean build artifacts"

# Create necessary directories
dirs:
	@mkdir -p $(OBJ_DIR) $(BUILD_DIR)

# Build the static library
$(LIB_PATH): $(OBJECTS)
	@echo "Creating static library: $@"
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@
	@echo "Library created successfully: $@"

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Build test executables
$(BUILD_DIR)/%$(EXE): $(TEST_DIR)/%.c $(LIB_PATH)
	@echo "Building test: $@"
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -ljcccol -o $@ $(LDFLAGS)

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
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR) $(BUILD_DIR)
	@echo "Clean complete."

# Install library (requires PREFIX to be set)
install: $(LIB_PATH)
ifndef PREFIX
	@echo "Error: PREFIX not set. Usage: make install PREFIX=/usr/local"
	@exit 1
endif
	@echo "Installing library to $(PREFIX)..."
	install -d $(PREFIX)/lib
	install -d $(PREFIX)/include/jcccol
	install -m 644 $(LIB_PATH) $(PREFIX)/lib/
	install -m 644 $(INCLUDE_DIR)/jcccol.h $(PREFIX)/include/
	install -m 644 $(INCLUDE_DIR)/jcccol/core.h $(PREFIX)/include/jcccol/
	@echo "Installation complete."

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

# Dependency tracking
-include $(OBJECTS:.o=.d)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< -MF $@
