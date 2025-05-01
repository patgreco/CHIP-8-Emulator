#include <stdint.h>

#define MEMORY_SIZE 4096
#define START_ADDRESS 0x200

#define WINDOW_SCALE 10
#define WINDOW_WIDTH (64 * WINDOW_SCALE)
#define WINDOW_HEIGHT (32 * WINDOW_SCALE)

// Represents the CHIP-8 RAM
uint8_t memory[MEMORY_SIZE];

// Represents the CHIP-8 general purpose registers
uint8_t V[16];

// Represents the CHIP-8 I register
uint16_t I;

// Represents the CHIP-8 program counter
uint16_t pc;

// Represents the CHIP-8 stack
uint16_t stack[16];

// Represents the CHIP-8 stack pointer (index into the stack)
uint8_t sp;

// Represents the CHIP-8 display (64 * 32 pixels)
uint8_t video[64 * 32];

// Represents the CHIP-8 keypad
uint8_t keypad[16]; // 1 = pressed, 0 = not pressed