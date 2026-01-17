CC = gcc
CROSS_CC = zig cc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = 
TARGET_ARM64 = aarch64-linux-musl
TARGET_X86_64 = x86_64-linux-musl
TARGET_ARM = arm-linux-musleabi
TARGET_X86 = x86-linux-musl

TARGET = hid-gadget
MOCK_TARGET = hid-gadget-mock

all: $(TARGET)

$(TARGET): hid-gadget.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(MOCK_TARGET): hid-gadget.c
	$(CC) $(CFLAGS) -DMOCK_HID -o $@ $< $(LDFLAGS)

# Functional static binaries for Android/Linux
static-arm64: hid-gadget.c
	$(CROSS_CC) --target=$(TARGET_ARM64) -static $(CFLAGS) -o hid-gadget-arm64-static $< $(LDFLAGS)

static-x86_64: hid-gadget.c
	$(CROSS_CC) --target=$(TARGET_X86_64) -static $(CFLAGS) -o hid-gadget-x86_64-static $< $(LDFLAGS)

static-arm: hid-gadget.c
	$(CROSS_CC) --target=$(TARGET_ARM) -static $(CFLAGS) -o hid-gadget-arm-static $< $(LDFLAGS)

static-x86: hid-gadget.c
	$(CROSS_CC) --target=$(TARGET_X86) -static $(CFLAGS) -o hid-gadget-x86-static $< $(LDFLAGS)

static: static-arm64 static-x86_64 static-arm static-x86

# Mock static binary for testing parsing on PC
mock-static: hid-gadget.c
	$(CC) $(CFLAGS) -static -DMOCK_HID -o hid-gadget-mock-static $< $(LDFLAGS)

clean:
	rm -f $(TARGET) $(MOCK_TARGET) hid-gadget-*-static hid-gadget-arm64 hid-gadget-test-arm64

.PHONY: all mock-static static static-arm64 static-x86_64 clean
