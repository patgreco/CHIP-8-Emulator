#define MEMORY_SIZE 4096
#define START_ADDRESS 0x200

// Should represent RAM
unsigned char memory[MEMORY_SIZE];

// Should represent the general purpose registers
unsigned char V[16];

// Should represent the I register
unsigned short I;

// Should represent the program counter
unsigned short pc;

// Should represent the stack
unsigned short stack[16];