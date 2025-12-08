CC      := gcc
VERSION := 0.1.0

CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 \
           -D_POSIX_C_SOURCE=200809L \
           -DDEVPACK_VERSION=\"$(VERSION)\"

INCLUDES:= -Isrc -Ithird_party/cJSON

SRCS := \
    src/main.c \
    src/stack.c \
    src/stack_list.c \
    src/stack_loader.c \
    third_party/cJSON/cJSON.c

OBJS := $(SRCS:.c=.o)

TARGET := devpack

# Where to install
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	mkdir -p "$(BINDIR)"
	cp "$(TARGET)" "$(BINDIR)/$(TARGET)"
	chmod 755 "$(BINDIR)/$(TARGET)"
	@echo "Installed $(TARGET) to $(BINDIR)"

uninstall:
	rm -f "$(BINDIR)/$(TARGET)"
	@echo "Removed $(BINDIR)/$(TARGET)"

# Simple source release tarball
release: clean
	mkdir -p dist/$(TARGET)-$(VERSION)
	cp -r src stacks third_party Makefile dist/$(TARGET)-$(VERSION) 2>/dev/null || true
	[ -f README.md ] && cp README.md dist/$(TARGET)-$(VERSION) || true
	tar czf dist/$(TARGET)-$(VERSION).tar.gz -C dist $(TARGET)-$(VERSION)
	rm -rf dist/$(TARGET)-$(VERSION)
	@echo "Created dist/$(TARGET)-$(VERSION).tar.gz"

.PHONY: all clean install uninstall release
