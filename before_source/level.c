#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "level.h"


int TemptLevel[LEVEL_SIZE] = { 0, };
int Level[LEVEL_SIZE] = { 0, };
int Level_Index = 0;


int	levelGetFirst() {
	return TemptLevel[Level_Index];
}

void levelInit() {
	srand((unsigned int)time(NULL));

	int i = 0;
	for (i = 0; i < LEVEL_SIZE; i++) {
		TemptLevel[i] = LEVEL_ROAD;
		Level[i] = LEVEL_ROAD;
		if ( i > 6) {
			TemptLevel[i] = rand() % 2;
			if ((TemptLevel[ i-1 ] && TemptLevel[ i -2] && TemptLevel[i-3]) == 1) TemptLevel[ i ] = LEVEL_ROAD;
			Level[i] = TemptLevel[i];
		}
	}

	Level_Index = 0;
}

void levelUpdate() {
	int i;

	TemptLevel[Level_Index] = rand() % 2;
	if ((TemptLevel[(Level_Index - 1 + LEVEL_SIZE) % LEVEL_SIZE] | TemptLevel[(Level_Index - 2 + LEVEL_SIZE) % LEVEL_SIZE] | TemptLevel[(Level_Index - 3 + LEVEL_SIZE) % LEVEL_SIZE] | TemptLevel[(Level_Index - 4  + LEVEL_SIZE) % LEVEL_SIZE] | TemptLevel[(Level_Index - 5 + LEVEL_SIZE) % LEVEL_SIZE]) != 1) TemptLevel[Level_Index] = LEVEL_TRAP;
	if ((TemptLevel[(Level_Index - 1 + LEVEL_SIZE) % LEVEL_SIZE] && TemptLevel[(Level_Index - 2 + LEVEL_SIZE) % LEVEL_SIZE] && TemptLevel[(Level_Index - 3 + LEVEL_SIZE) % LEVEL_SIZE] ) == LEVEL_TRAP) TemptLevel[Level_Index] = LEVEL_ROAD;

	Level_Index = (Level_Index + 1) % LEVEL_SIZE;

	for (i = 0; i < LEVEL_SIZE; i++) {
		Level[i] = TemptLevel[(Level_Index + i) % LEVEL_SIZE];
	}
}

int *levelGetList() {
	return Level;
}
