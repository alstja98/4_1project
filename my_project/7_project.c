#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "device.h"

#define MAX_INNINGS 9
#define MAX_DIGITS 4
#define NUM_LEDS 7
#define LED_ADDRESS(i) (FPGA_LED + i)

// Global variables
int answer[MAX_DIGITS]; // CPU가 생성한 4자리 숫자
int currentInning = 1; // 현재 이닝
int displayedInning = 0;
int strikes, balls; // 현재 이닝에서의 Strikes와 Balls 개수
int lastInning[MAX_INNINGS][3]; // [이닝별] [입력4숫자,Strikes,Balls] 결과 저장하는 배열
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
void *LEDOnFromBottomBasedOnLives(void *arg);
void *All_FND_Blink(void *arg);
void *Back4_FND_On(void *arg);
void *DOT_Timer_Thread(void *arg);
void *DOT_Baseball_Thread(void *arg);
void *CLCD_Display_Thread(void *arg);
void *CLCD_Display_Thread_GL(void *arg);
void *CLCD_Display_Thread_Denied(void *arg);
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


    DOT_Clear();
	closeDevices();
    return 0;
}

void initializeGame() { // 1단계
    srand(time(NULL)); // 시간을 기반으로 난수 생성기 초기화
    generateAnswer();
    printf("Answer testing: %d%d%d%d\n", answer[0], answer[1], answer[2], answer[3]); // answer 잘 만들어지는지 테스트

    run_in_parallel(DOT_Baseball_Thread, CLCD_Display_Thread, ALLLED_Blink, All_FND_Blink, NULL);
    while (1) {
        if (IsKeypadPressed(3, 2)) { // Enter (3,2)위치의 버튼을 누르면
            playGame();
            break;
        }
        usleep(1000); 
    }
    // touchpad의 enter를 누를 시에 playGame()으로 이동해야함 - 완료
    // led는 우선 모든 요소가 1초 간격으로 깜박이게 출력하게 - 완료
    // led는 1,3,5,7 번쨰와 2,4,6 번째 LED가 1초 간격으로 서로 번갈아가도록 출력 - 나중에 테스트
    // fnd는 모든 요소가 1초간격으로 깜박이게 출력 - 완료
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
        run_in_parallel(CLCD_Display_Thread_GL, DOT_Timer_Thread, Back4_FND_On, LEDOnFromBottomBasedOnLives, NULL);
        // 이미 cpu가 생성한 answer가 있는 상태
        // clcd는 good luck 출력 - 완료
        // fnd는 뒤에 4개는 8888 출력하고, 앞에 4개는 출력 안함 - 완료
        // led는 numLives 개수만큼 아래서부터 on - 완료
        // dot는 타이머 실행 - 완료

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
        // displayCLCDMessage(len1, len2, CG_or_DD, buf1, buf2);
        // displayFNDNumbers(answer); // fnd 수정해야함
        // displayDotMatrixAnimation(); // 게임 오버 애니메이션으로 수정해야함
        // updateLEDs(0); // led 수정해야함
    }
}

// 키패드로 숫자를 입력하고, 입력한 숫자를 배열에 저장하는 함수
// getInput을 하는동안은, lastInning 모드를 진행할 수 없다.
void getInput(int *input) {
    int index = 0;
    int value = 0;
    // 키패드에서 받은 값을 input 배열에 저장
    // 키패드에서 받은 값을 순서대로 FND 모듈에 출력
    while (index != 4) {
        if (IsKeypadPressed(0, 0)) {
            value = 7;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(0, 1)) {
            value = 8;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(0, 2)) {
            value = 9;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(0, 3)){
            if(index == 0){
                // 숫자를 입력한 적이 없는 경우만 가능함
                // 무조건 첫번째 이닝의 기록부터 보이게
                if(currentInning == 1){
                    CLCD_Clear();
                    char buf1[100]="Denied Last Inning mode", buf2[100]="You should enter 4 digits";
                    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
                }else{
                    displayedInning = currentInning - 1;
                    CLCD_Display_LastInning(displayedInning);
                }
            }else{
                CLCD_Clear();
                char buf1[100]="Denied Last Inning mode", buf2[100]="You should enter 4 digits";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }
        }else if (IsKeypadPressed(1, 0)) {
            value = 4;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(1, 1)) {
            value = 5;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(1, 2)) {
            value = 6;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(1, 3)){
            if(index == 0){
                // 만약 currentInning이 1이면 기능 안됨
                // 만약 currentInning이 1보다 크면 맨처음누를때는 1이닝 결과부터 보여줌 tempInning = 1;
                // 이후에 next 누르는 경우에는 tempInning++;
                // 보여주는건 tempInning 을 보여줌
                if(displayedInning == 0 || displayedInning == 1){
                    CLCD_Clear();
                    char buf1[100]="Denied Last Inning mode", buf2[100]="You should enter 4 digits";
                    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
                }else{
                    displayedInning--;
                    CLCD_Display_LastInning(displayedInning);
                }
            }else{
                CLCD_Clear();
                char buf1[100]="Denied Last Inning mode", buf2[100]="You should enter 4 digits";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }
        }else if (IsKeypadPressed(2, 0)) {
            value = 1;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(2, 1)) {
            value = 2;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(2, 2)) {
            value = 3;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(2, 3)){
            if(index==0){
                 if(displayedInning == 0 || displayedInning >= currentInning){
                     CLCD_Clear();
                    char buf1[100]="Denied Last Inning mode", buf2[100]="You should enter 4 digits";
                    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
                 }else{
                    displayedInning++;
                    CLCD_Display_LastInning(displayedInning);
                 }
            }else{
                CLCD_Clear();
                char buf1[100]="Denied Last Inning mode", buf2[100]="You should enter 4 digits";
                int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
                CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
            }
        }else if (IsKeypadPressed(3, 0)) {
            value = 0;
            $input[index] = value;
            FND_DrawNumber(index, value);
            index++;
        }else if (IsKeypadPressed(3, 1)){
            // 가장 최근에 입력된 fnd 숫자 지움
            // index도 하나 감소
            index--;
            FND_Clear(index);
        }else if (IsKeyPressed(3, 2)){
            // enter
            continue;
        }else if (IsKeyPressed(3, 3)){
            // 목숨 하나 추가
            continue;
        }

        usleep(1000); 
    }
    int number = &input[0]*1000 + &input[1]*100 + &input[2]*10 + &input[3];
    lastInning[currentInning-1][0] = number;
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
}


void updateLastInning(int inning, int strikes, int balls) {
    // Update the last inning array with the current inning's results
    lastInning[inning][0] = inning;
    lastInning[inning][1] = strikes;
    lastInning[inning][2] = balls;
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

// led threads
void *ALLLED_Blink(void *arg) {
    ALLLED_Blink();
    return NULL;
}

void *LEDOnFromBottomBasedOnLives(void *arg) {
    LEDOnFromBottomBasedOnLives(numLives);
    return NULL;
}

// fnd threads
void *All_FND_Blink(void *arg) {
    All_FND_Blink();
    return NULL;
}

void *Back4_FND_On(void *arg) {
    Back4_FND_On();
    return NULL;
}

// dot threads
void *DOT_ALL_Thread(void *arg){
    DOT_ALL();
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

// clcd threads
void *CLCD_Display_Thread(void *arg) {
    // 맨 처음 시작할 때
    int len1 = 14, len2 = 11, CG_or_DD = 1;
    char buf1[100]="Bulls and Cows", buf2[100]="Press Enter";
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_GL(void *arg) {
    // 맨 처음 시작할 때
    char buf1[100]="Good Luck!", buf2[100]="Enter your guess (4 digits):";
    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
    return NULL;
}
void *CLCD_Display_Thread_Denied(void *arg) {
    // 맨 처음 시작할 때
    char buf1[100]="Denied Last Inning mode", buf2[100]="You should enter 4 digits";
    int len1=strlen(buf1), len2=strlen(buf2), CG_or_DD = 1;
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
void CLCD_Display_LastInning(int inning) {
    char buf1[100], buf2[100];
    snprintf(buf1, sizeof(buf1), "%dth inning", inning);
    snprintf(buf2, sizeof(buf2), "digits: %d, %dS %dB", lastInning[inning][0], lastInning[inning][1], lastInning[inning][2]);
    int len1 = strlen(buf1), len2 = strlen(buf2);
    int CG_or_DD = 1;
    CLCD_Display_Custom(len1, len2, CG_or_DD, buf1, buf2);
}

