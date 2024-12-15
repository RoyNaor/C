# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude

# Directories
SRC_DIR = Ex2

# Target Name
TARGET = Main

# Source and Object Files
SRCS = methods.c main.c
OBJS = $(SRCS:.c=.o)

# Rule to Build the Target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Rule to Compile into Object
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# Clean Up Build Files
clean:
	rm -rf $(OBJS) $(TARGET)

