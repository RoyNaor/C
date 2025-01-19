CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = discovery

all: $(TARGET)

$(TARGET): discovery.c
	$(CC) $(CFLAGS) -o $(TARGET) discovery.c

clean:
	rm -f $(TARGET)
