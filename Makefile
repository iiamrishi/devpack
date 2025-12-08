# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L

INCLUDES:= -Isrc -Ithird_party/cJSON

# Sources
SRCS := \
    src/main.c \
    src/stack.c \
    src/stack_list.c \
    src/stack_loader.c \
    third_party/cJSON/cJSON.c

# Objects
OBJS := $(SRCS:.c=.o)

# Output binary
TARGET := devpack

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $@

# Generic rule for building .o from .c
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
