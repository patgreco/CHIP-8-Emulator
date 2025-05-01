#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "emulator.h"

int draw_flag = 0;
int running = 1;

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

void initialize(SDL_Renderer *renderer)
{
    pc = START_ADDRESS;
    I = 0x0000;
    memset(stack, 0, sizeof(stack));
    memset(V, 0, sizeof(V));
    memset(keypad, 0, sizeof(keypad));
    sp = 0;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void handle_input(SDL_Event *e, uint8_t *keypad)
{
    while (SDL_PollEvent(e))
    {
        if (e->type == SDL_QUIT)
            running = 0;

        else if (e->type == SDL_KEYDOWN || e->type == SDL_KEYUP)
        {
            int key = map_sdl_key_to_chip8(e->key.keysym.sym);
            if (key != -1)
            {
                keypad[key] = (e->type == SDL_KEYDOWN) ? 1 : 0;
            }
        }
    }
}

void emulate_cycle()
{
    uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
    pc += 2;

    switch (opcode & 0xF000)
    {
    case 0x0000:
        switch (opcode & 0x0FFF)
        {
        case 0x00E0:
            // Clear screen
            memset(video, 0, sizeof(video));
            draw_flag = 1;
            break;
        case 0x00EE:
            // Return from subroutine
            sp--;
            pc = stack[sp];
            draw_flag = 0;
            break;
        default:
            // SYS addr (usually ignored), deal with it later
            // printf("Warning: Ignored opcode 0x%04X (0NNN call)\n", opcode);
            break;
        }
        break;
    case 0x1000:
        // Jump
        pc = opcode & 0x0FFF;
        draw_flag = 0;
        break;
    case 0x2000:
        // Call subroutine
        stack[sp] = pc;
        sp++;
        pc = opcode & 0x0FFF;
        draw_flag = 0;
        break;
    case 0x3000:
        // Skip conditionally
        uint8_t value = opcode & 0x00FF;
        if (value == V[opcode & 0x0F00])
        {
            pc += 2;
        }
        draw_flag = 0;
        break;
    case 0x4000:
        // Skip conditionally
        uint8_t value = opcode & 0x00FF;
        if (value != V[opcode & 0x0F00])
        {
            pc += 2;
        }
        draw_flag = 0;
        break;
    case 0x5000:
        // Skip conditionally
        if ((opcode & 0x000F) == 0x0000)
        {
            if (V[opcode & 0x00F0] == V[opcode & 0x0F00])
            {
                pc += 2;
            }
            draw_flag = 0;
        }
        else
        {
            // Invalid opcode, deal with it later
        }
        break;
    case 0x9000:
        // Skip conditionally
        if ((opcode & 0x000F) == 0x0000)
        {
            if (V[opcode & 0x00F0] != V[opcode & 0x0F00])
            {
                pc += 2;
            }
            draw_flag = 0;
        }
        else
        {
            // Invalid opcode, deal with it later
        }
        break;
    case 0x6000:
        // Set
        V[opcode & 0x0F00] = opcode & 0x00FF;
        break;
    case 0x7000:
        // Add
        V[opcode & 0x0F00] += opcode & 0x00FF;
        break;
    case 0x8000:
        // Logical and arithmetic instructions
        break;
    case 0xA000:
        // Set index
        I = opcode & 0x0FFF;
        break;
    case 0xB000:
        // Jump with offset
        break;
    case 0xC000:
        // Random
        break;
    case 0xD000:
        // Display

        break;
    case 0xE000:
        // Skip if key
        break;
    case 0xF000:
        break;
    default:
        // default code block
    }
}

void render(SDL_Renderer *renderer, uint8_t *video)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White pixels

    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            if (video[y * 64 + x])
            {
                SDL_Rect pixel = {
                    x * WINDOW_SCALE,
                    y * WINDOW_SCALE,
                    WINDOW_SCALE,
                    WINDOW_SCALE};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s romfile\n", argv[0]);
        return 1;
    }

    if (load_rom(argv[1]) != 0)
    {
        return 1;
    }

    // Now memory[0x200] contains the first byte of the CHIP-8 program

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("CHIP-8 Emulator",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Event event;

    initialize(renderer);

    while (running)
    {
        handle_input(&event, &keypad);

        emulate_cycle();

        if (draw_flag)
        {
            render(renderer, &video);
            draw_flag = 0;
        }

        /*while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = 0;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);*/

        SDL_Delay(16); // ~60fps
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}