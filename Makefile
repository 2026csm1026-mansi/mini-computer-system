# CS527 Lab 5 - works with Linux, WSL and MinGW Windows

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
TARGET = cs527_lab5

ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
endif

SRC = main.c compiler.c processor.c memory.c os.c
OBJ = main.o compiler.o processor.o memory.o os.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ) $(TARGET) program.byte processor_0.log processor_1.log processor_2.log processor_3.log

.PHONY: all clean
