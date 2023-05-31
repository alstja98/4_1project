#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "device.h"
#include <pthread.h>

#define MAX_INNINGS 11
#define MAX_DIGITS 4
#define LED_ADDRESS(i) (FPGA_LED + i)

// Global variables
int answer[MAX_DIGITS]; // CPU가 생성한 4자리 숫자
int currentInning = 1; // 현재 이닝
int displayedInning = 0;
int strikes, balls; // 현재 이닝에서의 Strikes와 Balls 개수
int lastInning[MAX_INNINGS][3]; // [이닝별] [입력4숫자,Strikes,Balls] 결과 저장하는 배열
int numLives = 8; // 목숨 개수. 처음에 8개
int hiddenCoinUsed = 0; // Flag to track if hidden coin has been used


// game functions declare
void initializeGame();
void generateAnswer();
void playGame();
void getInput(int *input);
void checkGuess(int *guess);
void checkLastInning();
void updateLastInning(int currentInning, int strikes, int balls, int *input);
void showLastInning(int inning);
void displayFNDNumbers(int *numbers);

// thread functions
void run_in_parallel(void *(*func1)(void *), void *(*func2)(void *), void *(*func3)(void *), void *(*func4)(void *), void *(*func5)(void *));
void *ALLLED_Blink_Thread(void *arg);
void *LEDOnFromBottomBasedOnLives_Thread(void *arg);
void *All_FND_Blink_Thread(void *arg);
void *Back4_FND_On_Thread(void *arg);
void *DOT_Baseball_Thread(void *arg);
void *CLCD_Display_Thread(void *arg);
void *CLCD_Display_Thread_GL(void *arg);
void *CLCD_Display_Thread_Denied(void *arg);
void CLCD_Display_Ingame(int inning);
void *CLCD_Display_Thread_FinishWin(void *arg);
void CLCD_Display_LastInning(int inning);


int main() {
    // devices initialization 테스트 완료
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

    DOT_Clear();
	closeDevices();
    return 0;
}

void initializeGame() { // 1단계
    srand(time(NULL)); // 시간을 기반으로 난수 생성기 초기화
    generateAnswer();
    printf("Game Start!\n");

    run_in_parallel(DOT_Baseball_Thread, CLCD_Display_Thread, ALLLED_Blink_Thread, All_FND_Blink_Thread, NULL);
    while (1) {
        if (IsKeypadPressed(2, 3)) { // keypad enter 버튼
            printf("Let's baseball game :)\n");
            playGame();
            break;
        }
        usleep(1000); 
    }

    // 1. answer 잘 만들어짐 확인 완료
    // 2. DOT_Display_Baseball()이 잘 실행됨 확인 완료
    // 3. CLCD_Display_Thread()가 잘 실행됨 확인 완료 (BULLS AND COWS )
    // 4. ALL LED BLINK 확인 완료함. 근데 몇번 하다가 맘
    // 5. ALL FND 도 잘 확인함.
    // 6. keypad 완료.

    // 문제 : run_in_parallel에서 넘어가지 않는다. 


    // touchpad의 enter를 누를 시에 playGame()으로 이동해야함 - 완료
    // led는 우선 모든 요소가 1초 간격으로 깜박이게 출력하게 - 완료
    // led는 1,3,5,7 번쨰와 2,4,6 번째 LED가 1초 간격으로 서로 번갈아가도록 출력 - 나중에 테스트
    // fnd는 모든 요소가 1초간격으로 깜박이게 출력 - 완료
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
    printf("answer: %d%d%d%d\n", answer[0], answer[1], answer[2], answer[3]);
}


void playGame() {
    while (currentInning <= MAX_INNINGS && numLives > 0) {
        int guess[MAX_DIGITS];
        if(currentInning == 1){
            run_in_parallel(CLCD_Display_Thread_GL, NULL, Back4_FND_On_Thread, LEDOnFromBottomBasedOnLives_Thread, NULL);
        }else{
            run_in_parallel(NULL, NULL, Back4_FND_On_Thread, LEDOnFromBottomBasedOnLives_Thread, NULL);
        }
        DOT_Inning(currentInning);
        // 이미 cpu가 생성한 answer가 있는 상태
        // clcd는 good luck 출력 -> 변화가 안됨
        // fnd는 뒤에 4개는 8888 출력하고, 앞에 4개는 출력 안함 -> 앞에 4개가 출력됨
        // led는 numLives 개수만큼 아래서부터 on -> 괜찮음
        getInput(guess);
        checkGuess(guess);

        if (strikes == MAX_DIGITS) {
            // 정답을 맞춘 경우. 게임 종료
            printf("You win!\n");
            run_in_parallel(NULL, CLCD_Display_Thread_FinishWin, ALLLED_Blink_Thread, NULL, NULL);
            FND_Show_Answer_win(answer, guess);
            break;
        } else {
            // 그 다음 이닝으로 넘어간다.
            // 이전의 strikes, balls 정보를 lastInning 배열에 저장한다.
            // currentInning도 업데이트 한다.
            // led 개수를 하나 감소한다.
            // dot는 타이머로 돌아옴
            updateLastInning(currentInning, strikes, balls, guess); // 이닝별 스트라이크, 볼 개수 저장
            CLCD_Display_Ingame(currentInning);
            TurnOffTopLED();
            currentInning++;
            numLives--;
            printf("lives left: %d\n", numLives);
        }
    }

    if (currentInning > MAX_INNINGS || numLives == 0) { // 9회까지 진행했거나 목숨이 0개가 되었을 때
        char buf1[100]="Game Over!";
        char buf2[100]="Play Again?";
        int len1 = strlen(buf1), len2 = strlen(buf2);
        int CG_or_DD = 1;
        CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
        FND_Show_Answer_lose(answer);
        printf("Game Over!\n");
    }
}

// 키패드로 숫자를 입력하고, 입력한 숫자를 배열에 저장하는 함수
void getInput(int *input) {
    int index = 0;
    int value = 0;

    while (index != 4) {
        ushort key = GetKeypadWait();
        if (key == 1) {
            value = 7;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 16) {
            value = 8;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 256) {
            value = 9;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 4096){
            if(index == 0){
                // 숫자를 입력한 적이 없는 경우만 가능함
                // current - 1 이닝의 결과를 보여줘야함
                // 이닝이 1인 경우에는 이전 이닝이 없으므로, denied 출력
                if(currentInning == 1){
                    CLCD_Clear();
                    char buf1[100]="Denied!", buf2[100]="Keep Pressed!";
                    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
                }else{
                    displayedInning = currentInning - 1 ;
                    CLCD_Display_LastInning(displayedInning);
                }
            }else{
                CLCD_Clear();
                char buf1[100]="Denied!", buf2[100]="Keep Pressed!";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }
        }else if (key == 2) {
            value = 4;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 32) {
            value = 5;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 512){
            value = 6;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 8192){
            if(index == 0){
                // 만약 currentInning이 1이면 기능 안됨
                // 만약 currentInning이 1보다 크면 맨처음누를때는 1이닝 결과부터 보여줌 tempInning = 1;
                // 이후에 next 누르는 경우에는 tempInning++;
                // 보여주는건 tempInning 을 보여줌
                if(displayedInning == 0 || displayedInning == 1){
                    CLCD_Clear();
                    char buf1[100]="Denied!", buf2[100]="No more before";
                    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
                }else{
                    displayedInning--;
                    CLCD_Display_LastInning(displayedInning);
                }
            }else{
                CLCD_Clear();
                char buf1[100]="Denied!", buf2[100]="Keep Pressed!";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }
        }else if (key == 4) {
            value = 1;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 64) {
            value = 2;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 1024) {
            value = 3;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 16384){
            if(index==0){
                 if(displayedInning == 0 || displayedInning >= currentInning-1){
                    CLCD_Clear();
                    char buf1[100]="Denied!", buf2[100]="No more after";
                    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
                 }else{
                    displayedInning++;
                    CLCD_Display_LastInning(displayedInning);
                 }
            }else{
                CLCD_Clear();
                char buf1[100]="Denied!", buf2[100]="Keep Pressed!";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }
        }else if (key == 8) {
            value = 0;
            input[index] = value;
            FND_DrawNumber(index, value);
            printf("input[%d] = %d\n", index, input[index]);
            index++;
        }else if (key == 128) { //DEL
            // 다른 숫자 눌러서 index가 늘어난상태니 하나 감소시키고
            // 가장 최근에 입력된 fnd 숫자 지움
            if(index == 0){
                CLCD_Clear();
                char buf1[100]="Denied!", buf2[100]="Press Number!";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }else{
                index--;
                FND_Clear(7-index);
            }
        }else if (key == 2048){
            // enter
            printf("You pressed enter.\n press keypad number 0-9\n");
            usleep(500000);
            continue;
        }else if (key == 32768){
            // 목숨 하나 추가
            // led 1개 on
            // 목숨은 3개 까지 추가 가능
            if(hiddenCoinUsed == 3){
                CLCD_Clear();
                char buf1[100]="Denied!", buf2[100]="No more Coin";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }else{
                char buf1[100] = "Hidden Coin Used!";
                char buf2[100];
                snprintf(buf2, sizeof(buf2), "%dCoin left", 2-hiddenCoinUsed);
                int len1 = strlen(buf1), len2 = strlen(buf2);
                int CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
                hiddenCoinUsed++;
                numLives++;
                LEDOnFromBottomBasedOnLives(numLives);
                printf("you earn one more life\n now you have %d lives\n", numLives);
            }
            usleep(500000);
            continue;
        }

        usleep(500000); 
    }
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
    printf("Innings: %d, Strikes: %d, Balls: %d\n", currentInning, strikes, balls);
}


void updateLastInning(int inning, int strike, int ball, int *input) {
    // Update the last inning array with the current inning's results
    int number = input[0]*1000 + input[1]*100 + input[2]*10 + input[3];
    lastInning[inning-1][0] = number;
    lastInning[inning-1][1] = strike;
    lastInning[inning-1][2] = ball;
    printf("update %d inning, %d N, %d S, %d B\n", inning, lastInning[inning-1][0], lastInning[inning-1][1], lastInning[inning-1][2]);
}


// thread functions implement
void run_in_parallel(void *(*func1)(void *), void *(*func2)(void *), void *(*func3)(void *), void *(*func4)(void *), void *(*func5)(void *)) {
    pthread_t tid1, tid2, tid3, tid4, tid5;

    if (func1) pthread_create(&tid1, NULL, func1, NULL);
    if (func2) pthread_create(&tid2, NULL, func2, NULL);
    if (func3) pthread_create(&tid3, NULL, func3, NULL);
    if (func4) pthread_create(&tid4, NULL, func4, NULL);
    if (func5) pthread_create(&tid5, NULL, func5, NULL);
}


// led threads
void *ALLLED_Blink_Thread(void *arg) {
    ALLLED_Blink();
    return NULL;
}

void *LEDOnFromBottomBasedOnLives_Thread(void *arg) {
    LEDOnFromBottomBasedOnLives(numLives);
    return NULL;
}

// fnd threads
void *All_FND_Blink_Thread(void *arg) {
    All_FND_Blink();
    return NULL;
}

void *Back4_FND_On_Thread(void *arg) {
    Back4_FND_On();
    return NULL;
}

// dot threads

void *DOT_Baseball_Thread(void *arg) {
    DOT_Display_Baseball();
    return NULL;
}

// clcd threads
void *CLCD_Display_Thread(void *arg) {
    // 맨 처음 시작할 때
    int len1 = 14, len2 = 11, CG_or_DD = 1;
    char buf1[100]="Bulls and Cows", buf2[100]="Press Enter";
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_GL(void *arg) {
    int len1 = 10, len2 = 14 , CG_or_DD = 1;
    char buf1[100]="Good Luck!", buf2[100]="Press 4 digits";
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_Denied(void *arg) {
    // 맨 처음 시작할 때
    char buf1[100]="Denied!", buf2[100]="Keep Pressed";
    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void CLCD_Display_Ingame(int inning) {
    char buf1[100], buf2[100];
    snprintf(buf1, sizeof(buf1), "Number: %04d", lastInning[inning-1][0]); 
    snprintf(buf2, sizeof(buf2), "Strike:%d Ball:%d", strikes, balls);
    int len1 = strlen(buf1), len2 = strlen(buf2);
    int CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_FinishWin(void *arg) {
    char buf1[100]="Congratulations!";
    char buf2[100]="You Win!";
    int len1 = strlen(buf1), len2 = strlen(buf2);
    int CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void CLCD_Display_LastInning(int inning) {
    char buf1[100], buf2[100];
    snprintf(buf1, sizeof(buf1), "%dth inning", inning);
    snprintf(buf2, sizeof(buf2), "%04d NUM, %dS %dB", lastInning[inning-1][0], lastInning[inning-1][1], lastInning[inning-1][2]);
    int len1 = strlen(buf1), len2 = strlen(buf2);
    int CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
}

