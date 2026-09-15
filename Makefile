# Host side build: the pure logic and its tests, with no pico-sdk present.
# The firmware itself is built by cmake under firmware/ (see README.md); this
# file never touches it. The shape follows the stopbath-flipper Makefile so a
# contributor moving between the two repositories meets one set of targets.

# make predefines CC as "cc", which the MinGW distribution on the Windows host
# does not provide, so only a caller supplied CC survives; the default is gcc.
ifeq ($(origin CC),default)
CC := gcc
endif
PYTHON ?= python3

# The Flipper repository's warning set, kept identical here so the pure logic
# modules read and compile the same in both. The firmware build applies the
# same set to this project's own sources (firmware/CMakeLists.txt); vendored
# code is held to the base set without the two extras, exactly as there.
BASE_WARNINGS := -std=gnu2x -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls \
                 -Wdouble-promotion -Wundef
HOST_WARNINGS := $(BASE_WARNINGS) -Wshadow -Wconversion
HOST_CFLAGS := $(HOST_WARNINGS) -O1 -g
SANITISER_CFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
# Heap functions are wrapped in every test binary so a test can prove that
# the code under it did not allocate (Flipper 0.10). The wrappers live in
# tests/test_support.h.
ALLOCATION_WRAP_LDFLAGS := -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

BUILD_DIR := build/host

# Every pure logic module, compiled into every suite. Adding a module here is
# the only change needed for the tests to see it.
PURE_LOGIC_SOURCES := remote_input/remote_input_model.c \
                      remote_display/remote_bitmap.c \
                      remote_display/remote_font.c \
                      remote_display/remote_display_layout.c \
                      remote_display/remote_display_fixtures.c \
                      remote_display/remote_refresh_policy.c \
                      protocol/remote_protocol_tables.c \
                      protocol/remote_protocol.c \
                      peer/development_peer_core.c \
                      session/remote_session.c \
                      transport/remote_link_edge.c
PURE_LOGIC_HEADERS := $(wildcard remote_input/*.h remote_display/*.h protocol/*.h peer/*.h session/*.h transport/*.h)

# The fuzz driver: the protocol library plus the harness, run for a fixed
# number of deterministic inputs. FUZZ_ITERATIONS sets how many.
FUZZ_SOURCE := fuzz/fuzz_remote_protocol.c
FUZZ_ITERATIONS ?= 200000
PROTOCOL_SOURCES := protocol/remote_protocol_tables.c protocol/remote_protocol.c

# The development peer shell is POSIX (termios), so it builds on Linux and
# WSL, not under MinGW. It is a host tool, never part of the firmware.
PEER_SOURCES := peer/development_peer_shell.c peer/development_peer_core.c $(PROTOCOL_SOURCES)

# One binary per suite, named after its source.
TEST_SOURCES := $(wildcard tests/test_*.c)
TEST_HEADERS := tests/test_support.h
TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SOURCES))
SANITISED_TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%_sanitised,$(TEST_SOURCES))

.PHONY: test test-sanitise fuzz fuzz-sanitise peer check-typography check-protocol-tables check-protocol-definition check clean

test: $(TEST_BINARIES)
	@for suite in $(TEST_BINARIES); do echo "== $$suite"; $$suite || exit 1; done

test-sanitise: $(SANITISED_TEST_BINARIES)
	@for suite in $(SANITISED_TEST_BINARIES); do echo "== $$suite"; $$suite || exit 1; done

$(BUILD_DIR)/%: tests/%.c $(PURE_LOGIC_SOURCES) $(PURE_LOGIC_HEADERS) $(TEST_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $< $(PURE_LOGIC_SOURCES) $(ALLOCATION_WRAP_LDFLAGS)

$(BUILD_DIR)/%_sanitised: tests/%.c $(PURE_LOGIC_SOURCES) $(PURE_LOGIC_HEADERS) $(TEST_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) $(SANITISER_CFLAGS) -o $@ $< $(PURE_LOGIC_SOURCES) $(ALLOCATION_WRAP_LDFLAGS)

fuzz: $(BUILD_DIR)/fuzz_remote_protocol
	$(BUILD_DIR)/fuzz_remote_protocol $(FUZZ_ITERATIONS)

fuzz-sanitise: $(BUILD_DIR)/fuzz_remote_protocol_sanitised
	$(BUILD_DIR)/fuzz_remote_protocol_sanitised $(FUZZ_ITERATIONS)

$(BUILD_DIR)/fuzz_remote_protocol: $(FUZZ_SOURCE) $(PROTOCOL_SOURCES) $(PURE_LOGIC_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $(FUZZ_SOURCE) $(PROTOCOL_SOURCES)

$(BUILD_DIR)/fuzz_remote_protocol_sanitised: $(FUZZ_SOURCE) $(PROTOCOL_SOURCES) $(PURE_LOGIC_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) $(SANITISER_CFLAGS) -o $@ $(FUZZ_SOURCE) $(PROTOCOL_SOURCES)

peer: $(BUILD_DIR)/development_peer

$(BUILD_DIR)/development_peer: $(PEER_SOURCES) $(PURE_LOGIC_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $(PEER_SOURCES)

check-typography:
	$(PYTHON) scripts/check_typography.py

# The generated parser and encoder tables must match protocol.json exactly,
# and protocol.json must be the appliance's frozen definition.
check-protocol-tables:
	$(PYTHON) scripts/generate_protocol_tables.py --check

check-protocol-definition:
	$(PYTHON) scripts/check_protocol_definition.py

check: check-typography check-protocol-tables check-protocol-definition test fuzz

clean:
	rm -rf build
