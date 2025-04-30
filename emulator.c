#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include "emulator.h"

void initialize()
{
    pc = START_ADDRESS;
    I = 0x0;
}

int load_rom(const char *filename)
{
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        perror("Error opening file");
        return 1;
    }

    // Get ROM file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if (size < 0)
    {
        perror("ftell failed");
        fclose(file);
        return 1;
    }
    size_t rom_size = (size_t)size;

    rewind(file);

    if (rom_size + START_ADDRESS > MEMORY_SIZE)
    {
        fprintf(stderr, "ROM file too large to fit in memory\n");
        fclose(file);
        return 1;
    }

    // Load ROM into memory at starting at address 0x200
    size_t bytesRead = fread(&memory[START_ADDRESS], sizeof(unsigned char), rom_size, file);
    if (bytesRead != rom_size)
    {
        // Check if we read all the bytes
        fprintf(stderr, "Error: Only %zu bytes read from ROM file (expected %ld bytes)\n", bytesRead, rom_size);
        fclose(file);
        return 1;
    }

    fclose(file);

    printf("Loaded ROM file (%ld bytes) into memory at 0x%03X\n", rom_size, START_ADDRESS);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s romfile\n", argv[0]);
        return 1;
    }

    initialize();

    if (load_rom(argv[1]) != 0)
    {
        return 1;
    }

    // Now memory[0x200] contains the first byte of the CHIP-8 program

    // Next: implement fetch-decode-execute loop...
    // uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
    // Decode: Use bit masking to figure out which instruction it is.
    // Execute: Call the corresponding function or logic.

    return 0;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("SDL Demo", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int running = 1;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = 0;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}