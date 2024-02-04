#ifndef CHIP_8_TFG_MAIN_H
#define CHIP_8_TFG_MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define true 1
#define false 0
#define bool int

#define MEMORY_SIZE 4096
#define DP_ROWS 32
#define DP_COLUMNS 64
#define DP_SIZE (DP_COLUMNS*DP_ROWS)
#define STACK_SIZE 16
#define KEY_SIZE 16

#define GAME_LIMIT_SIZE (0x1000 - 0x200)

void chip_8_ini();
void chip_8_games();
void chip_8_opcycle();
void chip_8_keys();
void chip_8_tick();



#endif //CHIP_8_TFG_MAIN_H
