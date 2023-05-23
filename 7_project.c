#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h> // usleep 함수 사용을 위한 헤더 파일 추가
#include <termios.h>
#include <asm/fcntl.h>
#include <string.h>

#define MAX_INNINGS 9
#define MAX_DIGITS 4
#define MAX_LEDS 7

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

// Assuming FPGA_LED is the base address for controlling LEDs
#define FPGA_LED 0x12400000
#define LED_ON 0x1
#define LED_OFF 0x0
#define NUM_LEDS 7
#define LED_ADDRESS(i) (FPGA_LED + i)

// clcd 관련 변수
unsigned short *CLCD_CMD, *CLCD_DATA;
int            fd0 = 0;

// dot_matrix 관련 변수
unsigned short *dot;
unsigned int    fd;
unsigned short *DOT_COL1, *DOT_COL2, *DOT_COL3, *DOT_COL4, *DOT_COL5;
unsigned short dot_table[35][5] = {
	{0x00, 0x00, 0x00, 0x00, 0x00 }, //0 seconds left
	{0x00, 0x00, 0x00, 0x00, 0x40 }, //1 seconds left
	{0x00, 0x00, 0x00, 0x40, 0x40 }, //2 seconds left
	{0x00, 0x00, 0x40, 0x40, 0x40 },
	{0x00, 0x40, 0x40, 0x40, 0x40 },
	{0x40, 0x40, 0x40, 0x40, 0x40 },
	{0x60, 0x40, 0x40, 0x40, 0x40 },
	{0x60, 0x60, 0x40, 0x40, 0x40 },
	{0x60, 0x60, 0x60, 0x40, 0x40 },
	{0x60, 0x60, 0x60, 0x60, 0x40 },
	{0x60, 0x60, 0x60, 0x60, 0x60 },
	{0x60, 0x60, 0x60, 0x60, 0x70 },
	{0x60, 0x60, 0x60, 0x70, 0x70 },
	{0x60, 0x60, 0x70, 0x70, 0x70 },
	{0x60, 0x70, 0x70, 0x70, 0x70 },
	{0x70, 0x70, 0x70, 0x70, 0x70 },
	{0x78, 0x70, 0x70, 0x70, 0x70 },
	{0x78, 0x78, 0x70, 0x70, 0x70 },
	{0x78, 0x78, 0x78, 0x70, 0x70 },
	{0x78, 0x78, 0x78, 0x78, 0x70 },
	{0x78, 0x78, 0x78, 0x78, 0x78 },
	{0x78, 0x78, 0x78, 0x78, 0x7C },
	{0x78, 0x78, 0x78, 0x7C, 0x7C },
	{0x78, 0x78, 0x7C, 0x7C, 0x7C },
	{0x78, 0x7C, 0x7C, 0x7C, 0x7C },
	{0x7C, 0x7C, 0x7C, 0x7C, 0x7C },
	{0x7E, 0x7C, 0x7C, 0x7C, 0x7C },
	{0x7E, 0x7E, 0x7C, 0x7C, 0x7C },
	{0x7E, 0x7E, 0x7E, 0x7C, 0x7C },
	{0x7E, 0x7E, 0x7E, 0x7E, 0x7C },
	{0x7E, 0x7E, 0x7E, 0x7E, 0x7E },
	{0x7E, 0x7E, 0x7E, 0x7E, 0x7F },
	{0x7E, 0x7E, 0x7E, 0x7F, 0x7F },
	{0x7E, 0x7E, 0x7F, 0x7F, 0x7F },
	{0x7E, 0x7F, 0x7F, 0x7F, 0x7F },
	{0x7F, 0x7F, 0x7F, 0x7F, 0x7F },
};

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
void displayFNDNumbers(int *numbers);
void displayDotMatrixAnimation();
void updateLEDs(int lives);
void writeMemory(int address, int data);
int readMemory(int address);

// clcd functions declare
void displayCLCDMessage(const char *message);
static void setcommand(unsigned short command);
static void initialize_clcd(void);
static void function_set(int DL, int N, int F);
static void display_control(int D, int C, int B);
static void cursor_shift(int set_screen, int set_rightshift);
static void entry_mode_set(int ID, int S);
static void return_home(void);
static void clcd_clear(void);
static      set_RAM_address(int pos, int CG_or_DD);
static void clcd_exit(void);
void        write_byte(char ch);
int         clcd_init(void);


// dot_matrix functions declare
int dot_init(void);
void dot_write(int);
void dot_clear(void);
void dot_exit(void);

int main() {
    initializeGame();
    playGame();

    return 0;
}

void initializeGame() {
    int i, len1 = 14, len2 = 11, CG_or_DD = 1;
    char buf1[100]="Bulls and Cows", buf2[100]="Press Enter";
    srand(time(NULL)); // 시간을 기반으로 난수 생성기 초기화
    generateAnswer();
    
    // Additional initialization code for new components
    displayCLCDMessage(len1, len2, CG_or_DD, buf1, buf2);
    usleep(3000000);
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

void displayCLCDMessage(int len1, int len2, int CG_or_DD, char *buf1, char *buf2) {
    int i;
    // Write command to CLCD control register
    clcd_clear(); // Clear display

    // Write message to CLCD data register
    for (i = 0; i < len1; i++) {
        write_byte(buf1[i]);
    }
    set_RAM_address(0x40, CG_or_DD);
    for (i = 0; i < len2; i++) {
        write_byte(buf2[i]);
    }
    clcd_exit();
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
    int i;
    dot_init();
    for(i=34; i>=0; i--){
        dot_write(i);
        usleep(1000000); //1초마다 타이머
    }
    dot_exit();
}


void updateLEDs(int lives) {
    // Loop over all LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
        if (i < lives) {
            // If this LED represents a remaining life, turn it on
            writeMemory(LED_ADDRESS(i), LED_ON);
        } else {
            // Otherwise, turn it off
            writeMemory(LED_ADDRESS(i), LED_OFF);
        }
    }
}

void writeMemory(int address, int data) {
    // 주어진 주소를 포인터로 캐스팅합니다.
    int* pAddress = (int*)address;

    // 포인터를 통해 해당 주소에 데이터를 쓸 수 있습니다.
    *pAddress = data;
}


int readMemory(int address) {
    // Cast the address to a pointer
    int* pAddress = (int*)address;

    // Read the data at the given address
    return *pAddress;
}


// clcd functions
int clcd_init(void){
    int ierr = 0;
    CLCD_CMD = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_CLCD_WR);
    CLCD_DATA= mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_CLCD_RS);
    ierr = (int)((!CLCD_CMD)||(CLCD_DATA));
    return ierr;
}

void write_byte(char ch){
    unsigned short data;
    data = ch & 0x00FF; 
    *CLCD_DATA = data;
    usleep(50);
}

static void setcommand(unsigned short command){
    command &= 0x00FF; *CLCD_CMD = command; usleep(2000);
}

static void initialize_clcd(void){
    int DL = 1, N = 1, F = 0, D = 1, C = 0, B = 0, ID = 1, S = 0;
    function_set(DL, N, F);
    display_control(D, C, B);
    clcd_clear();
    entry_mode_set(ID, S);
    return_home();
}

static void function_set(int DL, int N, int F){
    unsigned short command = 0x20;
    if (DL  > 0 ) command |= 0x10;
    if (N   > 0 ) command |= 0x08;
    if (F   > 0 ) command |= 0x04;
    setcommand(command);
}

static void display_control(int D, int C, int B){
    unsigned short command = 0x08;
    if (D   > 0 ) command |= 0x04;
    if (C   > 0 ) command |= 0x02;
    if (B   > 0 ) command |= 0x01;
    setcommand(command);
}

static void cursor_shift(int set_screen, int set_rightshift){
    unsigned short command = 0x10;
    if (set_screen > 0)     command |= 0x08;
    if (set_rightshift > 0) command |= 0x04;
    setcommand(command);
}

static void entry_mode_set(int ID, int S){
    unsigned short command = 0x04;
    if (ID > 0) command |= 0x02;
    if (S  > 0) command |= 0x01;
    setcommand(command);
}

static void return_home(void){
    setcommand(0x02);
}

static void clcd_clear(void){
    setcommand(0x01);
}

static set_RAM_address( int pos, int CG_or_DD){
    unsigned short command = 0x00;
    if (CG_or_DD > 0) command = 0x80;
    command |= pos;
    setcommand(command);
}

void clcd_exit(void){
    munmap(CLCD_CMD,2);
    munmap(CLCD_DATA,2);
    close(fd);
}


// dot matrix functions
int dot_init(void){
	int ierr=0;
	DOT_COL1 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL1);
	DOT_COL2 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL2);
	DOT_COL3 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL3);
	DOT_COL4 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL4);
	DOT_COL5 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL5);
	ierr=(int)DOT_COL1+(int)DOT_COL2+(int)DOT_COL3+(int)DOT_COL4+(int)DOT_COL5;
	return ierr;
}


void dot_write(int time){
	*DOT_COL1 = dot_table[time][0];
	*DOT_COL2 = dot_table[time][1];
	*DOT_COL3 = dot_table[time][2];
	*DOT_COL4 = dot_table[time][3];
	*DOT_COL5 = dot_table[time][4];
}

void dot_clear(void){
	*DOT_COL1 = 0x00;
	*DOT_COL2 = 0x00;
	*DOT_COL3 = 0x00;
	*DOT_COL4 = 0x00;
	*DOT_COL5 = 0x00;
}

void dot_exit(void){
	dot_clear();
	munmap(DOT_COL1, 2);
	munmap(DOT_COL2, 2);
	munmap(DOT_COL3, 2);
	munmap(DOT_COL4, 2);
	munmap(DOT_COL5, 2);
	close(fd);
}
