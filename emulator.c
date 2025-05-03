#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <time.h>
#include "emulator.h"

int draw_flag = 0;
int running = 1;
int CONFIGURED = 0;

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
    delay_timer = 0;
    sound_timer = 0;

    // Set fonts
    // 0
    memory[0x050] = 0xF0;
    memory[0x051] = 0x90;
    memory[0x052] = 0x90;
    memory[0x053] = 0x90;
    memory[0x054] = 0xF0;

    // 1
    memory[0x055] = 0x20;
    memory[0x056] = 0x60;
    memory[0x057] = 0x20;
    memory[0x058] = 0x20;
    memory[0x059] = 0x70;

    // 2
    memory[0x05A] = 0xF0;
    memory[0x05B] = 0x10;
    memory[0x05C] = 0xF0;
    memory[0x05D] = 0x80;
    memory[0x05E] = 0xF0;

    // 3
    memory[0x05F] = 0xF0;
    memory[0x060] = 0x10;
    memory[0x061] = 0xF0;
    memory[0x062] = 0x10;
    memory[0x063] = 0xF0;

    // 4
    memory[0x064] = 0x90;
    memory[0x065] = 0x90;
    memory[0x066] = 0xF0;
    memory[0x067] = 0x10;
    memory[0x068] = 0x10;

    // 5
    memory[0x069] = 0xF0;
    memory[0x06A] = 0x80;
    memory[0x06B] = 0xF0;
    memory[0x06C] = 0x10;
    memory[0x06D] = 0xF0;

    // 6
    memory[0x06E] = 0xF0;
    memory[0x06F] = 0x80;
    memory[0x070] = 0xF0;
    memory[0x071] = 0x90;
    memory[0x072] = 0xF0;

    // 7
    memory[0x073] = 0xF0;
    memory[0x074] = 0x10;
    memory[0x075] = 0x20;
    memory[0x076] = 0x40;
    memory[0x077] = 0x40;

    // 8
    memory[0x078] = 0xF0;
    memory[0x079] = 0x90;
    memory[0x07A] = 0xF0;
    memory[0x07B] = 0x90;
    memory[0x07C] = 0xF0;

    // 9
    memory[0x07D] = 0xF0;
    memory[0x07E] = 0x90;
    memory[0x07F] = 0xF0;
    memory[0x080] = 0x10;
    memory[0x081] = 0xF0;

    // A
    memory[0x082] = 0xF0;
    memory[0x083] = 0x90;
    memory[0x084] = 0xF0;
    memory[0x085] = 0x90;
    memory[0x086] = 0x90;

    // B
    memory[0x087] = 0xE0;
    memory[0x088] = 0x90;
    memory[0x089] = 0xE0;
    memory[0x08A] = 0x90;
    memory[0x08B] = 0xE0;

    // C
    memory[0x08C] = 0xF0;
    memory[0x08D] = 0x80;
    memory[0x08E] = 0x80;
    memory[0x08F] = 0x80;
    memory[0x090] = 0xF0;

    // D
    memory[0x091] = 0xE0;
    memory[0x092] = 0x90;
    memory[0x093] = 0x90;
    memory[0x094] = 0x90;
    memory[0x095] = 0xE0;

    // E
    memory[0x096] = 0xF0;
    memory[0x097] = 0x80;
    memory[0x098] = 0xF0;
    memory[0x099] = 0x80;
    memory[0x09A] = 0xF0;

    // F
    memory[0x09B] = 0xF0;
    memory[0x09C] = 0x80;
    memory[0x09D] = 0xF0;
    memory[0x09E] = 0x80;
    memory[0x09F] = 0x80;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    // Initialize the random seed for the CXNN instruction
    srand(time(NULL));
}

int map_sdl_key_to_chip8(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_1:
        return 0x1;
    case SDLK_2:
        return 0x2;
    case SDLK_3:
        return 0x3;
    case SDLK_4:
        return 0xC;

    case SDLK_q:
        return 0x4;
    case SDLK_w:
        return 0x5;
    case SDLK_e:
        return 0x6;
    case SDLK_r:
        return 0xD;

    case SDLK_a:
        return 0x7;
    case SDLK_s:
        return 0x8;
    case SDLK_d:
        return 0x9;
    case SDLK_f:
        return 0xE;

    case SDLK_z:
        return 0xA;
    case SDLK_x:
        return 0x0;
    case SDLK_c:
        return 0xB;
    case SDLK_v:
        return 0xF;

    default:
        return -1; // Not a CHIP-8 key
    }
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

    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t value = opcode & 0x00FF;
    uint8_t key = V[x];

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
        if (value == V[x])
        {
            pc += 2;
        }
        draw_flag = 0;
        break;
    case 0x4000:
        // Skip conditionally
        if (value != V[x])
        {
            pc += 2;
        }
        draw_flag = 0;
        break;
    case 0x5000:
        // Skip conditionally
        if ((opcode & 0x000F) == 0x0000)
        {
            if (V[x] == V[y])
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
            if (V[x] != V[y])
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
        V[x] = opcode & 0x00FF;
        draw_flag = 0;
        break;
    case 0x7000:
        // Add
        V[x] += opcode & 0x00FF;
        draw_flag = 0;
        break;
    case 0x8000:
        // Logical and arithmetic instructions
        switch (opcode & 0x000F)
        {
        case 0x0000:
            // Set
            V[x] = V[y];
            draw_flag = 0;
            break;
        case 0x0001:
            // Binary OR
            V[x] = V[x] | V[y];
            draw_flag = 0;
            break;
        case 0x0002:
            // Binary AND
            V[x] = V[x] & V[y];
            draw_flag = 0;
            break;
        case 0x0003:
            // Binary XOR
            V[x] = V[x] ^ V[y];
            draw_flag = 0;
            break;
        case 0x0004:
            // Add
            if (V[x] + V[y] > 255)
                V[0xF] = 1;
            else
                V[0xF] = 0;

            V[x] = V[x] + V[y];
            draw_flag = 0;
            break;
        }
        break;
    case 0x0005:
        // Subtract
        if (V[x] >= V[y])
            V[0xF] = 1;
        else
            V[0xF] = 0;

        V[x] = V[x] - V[y];
        draw_flag = 0;
        break;
    case 0x0007:
        // Subtract
        if (V[y] >= V[x])
            V[0xF] = 1;
        else
            V[0xF] = 0;

        V[x] = V[y] - V[x];
        draw_flag = 0;
        break;
    case 0x0006:
        // Shift
        if (CONFIGURED)
        {
            V[x] = V[y];
        }
        if ((V[x] & 0x01) == 0x01)
            V[0xF] = 1;
        else
            V[0xF] = 0;

        V[x] = V[x] >> 1;
        draw_flag = 0;
        break;
    case 0x000E:
        // Shift
        if (CONFIGURED)
        {
            V[x] = V[y];
        }
        if ((V[x] & 0x80) == 0x80)
            V[0xF] = 1;
        else
            V[0xF] = 0;

        V[x] = V[x] << 1;
        draw_flag = 0;
        break;
    case 0xA000:
        // Set index
        I = opcode & 0x0FFF;
        draw_flag = 0;
        break;
    case 0xB000:
        // Jump with offset
        pc = (opcode & 0x0FFF) + V[0];
        break;
    case 0xC000:
        // Random
        V[x] = (rand() % 256) & (opcode & 0x00FF);
        break;
    case 0xD000:
        // Display
        {
            uint8_t xCoord = V[x] % 64;
            uint8_t yCoord = V[y] % 32;
            V[0xF] = 0;
            int N = opcode & 0x000F;
            for (int i = 0; i < N; i++)
            {
                u_int8_t NthByte = memory[I + i];
                uint8_t pixelY = (yCoord + i) % 32;
                for (int j = 0; j < 8; j++)
                {
                    uint8_t pixelX = (xCoord + j) % 64;
                    uint8_t bit = (NthByte >> (7 - j)) & 1;
                    uint32_t index = pixelY * 64 + pixelX;

                    if (bit && video[index])
                    {
                        video[index] = 0;
                        V[0xF] = 1;
                    }
                    else if (bit && video[index] == 0)
                    {
                        video[index] = 1;
                    }
                    if (pixelX == 63)
                    {
                        // Reached edge of screen
                        break;
                    }
                }
                if (pixelY == 31)
                {
                    // Reached edge of screen
                    break;
                }
            }
            draw_flag = 1;
            break;
        }
    case 0xE000:
        // Skip if key
        if ((opcode & 0x00FF) == 0x009E)
        {
            if (keypad[key])
            {
                pc += 2;
            }
        }
        else if ((opcode & 0x00FF) == 0x00A1)
        {
            if (!keypad[key])
            {
                pc += 2;
            }
        }
        else
        {
            // Invalid opcode, deal with it later
        }
        break;
    case 0xF000:
        switch (opcode & 0x00FF)
        {
        case 0x0007:
            // Delay timer
            V[x] = delay_timer;
            break;
        case 0x0015:
            // Delay timer
            delay_timer = V[x];
            break;
        case 0x0018:
            // Sound timer
            sound_timer = V[x];
            break;
        default:
            // Invalid opcode, deal with it later
            break;
        }
        break;
    default:
        printf("Not done here yet\n");
        break;
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
    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "Usage: %s romfile [FLAG]\nwhere FLAG is \"set\"\n", argv[0]);
        return 1;
    }

    if (argc == 3)
    {
        if (strcmp(argv[2], "set") == 0)
        {
            CONFIGURED = 1;
        }
        else
        {
            fprintf(stderr, "Usage: %s romfile [FLAG]\nwhere FLAG is \"set\"\n", argv[0]);
            return 1;
        }
    }

    if (load_rom(argv[1]) != 0)
    {
        return 1;
    }

    // Now memory[0x200] contains the first byte of the CHIP-8 program

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("CHIP-8 Emulator",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

    if (window == NULL)
    {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL)
    {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Event event;

    initialize(renderer);

    uint32_t last_timer_tick = 0;

    while (running)
    {
        handle_input(&event, keypad);

        emulate_cycle();

        if (draw_flag)
        {
            render(renderer, video);
            draw_flag = 0;
        }

        // Decrement timers at 60Hz
        uint32_t now = SDL_GetTicks();          // Milliseconds since SDL init
        if (now - last_timer_tick >= 1000 / 60) // 16.67 ms
        {
            if (delay_timer > 0)
                delay_timer--;

            if (sound_timer > 0)
            {
                sound_timer--;
                // printf("BEEP!\n");
                // Can play a beep while sound_timer > 0
            }

            last_timer_tick = now;
        }

        SDL_Delay(1); // ~60fps
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}