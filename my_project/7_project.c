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
int lastInning[MAX_INNINGS][3]; // 이닝별 Strikes와 Balls 결과 저장하는 배열
int numLives = 7; // 목숨 개수. 처음에 7개
int hiddenCoinUsed = 0; // Flag to track if hidden coin has been used


// game functions declare
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
void *ALLLED_Blink(void *arg);
void *DOT_Timer_Thread(void *arg);
void *DOT_Baseball_Thread(void *arg);
void *CLCD_Display_Thread(void *arg);
void *CLCD_Display_Thread_GL(void *arg);
void *CLCD_Display_Thread_Ingame(void *arg);
void *CLCD_Display_Thread_FinishWin(void *arg);


int main() {
    // devices initialization
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
    
    initializeGame();
    // 아래는 테스트 해볼 함수들
    // DOT_Display_Baseball();
    // ALLLED_Blink();
    // AlternateLEDBlink();
    // TurnOffTopLED();
    // run_in_parallel(CLCD_Display_Thread_Ingame, NULL, NULL, NULL, NULL);
    printf("initialize test finish");
    // playGame();
    DOT_Clear();
	closeDevices();
    return 0;
}

void initializeGame() { // 1단계
    srand(time(NULL)); // 시간을 기반으로 난수 생성기 초기화
    generateAnswer();
    printf("Answer testing: %d%d%d%d\n", answer[0], answer[1], answer[2], answer[3]); // answer 잘 만들어지는지 테스트

    run_in_parallel(DOT_Baseball_Thread, CLCD_Display_Thread, ALLLED_Blink, NULL, NULL);
    while (1) {
        if (IsKeypadPressed(3, 2)) { // Enter (3,2)위치의 버튼을 누르면
            playGame();
            break;
        }
        usleep(1000); // Wait for a bit to prevent excessive CPU usage
    }
    // touchpad의 enter를 누를 시에 playGame()으로 이동해야함 - 완료
    // led는 우선 모든 요소가 1초 간격으로 깜박이게 출력하게 임시로 테스트
    // led는 1,3,5,7 번쨰와 2,4,6 번째 LED가 1초 간격으로 서로 번갈아가도록 출력
    // fnd는 모든 요소가 1초간격으로 깜박이게 출력
}

void generateAnswer() { // 잘 실행됨.
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
            // 정답을 맞춘 경우. 게임 종료
            run_in_parallel(NULL, CLCD_Display_Thread_FinishWin, ALLLED_Blink, NULL, NULL);
            break;
        } else {
            // 그 다음 이닝으로 넘어간다.
            // 이전의 strikes, balls 정보를 lastInning 배열에 저장한다.
            // currentInning도 업데이트 한다.
            // led 개수를 하나 감소한다.
            // dot는 타이머로 돌아옴
            run_in_parallel(NULL, CLCD_Display_Thread_Ingame, TurnOffTopLED, NULL, NULL);
            // Additional code for new components
            updateLastInning(currentInning, strikes, balls); // 이닝별 스트라이크, 볼 개수 저장
            currentInning++;
        }
    }

    if (currentInning > MAX_INNINGS || numLives == 0) { // 9회까지 진행했거나 목숨이 0개가 되었을 때
        int len1 = 9, len2 = 8, CG_or_DD = 1;
        char buf1[100]="Game over", buf2[100]="You lose";
        // 게임 오버
        printf("Game over! You lost the game.\n");
        // Additional code for new components
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


// thread functions implement
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

void *ALLLED_Blink(void *arg) {
    ALLLED_Blink();
    return NULL;
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
    // 맨 처음 시작할 때
    int len1 = 14, len2 = 11, CG_or_DD = 1;
    char buf1[100]="Bulls and Cows", buf2[100]="Press Enter";
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_GL(void *arg) {
    // 맨 처음 시작할 때
    int len1 = 9, len2 = 1, CG_or_DD = 1;
    char buf1[100]="Good Luck!", buf2[100]=" ";
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_Ingame(void *arg) {
    char buf1[100], buf2[100];
    snprintf(buf1, sizeof(buf1), "%dth inning", currentInning);
    snprintf(buf2, sizeof(buf2), "%dS %dB", strikes, balls);
    int len1 = strlen(buf1), len2 = strlen(buf2);
    int CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_FinishWin(void *arg) {
    char buf1[100];
    snprintf(buf1, sizeof(buf1), "%dth inning", currentInning);
    char buf2[100]="You Win!";
    int len1 = strlen(buf1), len2 = strlen(buf2);
    int CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}

