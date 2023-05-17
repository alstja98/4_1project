#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_INNINGS 9
#define MAX_DIGITS 4

// Global variables
int answer[MAX_DIGITS]; // CPU가 생성한 4자리 숫자
int currentInning = 1; // 현재 이닝
int strikes, balls; // 현재 이닝에서의 Strikes와 Balls 개수

// Memory mapping addresses
#define FND_CS0 0x11000000
#define FND_CS1 0x11100000
#define FND_CS2 0x11200000
#define FND_CS3 0x11300000
#define FND_CS4 0x11400000
#define FND_CS5 0x11500000
#define FND_CS6 0x11600000
#define FND_CS7 0x11700000
#define FPGA_DOT_COL1 0x11800000
#define FPGA_DOT_COL2 0x11900000
#define FPGA_DOT_COL3 0x11A00000
#define FPGA_DOT_COL4 0x11B00000
#define FPGA_DOT_COL5 0x11C00000
#define FPGA_CLCD_WR 0x12300000
#define FPGA_CLCD_RS 0x12380000
#define KEY_IN_ADDR 0x11E00000
#define KEY_OUT_ADDR 0x11D00000

// Additional global variables for new components
int lastInning[MAX_INNINGS][3]; // Array to store the results of each inning
int numLives = 7; // Number of lives remaining
int hiddenCoinUsed = 0; // Flag to track if hidden coin has been used

// Function prototypes
void initializeGame();
void generateAnswer();
void playGame();
void getInput(int *input);
void checkGuess(int *guess);
void checkLastInning();
void updateLastInning(int inning, int strikes, int balls);
void showLastInning(int inning);
void displayCLCDMessage(const char *message);
void displayFNDNumbers(int *numbers);
void displayDotMatrixAnimation();
void updateLEDs(int lives);
void writeMemory(int address, int data);
int readMemory(int address);

int main() {
    initializeGame();
    playGame();

    return 0;
}

void initializeGame() {
    srand(time(NULL)); // 시간을 기반으로 난수 생성기 초기화
    generateAnswer();
    
    // Additional initialization code for new components
    displayCLCDMessage("Bulls and Cows / Press Enter");
    displayFNDNumbers(answer);
    displayDotMatrixAnimation();
    updateLEDs(numLives);
}

void generateAnswer() {
    // CPU가 무작위로 중복되지 않는 4자리 숫자 생성
    int digits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int i, j, temp;

    for (i = 0; i < MAX_DIGITS; i++) {
        j = rand() % (10 - i) + i;
        temp = digits[i];
        digits[i] = digits[j];
        digits[j] = temp;
        answer[i] = digits[i];
    }
}



void playGame() {
    while (currentInning <= MAX_INNINGS && numLives > 0) {
        int guess[MAX_DIGITS];
        getInput(guess);
        checkGuess(guess);

        if (strikes == MAX_DIGITS) {
            // 정답인 경우
            printf("Congratulations! You won the game in %d innings.\n", currentInning);
            // Additional code for new components
            displayCLCDMessage("Game over // you win 'n'th");
            displayFNDNumbers(answer);
            displayDotMatrixAnimation();
            updateLEDs(numLives);
            break;
        } else {
            printf("%dth Inning // %dS %dB\n", currentInning, strikes, balls);
            // Additional code for new components
            updateLastInning(currentInning, strikes, balls);
            currentInning++;
        }
    }

    if (currentInning > MAX_INNINGS || numLives == 0) {
        // 게임 오버
        printf("Game over! You lost the game.\n");
        // Additional code for new components
        displayCLCDMessage("Game over // you lose");
        displayFNDNumbers(answer);
        displayDotMatrixAnimation();
        updateLEDs(0);
    }
}

void getInput(int *input) {
    printf("Enter your guess (4 digits): ");
    scanf("%1d%1d%1d%1d", &input[0], &input[1], &input[2], &input[3]);
}

void checkGuess(int *guess) {
    strikes = 0;
    balls = 0;

    // 입력한 숫자와 정답을 비교하여 Strikes와 Balls 개수 계산
    int i, j;
    for (i = 0; i < MAX_DIGITS; i++) {
        if (guess[i] == answer[i]) {
            strikes++;
        } else {
            for (j = 0; j < MAX_DIGITS; j++) {
                if (guess[i] == answer[j]) {
                    balls++;
                    break;
                }
            }
        }
    }

    // Additional code for new components
    displayCLCDMessage("Last inning // 'X'S 'X'B");
    displayFNDNumbers(guess);
    displayDotMatrixAnimation();
    updateLEDs(numLives);

    checkLastInning();
}

void checkLastInning() {
    if (currentInning > 1) {
        printf("Show last inning? (Y/N): ");
        char choice;
        scanf(" %c", &choice);

        if (choice == 'Y' || choice == 'y') {
            int inning;
            printf("Enter inning number (1-%d): ", currentInning - 1);
            scanf("%d", &inning);
            showLastInning(inning);
        }
    }
}

void updateLastInning(int inning, int strikes, int balls) {
    // Update the last inning array with the current inning's results
    lastInning[inning][0] = inning;
    lastInning[inning][1] = strikes;
    lastInning[inning][2] = balls;
}

void showLastInning(int inning) {
    if (inning >= 1 && inning < currentInning) {
        int strikes = lastInning[inning][1];
        int balls = lastInning[inning][2];
        printf("%dth Inning // %dS %dB\n", inning, strikes, balls);
    } else {
        printf("Invalid inning number.\n");
    }
}

void displayCLCDMessage(const char *message) {
    // Write command to CLCD control register
    writeMemory(FPGA_CLCD_WR, 0x1); // Clear display

    // Write data to CLCD data register
    int i; // 세미콜론 추가
    for (i = 0; message[i] != '\0'; i++) {
        writeMemory(FPGA_CLCD_RS, message[i]);
    }
}


void displayFNDNumbers(int *numbers) {
    // Write numbers to FND display using memory mapping
    int i;
    for (i = 0; i < MAX_DIGITS; i++) {
        int fndAddress = FND_CS0 + (i * 0x10000);
        writeMemory(fndAddress, numbers[i]);
    }
}

void displayDotMatrixAnimation() {
    // Display dot matrix animation using memory mapping
    // Implementation not provided, please add your own code here
}

void updateLEDs(int lives) {
    // Update LEDs based on the number of lives remaining
    // Implementation not provided, please add your own code here
}

void writeMemory(int address, int data) {
    // Write data to the specified memory address using memory mapping
    // Implementation not provided, please add your own code here
}

int readMemory(int address) {
    // Read data from the specified memory address using memory mapping
    // Implementation not provided, please add your own code here
}