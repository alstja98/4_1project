#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "device.h"

#define MAX_INNINGS 9
#define MAX_DIGITS 4
#define NUM_LEDS 7
#define LED_ADDRESS(i) (FPGA_LED + i)

// Global variables
int answer[MAX_DIGITS]; // CPU가 생성한 4자리 숫자
int currentInning = 1; // 현재 이닝
int strikes, balls; // 현재 이닝에서의 Strikes와 Balls 개수
int lastInning[MAX_INNINGS][3]; // Array to store the results of each inning
int numLives = 7; // Number of lives remaining
int hiddenCoinUsed = 0; // Flag to track if hidden coin has been used


// general functions declare
void initializeGame();
void generateAnswer();
void playGame();
void getInput(int *input);
void checkGuess(int *guess);
void checkLastInning();
void updateLastInning(int inning, int strikes, int balls);
void showLastInning(int inning);
void displayFNDNumbers(int *numbers);

// thread functions
void run_in_parallel(void *(*func1)(void *), void *(*func2)(void *), void *(*func3)(void *), void *(*func4)(void *), void *(*func5)(void *));
void *DOT_Timer_Thread(void *arg);
void *DOT_Baseball_Thread(void *arg);
void *CLCD_Display_Thread(void *arg);
// void *Thread_Func3(void *arg);
// void *Thread_Func4(void *arg);
// void *Thread_Func5(void *arg);


// general functions define
int main() {
    // devices initialization 완료
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
    
    printf("Game Start!\n");

    initializeGame();
    printf("initialize test finish");
    // playGame();

    // 병렬처리 예시 
    // run_in_parallel(DOT_Timer_Thread, CLCD_Display_Thread,NULL, NULL, NULL);

    return 0;
}

void initializeGame() {
    srand(time(NULL)); // 시간을 기반으로 난수 생성기 초기화
    generateAnswer();
    printf("Answer testing: %d%d%d%d\n", answer[0], answer[1], answer[2], answer[3]);
    // Additional initialization code for new components
    // displayCLCDMessage(len1, len2, CG_or_DD, buf1, buf2);
    run_in_parallel(DOT_Baseball_Thread, CLCD_Display_Thread,NULL, NULL, NULL);
    usleep(3000000);
    // displayFNDNumbers(answer);
    // displayDotMatrixAnimation();
    // updateLEDs(numLives);
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
            int len1 = 9, len2 = 15, CG_or_DD = 1;
            char buf1[100]="Game over";
            char buf2[100]="you win";
            snprintf(buf2, sizeof(buf2), "%d inning", currentInning);
            // 정답인 경우
            printf("Congratulations! You won the game in %d innings.\n", currentInning);
            // Additional code for new components
            // displayCLCDMessage(len1, len2, CG_or_DD, buf1, buf2);
            // displayFNDNumbers(answer);
            // displayDotMatrixAnimation();
            // updateLEDs(numLives);
            break;
        } else {
            printf("%dth Inning // %dS %dB\n", currentInning, strikes, balls);
            // Additional code for new components
            updateLastInning(currentInning, strikes, balls);
            currentInning++;
        }
    }

    if (currentInning > MAX_INNINGS || numLives == 0) {
        int len1 = 9, len2 = 8, CG_or_DD = 1;
        char buf1[100]="Game over", buf2[100]="You lose";
        // 게임 오버
        printf("Game over! You lost the game.\n");
        // Additional code for new components
        // displayCLCDMessage(len1, len2, CG_or_DD, buf1, buf2);
        // displayFNDNumbers(answer); // fnd 수정해야함
        // displayDotMatrixAnimation(); // 게임 오버 애니메이션으로 수정해야함
        // updateLEDs(0); // led 수정해야함
    }
}

// 이거가 사용자가 숫자를 입력하는 함수인데, keypad를 활용할 수 있게 수정해야함
void getInput(int *input) {
    printf("Enter your guess (4 digits): ");
    scanf("%1d%1d%1d%1d", &input[0], &input[1], &input[2], &input[3]); // keypad로 받을수있게.
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

    int len1 = 8, len2 = 5, CG_or_DD = 1;
    char buf1[100];
    snprintf(buf1, sizeof(buf1), "%d inning", currentInning);
    char buf2[100];
    snprintf(buf2, sizeof(buf2), "%dS %dB", strikes, balls);

    // displayCLCDMessage(len1, len2, CG_or_DD, buf1, buf2); //CLCD 수정완료
    // displayFNDNumbers(guess); //수정해야함
    // displayDotMatrixAnimation(); //35초 타이머 애니메이션 돌아감
    // updateLEDs(numLives); //수정해야함

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


// thread functions declare

// The generic function to run any two functions in parallel
void run_in_parallel(void *(*func1)(void *), void *(*func2)(void *), void *(*func3)(void *), void *(*func4)(void *), void *(*func5)(void *)) {
    pthread_t tid1, tid2, tid3, tid4, tid5;

    int t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0;

    if (func1 && pthread_create(&tid1, NULL, func1, NULL) == 0) {
        t1 = 1;
    }

    if (func2 && pthread_create(&tid2, NULL, func2, NULL) == 0) {
        t2 = 1;
    }

    if (func3 && pthread_create(&tid3, NULL, func3, NULL) == 0) {
        t3 = 1;
    }

    if (func4 && pthread_create(&tid4, NULL, func4, NULL) == 0) {
        t4 = 1;
    }

    if (func5 && pthread_create(&tid5, NULL, func5, NULL) == 0) {
        t5 = 1;
    }

    if (t1) {
        pthread_join(tid1, NULL);
    }

    if (t2) {
        pthread_join(tid2, NULL);
    }

    if (t3) {
        pthread_join(tid3, NULL);
    }

    if (t4) {
        pthread_join(tid4, NULL);
    }

    if (t5) {
        pthread_join(tid5, NULL);
    }
}


void *DOT_Timer_Thread(void *arg) {
    DOT_Timer();
    return NULL;
}

void *DOT_Baseball_Thread(void *arg) {
    DOT_Display_Baseball();
    return NULL;
}

void *CLCD_Display_Thread(void *arg) {
    // Parameters would normally be passed via arg, but for simplicity, I'll define them directly here
    int len1 = 14, len2 = 11, CG_or_DD = 1;
    char buf1[100]="Bulls and Cows", buf2[100]="Press Enter";
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}

