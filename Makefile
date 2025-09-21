# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: frthierr <frthierr@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/21 16:41:09 by frthierr          #+#    #+#              #
#    Updated: 2025/09/21 16:55:17 by frthierr         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Makefile — portable build for libft_malloc on Linux & macOS
# Builds a shared library: libft_malloc_$(HOSTTYPE).so
# Creates a symlink:      libft_malloc.so -> libft_malloc_$(HOSTTYPE).so
# Objects & deps mirrored under build/ (e.g., lib/x/y.c -> build/x/y.o, build/x/y.d)
# Knobs:
#   DEBUG=1  -> add -g -O0 -DDEBUG
#   SAN=1    -> add sanitizers (intended for unit-test binaries; may conflict with interposed malloc)
#   EXPORTS=0-> disable symbol export lists (by default exports are enabled)

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

# Discover all C sources under lib/
SRCS      := $(shell find lib -name '*.c')
# Mirror lib/%.c -> build/%.o
OBJS      := $(patsubst lib/%.c,build/%.o,$(SRCS))
DEPS      := $(OBJS:.o=.d)

# ------------------------------- flags -----------------------------------------

CFLAGS   ?= -O2
CFLAGS   += -fPIC -Wall -Wextra -Ilib -MMD -MP
LDFLAGS  ?=
LDLIBS   ?=

# Optional: hide non-API symbols by default (pair with export lists below)
# Disable with: make NO_VIS=1
ifeq ($(NO_VIS),)
CFLAGS   += -fvisibility=hidden
endif

# Debug knob
ifeq ($(DEBUG),1)
  CFLAGS := $(filter-out -O2,$(CFLAGS))
  CFLAGS += -g -O0 -DDEBUG
endif

# Sanitizer knob (use for unit tests; ASan may conflict with interposed malloc)
ifeq ($(SAN),1)
  CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
  LDFLAGS += -fsanitize=address,undefined
endif

# ------------------------------- platform link mode -----------------------------

# Default to using export lists; disable with EXPORTS=0
EXPORTS ?= 1

ifeq ($(UNAME_S),Linux)
  SHARED_FLAG       := -shared
  SONAME_FLAG       := -Wl,-soname,$(TARGET)
  EXPORTS_MAP       := build/exports.map
  EXPORTS_FLAG_DEF  := -Wl,--version-script,$(EXPORTS_MAP)
  EXPORTS_FILE_DEF  := $(EXPORTS_MAP)
else ifeq ($(UNAME_S),Darwin)
  SHARED_FLAG       := -dynamiclib
  INSTALL_NAME_FLAG := -Wl,-install_name,@rpath/$(TARGET)
  EXPORTS_OSX       := build/exports_osx.txt
  EXPORTS_FLAG_DEF  := -Wl,-exported_symbols_list,$(EXPORTS_OSX)
  EXPORTS_FILE_DEF  := $(EXPORTS_OSX)
else
  # Fallback (other UNIX)
  SHARED_FLAG       := -shared
  SONAME_FLAG       :=
  EXPORTS_FLAG_DEF  :=
  EXPORTS_FILE_DEF  :=
endif

# Allow disabling export lists
ifeq ($(EXPORTS),1)
  EXPORTS_FLAG := $(EXPORTS_FLAG_DEF)
  EXPORTS_FILE := $(EXPORTS_FILE_DEF)
else
  EXPORTS_FLAG :=
  EXPORTS_FILE :=
endif

# ------------------------------- targets ---------------------------------------

.PHONY: all clean fclean symlink

all: $(TARGET) symlink

# Link the shared library
$(TARGET): $(OBJS) $(EXPORTS_FILE)
	$(CC) $(SHARED_FLAG) $(SONAME_FLAG) $(INSTALL_NAME_FLAG) $(LDFLAGS) -o $@ $(OBJS) $(EXPORTS_FLAG) $(LDLIBS)

# Symlink that the subject expects
symlink: $(TARGET)
	@ln -sf $(TARGET) $(SYMLINK)

# Pattern rule: compile lib/%.c -> build/%.o (with deps alongside)
build/%.o: lib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Auto-generated dependency files
-include $(DEPS)

# Generate export list for Linux (version script)
build/exports.map:
	@mkdir -p $(dir $@)
	@printf '%s\n' '{' \
	'  global:' \
	'    malloc;' \
	'    free;' \
	'    realloc;' \
	'    show_alloc_mem;' \
	'  local:' \
	'    *;' \
	'};' > $@

# Generate export list for macOS (note leading underscores)
build/exports_osx.txt:
	@mkdir -p $(dir $@)
	@printf '%s\n' '_malloc' '_free' '_realloc' '_show_alloc_mem' > $@

# Cleaning rules
clean:
	$(RM) $(TARGET) $(SYMLINK)

fclean: clean
	$(RM) -r build

re: fclean all
