# Name of the final executable
TARGET = emulator

# Source files
SRC = emulator.c

# Compiler
CC = gcc

# SDL2 compile and link flags
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

# Build rule
all:
	$(CC) -Wall -Wextra -g -o $(TARGET) $(SRC) $(SDL_CFLAGS) $(SDL_LDFLAGS)

# Clean up
clean:
	rm -f $(TARGET)