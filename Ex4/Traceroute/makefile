# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# Executable file
EXEC = traceroute

# Source and object files
SRC = traceroute.c
OBJ = traceroute.o

# Default target
all: $(EXEC)

# Compile the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)

# Compile the object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Run the program
run: $(EXEC)
	sudo ./$(EXEC) -a 8.8.8.8

# Clean build files
clean:
	rm -f $(OBJ) $(EXEC)
