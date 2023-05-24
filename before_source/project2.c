/* project2.c */
#include <stdio.h>
#include <time.h>
#include "device.h"
#include "level.h"

//
// Definition
//
#define LEVEL_UPDATE_TIME	500
#define CLEAR_SCORE 		1000

//
// Type definition
//
typedef bool (*StateFunc)(float);

typedef enum {
	STATE_START	= 0,
	STATE_RUN,
	STATE_GAMEOVER,
	STATE_CLEAR,
	STATE_COUNT
} GAME_STATE;

typedef enum {
	STATE_NONE = 0,
	STATE_WALK,
	STATE_JUMP
} PLAYER_STATE;

typedef enum {
	ANIM_IDLE = 0,
	ANIM_RUN1 = 1,
	ANIM_RUN2 = 2,
	ANIM_JUMP = 3,
	ANIM_OVER = 4,
	ANIM_CLEAR = 5,
	ANIM_COUNT
} DOT_ANIMS;

typedef struct {
	float needTime;
	float animTime;
	float scoreMod;
} DiffInfo;

typedef struct {
	GAME_STATE currentState;
	PLAYER_STATE playerState;
	int jumpCount;
	double elapsedTime;
	int currentScore;
	int difficulty;
	int topScore;
} GameInfo;


//
// Declare functions
//
bool StartState(float);
bool RunState(float);
bool GameOverState(float);
bool ClearState(float);

void SetPlayerState(PLAYER_STATE);

void Run_UpdateLevel(float);
void Run_DrawLevel();
void Run_DrawPlayer();
void Run_DrawDOT(float);
bool Run_CheckInput();

//
// Global variables
//
ushort DOT_ANIM[ANIM_COUNT][5] = {
	{ 0x09, 0x12, 0x3C, 0x12, 0x09 },	// IDLE
	{ 0x24, 0x12, 0x3C, 0x12, 0x09 },	// RUN #1
	{ 0x12, 0x12, 0x3C, 0x13, 0x10 },	// RUN #2
	{ 0x12, 0x2C, 0x78, 0x2A, 0x44 }, 	// JUMP
	{ 0x01, 0x07, 0x04, 0x07, 0x04 },	// GAME_OVER
	{ 0x40, 0x27, 0x7C, 0x27, 0x40 }	// CLEAR!
};

ushort CLCD_ANIM[ANIM_COUNT][8] = {
	{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },  // GROUND
	{ 0x06, 0x14, 0x0E, 0x05, 0x14, 0x0A, 0x01, 0x00 },  // RUN #1
	{ 0x06, 0x04, 0x1F, 0x04, 0x04, 0x1A, 0x02, 0x00 },  // RUN #2
	{ 0x06, 0x05, 0x0E, 0x14, 0x06, 0x09, 0x12, 0x00 },  // JUMP
};

DiffInfo GameData[9] = {
	{ 10000, 500, 1.0 },	// Level 0: next level after 10s
	{ 10000, 400, 1.1 },	// Level 1: next level after 10s
	{ 10000, 300, 1.2 },	// Level 2: next level after 10s
	{ 10000, 200, 1.3 },	// Level 3: next level after 10s
	{ 10000, 100, 1.4 },	// Level 4: next level after 10s
	{ 10000, 80, 1.5 },		// Level 5: next level after 10s
	{ 10000, 60, 1.6 },		// Level 6: next level after 10s
	{ 10000, 40, 1.7 },		// Level 7: next level after 10s
	{ 10000, 20, 1.8 }		// Level 8: no more level 
};

// State function list
StateFunc StateList[STATE_COUNT] = {
	StartState,
	RunState,
	GameOverState,
	ClearState
};

// Global game information
GameInfo g_info = {
	STATE_START,	// Default Game State
	STATE_NONE,		// Default Player State
	0,				// Default Jump Count
	0,				// Default Elapsed Time
	0,				// Default Current Score
	0,				// Default Difficulty
	0,				// Default Top Score
};

int main(int argc, char** argv)
{	
	ushort keyin = 0;
	int result = initDevices(DEVICE_ALL);
	if (result == FAIL_MEMORY_OPEN) {
		perror("Could not open 'dev/mem'.");
		return -1;
	}

	if ((result & FAIL_INIT_DEVICES) == FAIL_INIT_DEVICES) {		
		perror("Could not initialize devices.");
		closeDevices();
		return -1;
	}

	// User Font Setting
	CLCD_SetUserFont(0, CLCD_ANIM[0]);
	CLCD_SetUserFont(1, CLCD_ANIM[1]);
	CLCD_SetUserFont(2, CLCD_ANIM[2]);
	CLCD_SetUserFont(3, CLCD_ANIM[3]);

	bool isEnd = FALSE;
	clock_t start, end;

	while (!isEnd) {
		float elapsedTime = getElapsedTime();
		isEnd = StateList[g_info.currentState](elapsedTime);
		usleep(100);
	}

	CLCD_PrintFit("Thank you\nfor playing!");
	DOT_Clear();
	closeDevices();

	return 0;
}

bool StartState(float elapsed)
{
	// Show Terminal
	printf("\n## CAU RUNNER ##\n");
	printf("Welcome to our game which call \"CAU RUNNER\"!\n");
	printf("You can start the game to press any keypad.\n");

	// Show CLCD
	CLCD_PrintFit("CAU RUNNER\nPress to start");
	AllFND_Clear();
	AllLED_Off();
	DOT_Write(DOT_ANIM[ANIM_IDLE]);

	ushort key = GetKeypadWait();
	if (key == KEYPAD_3_3) return TRUE;

	// Show Message
	printf("\n## GAME START ##\n");
	printf("How to control:\n");
	printf("Press column 1: Jump a tile.\n");
	printf("Press column 2: Jump two tiles.\n");
	printf("Press column 3: Jump three tiles.\n");
	printf("Press keypad 4x4: Quit the game.\n");

	// initialize game state
	g_info.elapsedTime 	= 0;
	g_info.currentScore = 0;	
	g_info.difficulty 	= 0;

	DOT_Write(DOT_ANIM[ANIM_RUN1]);
	LEDOnFromBottom(g_info.difficulty);

	g_info.currentState = STATE_RUN;

	// initialize player state
	g_info.playerState = STATE_WALK;
	g_info.jumpCount = 0;

	// initialize a level and draw a level.
	levelInit();
	CLCD_Clear();
	Run_DrawLevel();
	Run_DrawPlayer();

	// for initializing elapsed time.
	getElapsedTime();

	return FALSE;
}

bool RunState(float elapsed)
{
	g_info.elapsedTime += elapsed;

	// Update game level.
	Run_UpdateLevel(elapsed);
	Run_DrawLevel();
	
	// Draw current score: 10 points per 250ms
	g_info.currentScore = (int)(g_info.elapsedTime / 250 * GameData[g_info.difficulty].scoreMod) * 10;
	if (g_info.currentScore >= CLEAR_SCORE) {
		g_info.currentScore = CLEAR_SCORE;
		g_info.currentState = STATE_CLEAR;
	}
	FND_DrawNumber(g_info.currentScore);

	// Draw animation and other informations.
	Run_DrawDOT(elapsed);

	// Get first level and check player state.
	int currentLevel = levelGetFirst();

	// Check state of player.
	if ((currentLevel == LEVEL_TRAP) && (g_info.playerState != STATE_JUMP) && (g_info.currentState != STATE_CLEAR)) {
		g_info.currentState = STATE_GAMEOVER;
	}

	// Check user input, if user input KEYPAD_3_3, game will be quit.
	if (Run_CheckInput()) return TRUE;

	return FALSE;	
}

bool GameOverState(float elapsed)
{
	char score[64];

	// Update top score.
	if (g_info.topScore < g_info.currentScore) g_info.topScore = g_info.currentScore;

	// Show on Board
	sprintf(score, "TOP SCORE:%6dYOU SCORE:%6d", g_info.topScore, g_info.currentScore);
	CLCD_PrintFit(score);
	AllFND_Clear();
	AllLED_Off();
	DOT_Write(DOT_ANIM[ANIM_OVER]);

	// Show on terminal
	printf("\n## GAME OVER ##\n");
	printf("Top Score: %d\n", g_info.topScore);
	printf("Your Score: %d\n", g_info.currentScore);
	printf("Press any keypad to restart.\n");	

	unsigned int key = GetKeypadWait();
	if (key == KEYPAD_3_3) return TRUE;

	g_info.currentState = STATE_START;
	return FALSE;
}

bool ClearState(float elapsed)
{
	// Show on Board
	CLCD_PrintFit("You are the bestPress to restart");
	AllFND_Clear();
	AllLED_Off();
	DOT_Write(DOT_ANIM[ANIM_CLEAR]);

	// Show on terminal
	printf("\n## GAME CLEAR ##\n");
	printf("You are the best!\n");
	printf("Press any keypad to restart.\n");
	
	unsigned int key = GetKeypadWait();
	if (key == KEYPAD_3_3) return TRUE;
	
	g_info.currentState = STATE_START;

	return FALSE;
}

void SetPlayerState(PLAYER_STATE state)
{
	g_info.playerState = state;	
	Run_DrawPlayer();
	Run_DrawDOT(1000);
}

void Run_UpdateLevel(float elapsed)
{
	static float gameElapsed = 0, difElapsed = 0;

	// Update game level
	gameElapsed += elapsed;
	if (gameElapsed >= (LEVEL_UPDATE_TIME * (2.0 - GameData[g_info.difficulty].scoreMod))) {
		levelUpdate();
		Run_DrawLevel();

		// Update player state
		switch (g_info.playerState) {
			case STATE_JUMP:
				g_info.jumpCount--;
				if (g_info.jumpCount <= 0) {
					SetPlayerState(STATE_WALK);
					break;
				}
				break;
		}

		gameElapsed = 0;
	}

	// Update difficulty
	difElapsed += elapsed;
	if (difElapsed >= GameData[g_info.difficulty].needTime) {
		difElapsed = 0;
		g_info.difficulty++;
		if (g_info.difficulty > 8) g_info.difficulty = 8;

		LEDOnFromBottom(g_info.difficulty);		
	}
}

void Run_DrawLevel()
{		
	char tile[2] = { 0x0, ' ' };
	int* level = levelGetList();	
	int count = 0;

	CLCD_SetCursorPos(0x40);
	for (count = 0; count < LEVEL_SIZE; count++)
		CLCD_Put(tile[*level++]);
}

void Run_DrawPlayer()
{
	char player[3] = { 0x0, 0x1, 0x3 };

	CLCD_SetCursorPos(0x0);
	CLCD_Put(player[g_info.playerState]);
}

void Run_DrawDOT(float elapsed)
{
	static float runElapsed = 0;
	static int runIndex = 0;

	runElapsed += elapsed;
	switch (g_info.playerState)
	{
		case STATE_WALK:			
			if (runElapsed >= GameData[g_info.difficulty].animTime) {
				runElapsed = 0;
				runIndex = (runIndex + 1) % 2;
				DOT_Write(DOT_ANIM[ANIM_RUN1 + runIndex]);
				CLCD_SetUserFont(1, CLCD_ANIM[ANIM_RUN1 + runIndex]);
			}
			break;

		case STATE_JUMP:
			DOT_Write(DOT_ANIM[ANIM_JUMP]);
			break;
	}
}

bool Run_CheckInput()
{
	ushort key = GetKeypad();

	// Jump a trap.
	if ((key & KEYPAD_0) && (g_info.playerState == STATE_WALK)) {
		SetPlayerState(STATE_JUMP);
		g_info.jumpCount = 2;
	}

	// Jump two trap.
	if ((key & KEYPAD_1) && (g_info.playerState == STATE_WALK)) {
		SetPlayerState(STATE_JUMP);
		g_info.jumpCount = 3;
	}

	// Jump three trap.
	if ((key & KEYPAD_2) && (g_info.playerState == STATE_WALK)) {
		SetPlayerState(STATE_JUMP);
		g_info.jumpCount = 4;
	}

	if ((key & (KEYPAD_3_0 | KEYPAD_3_1)) == (KEYPAD_3_0 | KEYPAD_3_1)) {
		SetPlayerState(STATE_JUMP);
		g_info.jumpCount = 2;
	}

	// Quit game
	if (key == KEYPAD_3_3) return TRUE;

	return FALSE;
}
