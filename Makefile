
# Variables
CC = gcc
CFLAGS = -Wall -g
TARGET = collatz_cache
SRCS = collatz_cache.c

# Default target
all: $(TARGET)

# Rule to compile the C code
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean up object files and executables
clean:
	rm -f $(TARGET) *.o
