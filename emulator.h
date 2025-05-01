#include <stdint.h>

#define MEMORY_SIZE 4096
#define START_ADDRESS 0x200

#define WINDOW_SCALE 10
#define WINDOW_WIDTH (64 * WINDOW_SCALE)
#define WINDOW_HEIGHT (32 * WINDOW_SCALE)

// Should represent RAM
uint8_t memory[MEMORY_SIZE];

// Should represent the general purpose registers
uint8_t V[16];

// Should represent the I register
uint16_t I;

// Should represent the program counter
uint16_t pc;

// Should represent the stack
uint16_t stack[16];

uint8_t video[64 * 32];