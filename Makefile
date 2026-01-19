CC = gcc
CROSS_CC = zig cc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = 
TARGET = hid-gadget
MOCK_TARGET = hid-gadget-mock

# Track only the source files you have
SRC = hid-gadget.c tui.c

# Architectures to build
ARCHS = arm64 x86_64 arm x86

# Mapping names to Zig target triples
TARGET_arm64 = aarch64-linux-musl
TARGET_x86_64 = x86_64-linux-musl
TARGET_arm = arm-linux-musleabi
TARGET_x86 = x86-linux-musl

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

$(MOCK_TARGET): $(SRC)
	$(CC) $(CFLAGS) -DMOCK_HID -o $@ $(SRC) $(LDFLAGS)

# This rule handles directory creation and compilation in one go
static-%: $(SRC)
	@mkdir -p ./blobs/$*
	$(CROSS_CC) --target=$(TARGET_$(subst -,_,$*)) -static $(CFLAGS) -o hid-gadget-$*-static $(SRC) $(LDFLAGS)
	cp hid-gadget-$*-static ./blobs/$*/hid-gadget

static: $(addprefix static-, $(ARCHS))

mock-static: $(SRC)
	$(CC) $(CFLAGS) -static -DMOCK_HID -o hid-gadget-mock-static $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(MOCK_TARGET) *-static
	rm -rf ./blobs/*

.PHONY: all mock-static static clean
