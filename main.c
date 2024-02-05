#include "main.h"

#define unknown_opcode(op) \
    do { \
        fprintf(stderr, "Unknown opcode: 0x%x\n", op); \
        fprintf(stderr, "kk: 0x%02x\n", kk); \
        exit(42); \
    } while (0)


#define IS_BIT_SET(byte, bit) <<<0x80 >> (bit) & (byte) != 0x0)
#define FONTSET_ADRESS 0x00
#define FONTSET_BYTES_CHAR 5
unsigned char chip_8_fontset[80] =
        {
                0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                0x20, 0x60, 0x20, 0x20, 0x70, // 1
                0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };


static inline uint8_t randbyte() { return (rand() % 256); }

uint16_t opcode;
uint8_t memory[MEMORY_SIZE];
uint8_t V[16];
uint16_t I;
uint16_t PC;
uint8_t dp[DP_ROWS][DP_COLUMNS];
uint8_t delay_timer;
uint8_t sound_timer;
uint16_t stack[STACK_SIZE];
uint16_t SP;
uint8_t key[KEY_SIZE];
bool chip8_draw_flag;

void draw_sprite(uint8_t x, uint8_t y, uint8_t n) {
    unsigned row = y, col = x;
    unsigned byte_index;
    unsigned bit_index;

    V[0xF] = 0;
    for (byte_index = 0; byte_index < n; byte_index++) {
        uint8_t byte = memory[I + byte_index];

        for (bit_index = 0; bit_index < 8; bit_index++) {
            uint8_t bit = (byte >> bit_index) & 0x1; //value of bit in sprite
            uint8_t *pixel = &dp[(row + byte_index) % DP_ROWS][(col + (7 - bit_index)) %
                                                               DP_COLUMNS]; //value of pixel on screen
            if (bit == 1 && *pixel == 1) V[0xF] = 1;  //Set ColisionFlag to 1 if any pixel would be erased.
            *pixel = *pixel ^ bit; //Draw Pixel by XOR
        }
    }
}

static void print_state() {
    printf("------------------------------------------------------------------\n");
    printf("\n");

    printf("V0: 0x%02x  V4: 0x%02x  V8: 0x%02x  VC: 0x%02x\n",
           V[0], V[4], V[8], V[12]);
    printf("V1: 0x%02x  V5: 0x%02x  V9: 0x%02x  VD: 0x%02x\n",
           V[1], V[5], V[9], V[13]);
    printf("V2: 0x%02x  V6: 0x%02x  VA: 0x%02x  VE: 0x%02x\n",
           V[2], V[6], V[10], V[14]);
    printf("V3: 0x%02x  V7: 0x%02x  VB: 0x%02x  VF: 0x%02x\n",
           V[3], V[7], V[11], V[15]);

    printf("\n");
    printf("PC: 0x%04x\n", PC);
    printf("\n");
    printf("\n");
}

void chip_8_ini() {
    int i;

    PC = 0x200;
    opcode = 0;
    I = 0;
    SP = 0;

    memset(memory, 0, sizeof(uint8_t) * MEMORY_SIZE);
    memset(V, 0, sizeof(uint8_t) * 16);
    memset(dp, 0, sizeof(uint8_t) * DP_SIZE);
    memset(stack, 0, sizeof(uint16_t) * STACK_SIZE);
    memset(key, 0, sizeof(uint8_t) * KEY_SIZE);

    for (i = 0; i < 80; i++) {
        memory[FONTSET_ADRESS + i] = chip_8_fontset[i];
    }

    chip8_draw_flag = true;
    delay_timer = 0;
    sound_timer = 0;
    srand(time(NULL));
}

void chip_8_games(char *game) {
    FILE *fgame;

    fgame = fopen(game, "rb");
    if (NULL == fgame) {
        fprintf(stderr, "Cannot open game %s\n", game);
        exit(42);
    }

    fread(&memory[0x200], 1, GAME_LIMIT_SIZE, fgame);
    fclose(fgame);
}

void chip_8_tick() {
    // update timers
    if (delay_timer > 0) {
        --delay_timer;
    }
    if (sound_timer > 0) {
        --sound_timer;
        if (sound_timer == 0) {
            printf("BEEP!\n");
        }
    }
}

void chip_8_opcycle() {
    int i;
    uint8_t x, y, n;
    uint8_t kk;
    uint16_t nnn;

    //fetch
    opcode = memory[PC] << 8 | memory[PC + 1];
    x = (opcode >> 8) & 0x000F; // the lower 4 bits of the high byte
    y = (opcode >> 4) & 0x000F; // the upper 4 bits of the low byte
    n = opcode & 0x000F; // the lowest 4 bits
    kk = opcode & 0x00FF; // the lowest 8 bits
    nnn = opcode & 0x0FFF; // the lowest 12 bits


#ifdef DEBUG
    printf("PC: 0x%04x Op: 0x%04x\n", PC, opcode);
#endif

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (kk) {
                case 0x00E0: //CLS
                    memset(dp, 0, sizeof(uint8_t) * DP_SIZE);
                    chip8_draw_flag = true;
                    PC += 2;
                    break;
                case 0x00EE: //RET
                    PC = stack[--SP];
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
        case 0x1000: //1NNN - JP addr
            PC = nnn;
            break;
        case 0x2000: //2NNN - CALL addr
            stack[SP++] = PC + 2;
            break;
        case 0x3000: //3xKK - SE Vx,byte
            PC += (V[x] == kk) ? 4 : 2;
            break;
        case 0X4000: //4xKK - SNE Vx,byte
            PC += (V[x] != kk) ? 4 : 2;
            break;
        case 0x5000: //5xy0 - SE Vx,Vy
            PC += (V[x] == V[y]) ? 4 : 2;
            break;
        case 0x6000: // 6xKK - LD Vx,byte
            V[x] = kk;
            PC += 2;
            break;
        case 0x7000: // 7xKK - ADD Vx,byte
            V[x] += kk;
            PC += 2;
            break;
        case 0x8000: // 8xyn
            switch (n) {
                case 0x0:
                    V[x] = V[y]; //8xy0 - LD Vx,Vy
                    break;
                case 0x1:
                    V[x] = V[x] | V[y]; //8xy1 - OR Vx,Vy
                    break;
                case 0x2:
                    V[x] = V[x] & V[y]; //8xy2 - AND Vx,Vy
                    break;
                case 0x3:
                    V[x] = V[x] ^ V[y]; //8xy3 - XOR Vx,Vy
                    break;
                case 0x4:
                    V[0xF] = ((int) V[x] + (int) V[y]) > 255 ? 1 : 0;
                    V[x] = V[x] + V[y]; // 8xy4 - ADD Vx,Vy
                    break;
                case 0x5:
                    V[0xF] = (V[x] > V[y]) ? 1 : 0; // 8xy5 - SUB Vx,Vy
                    break;
                case 0x6:
                    V[0xF] = V[x] & 0x1;
                    V[x] = (V[x] >> 1); // 8xy6 - SHR Vx {,Vy}
                    break;
                case 0x7:
                    V[0xF] = (V[y] > V[x]) ? 1 : 0;
                    V[x] = V[y] - V[x]; // 8xy7 - SUBN Vx,Vy
                    break;
                case 0xE:
                    V[0xF] = (V[x] >> 7) & 0x1;
                    V[x] = (V[x] << 1); // 8xyE - SHL Vx,{Vy}
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
        case 0x900:
            switch (n) {
                case 0x0:
                    PC += (V[x] != V[y]) ? 4 : 2; // 9xy0 - SNE Vx,Vy
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
        case 0xA000:
            I = nnn; // Annn - LD I,addr
            PC += 2;
            break;
        case 0xB000:
            PC = V[0] + nnn; // Bnnn - JP V0,addr
            break;
        case 0xC000:
            V[x] = kk & randbyte(); // Cnnn - RND Vx,byte
            PC += 2;
            break;
        case 0xD000:
            draw_sprite(V[x], V[y], n);
            PC += 2;
            chip8_draw_flag = true;
            break;
        case 0xE000:
            switch (kk) {
                case 0x9E:
                    PC += (key[V[x]]) ? 4 : 2; // Ex9E - SKP Vx
                    break;
                case 0xA1:
                    PC += (!key[V[x]]) ? 4 : 2; //ExA1 - SKNP Vx
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
        case 0xF000:
            switch (kk) {
                case 0x07:
                    V[x] = delay_timer; // Fx07 - LD Vx,DT
                    PC += 2;
                    break;
                case 0x0A:
                    i = 0;
                    while (true) {
                        for (i = 0; i < KEY_SIZE; i++) {
                            if (key[i]) {
                                V[x] = i; // FX0A - LD Vx,K
                                goto key_pressed;
                            }
                        }
                    }
                key_pressed:
                    PC += 2;
                    break;
                case 0x15:
                    delay_timer = V[x];  //Fx15 - LD DT,Vx
                    PC += 2;
                    break;
                case 0x18:
                    sound_timer = V[x];  //Fx18 - LD ST,Vx
                    PC += 2;
                    break;
                case 0x1E:
                    V[0xF] = (I + V[x] > 0xfff) ? 1 : 0;
                    I = V[x] + I;// Fx1E - ADD I,Vx
                    PC += 2;
                    break;
                case 0x29:
                    I = FONTSET_BYTES_CHAR * V[x];  // Fx29 - LD F,Vx
                    PC += 2;
                    break;
                case 0x33:
                    memory[I] = (V[x] % 1000) / 100;
                    memory[I + 1] = (V[x] % 100) / 10;
                    memory[I + 2] = (V[x] % 10); // Fx33 - LD B,Vx
                    PC += 2;
                    break;
                case 0x55:
                    for (i = 0; i <= x; i++) { memory[I + 1] = V[i]; }
                    I += x + 1;  // Fx55 - LD[I],Vx
                    PC += 2;
                    break;
                case 0x65:
                    for (i = 0; i <= x; i++) { V[i] = memory[I + 1]; }
                    I += x + 1;  // Fx55 - LD[I],Vx
                    PC += 2;
                    break;
                default:
                    unknown_opcode(opcode);
            }
            break;
    }


}

