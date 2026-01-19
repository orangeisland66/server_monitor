CC = gcc
CFLAGS = -Wall -Wextra -I./include -I./lib/sqlite/sqlite-amalgamation-3450100 -pthread -D_GNU_SOURCE
LDFLAGS = -lpthread -ldl

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = lib

# SQLite source
SQLITE_SRC = ./lib/sqlite/sqlite-amalgamation-3450100/sqlite3.c
SQLITE_OBJ = $(OBJ_DIR)/sqlite3.o

# Project sources
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = $(BIN_DIR)/server_monitor

.PHONY: all clean directories

all: directories $(TARGET)

$(TARGET): $(OBJS) $(SQLITE_OBJ)
	$(CC) $(OBJS) $(SQLITE_OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SQLITE_OBJ): $(SQLITE_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
