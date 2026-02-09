CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = shell
SRC = src/myshell.c
BIN_DIR = bin

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$(TARGET) $(SRC)

clean:
	rm -f $(BIN_DIR)/$(TARGET)

.PHONY: all clean
