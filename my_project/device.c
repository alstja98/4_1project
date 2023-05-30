#include "device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>

#define SAFE_UNMAP(x)	if(x != MAP_FAILED) { munmap(x, 2); x = 0; }

#define FPGA_FND_CS0	0x11000000
#define FPGA_FND_CS1	0x11100000
#define FPGA_FND_CS2	0x11200000
#define FPGA_FND_CS3	0x11300000
#define FPGA_FND_CS4	0x11400000
#define FPGA_FND_CS5	0x11500000
#define FPGA_FND_CS6	0x11600000
#define FPGA_FND_CS7	0x11700000

#define FPGA_DOT_COL1	0x11800000
#define FPGA_DOT_COL2	0x11900000
#define FPGA_DOT_COL3	0x11A00000
#define FPGA_DOT_COL4	0x11B00000
#define FPGA_DOT_COL5	0x11C00000

#define FPGA_KEY_OUT 	0x11D00000
#define FPGA_KEY_IN 	0x11E00000

#define FPGA_CLCD_WR	0x12300000
#define FPGA_CLCD_RS	0x12380000

#define FPGA_LED	0x12400000

// Declartion functions
void initKeyboard();
void closeKeyboard();

int initLED(int fd);
void closeLED();

int initFND(int fd);
void closeFND();
=======

//숫자야구용 fnd function -u
void All_FND_toggle(void); //컴파일 확인
void FND_BACK4_8();//컴파일 확인
void FND_Shuffle();//컴파일 확인
void FND_Show_Answere_win();//컴파일 확인
void FND_Show_Answere_lose();//컴파일 확인


int initDOT(int fd);
void closeDOT();
void DOT_ALL();
void DOT_Timer();
void DOT_Display_Baseball();

int initCLCD(int fd);
void closeCLCD();
void CLCD_SET_COMMAND(ushort command);
void CLCD_WRITE(char ch);
void CLCD_SET_SHIFT(bool C_or_D, bool L_or_R);
void CLCD_SET_DISPLAY();
void CLCD_SET_ENTRY_MODE();
void CLCD_SET_FUNCTION();
void CLCD_Display_Custom(int len1, int len2, int CG_or_DD, char *buf1, char *buf2);


int initKEYPAD(int fd);
void closeKEYPAD();

// dev/mem file descriptor.
int fdMem = 0;

// keyboard variables
struct termios keyaboardInitialSettings, keyboardNewSettings;
bool isKeyboardInitialized = FALSE;
char keyboardPeekChar = -1;

// LED variables
LEDPTR *LED = 0;

// FND variables
bool isFNDInitialized = FALSE;
ushort* pFND[8] = { 0,  };

ushort FND_TABLE[16] = {
	0x3F, 0x06, 0x5B, 0x4F, 
	0x66, 0x6D, 0x7D, 0x07,
	0x7F, 0x67, 0x77, 0x7C,
	0x39, 0x5E, 0x79, 0x71
};




// DOT variables
bool isDOTInitialized = FALSE;
ushort *pDOT_COL1 = 0, *pDOT_COL2 = 0, *pDOT_COL3 = 0, *pDOT_COL4 = 0, *pDOT_COL5 = 0;

ushort DOT_TABLE[43][5] = {
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
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3F, 0x44, 0x44, 0x44, 0x3F }, // A
    {0x32, 0x49, 0x49, 0x49, 0x62}, //S
    {0x7F, 0x49, 0x49, 0x49, 0x41}, //E
    {0x7F, 0x49, 0x49, 0x49, 0x36 }, // B
    {0x3F, 0x44, 0x44, 0x44, 0x3F }, // A
    {0x7F, 0x01, 0x01, 0x01, 0x01 },  // L
    {0x7F, 0x01, 0x01, 0x01, 0x01 },  // L
};

// CLCD variables
bool isCLCDInitialized = FALSE;
ushort *pCLCD_CMD = 0, *pCLCD_DATA = 0;

CLCDINFO CLCDEmpty = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
CLCDINFO CLCDInfo = {
	CLCD_ENTRY_INCREMENT,
	FALSE,
	TRUE,
	FALSE,
	FALSE,
	CLCD_SHIFT_RIGHT,
	CLCD_SHIFT_RIGHT,
	CLCD_DATA_8BIT,
	CLCD_2_LINE,
	CLCD_FONT_5x10
};

// Keypad variables
bool isKeypadInitialized = FALSE;
ushort *pKeyIn = 0, *pKeyOut = 0;

INIT_RESULT	initDevices(DEVICE devices)
{
	if ((fdMem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) return FAIL_MEMORY_OPEN;

	initKeyboard();

	int deviceResult = 0;
	if ((devices & DEVICE_LED) == DEVICE_LED) deviceResult |= initLED(fdMem);
	if ((devices & DEVICE_FND) == DEVICE_FND) deviceResult |= initFND(fdMem);
	if ((devices & DEVICE_DOT) == DEVICE_DOT) deviceResult |= initDOT(fdMem);
	if ((devices & DEVICE_CLCD) == DEVICE_CLCD) deviceResult |= initCLCD(fdMem);
	if ((devices & DEVICE_KEYPAD) == DEVICE_KEYPAD) deviceResult |= initKEYPAD(fdMem);

	return deviceResult;
}

void closeDevices()
{
	printf("All device is closing.\n");

	closeKEYPAD();
	closeCLCD();
	closeDOT();
	closeFND();
	closeLED();
	closeKeyboard();
	close(fdMem);

	printf("All device was closed.\n");
}

//
// Time functions
//
float getElapsedTime()
{
	static struct timeval prev = { 0, 0 };
	struct timeval current;

	gettimeofday(&current, NULL);
	
	float elapsed = (current.tv_sec - prev.tv_sec) * 1000.0f + (current.tv_usec - prev.tv_usec) / 1000.0f;
	prev = current;

	return elapsed;
}

//
// Keyboard functions
//
void initKeyboard()
{
	tcgetattr(0, &keyaboardInitialSettings);
	keyboardNewSettings = keyaboardInitialSettings;
	keyboardNewSettings.c_lflag &= ~ICANON;
	keyboardNewSettings.c_lflag &= ~ECHO;
	keyboardNewSettings.c_lflag &= ~ISIG;
	keyboardNewSettings.c_cc[VMIN] = 1;
	keyboardNewSettings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &keyboardNewSettings);

	isKeyboardInitialized = TRUE;
	printf("Keyboard was initialized.\n");
}

void closeKeyboard()
{
	if (!isKeyboardInitialized) return;
	tcsetattr(0, TCSANOW, &keyaboardInitialSettings);
	printf("Keyboard was closed.\n\r");
}

bool kbhit()
{
	char ch;
	int nread;
	
	if (keyboardPeekChar != (char)-1) return FALSE;

	keyboardNewSettings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &keyboardNewSettings);
	nread = read(0, &ch, 1);
	keyboardNewSettings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &keyboardNewSettings);
	if (nread == 1) {
		keyboardPeekChar = ch;
		return 1;
	}

	return TRUE;
}

char getch()
{
	char ch;
	if (keyboardPeekChar != (char)-1)
	{ 
		ch = keyboardPeekChar;
		keyboardPeekChar = -1;
		return ch;
	}

	read(0, &ch, 1);
	return ch;
}

//
// LED functions
//
int initLED(int fd)
{
	LED = (LEDPTR*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_LED);
	if (LED == MAP_FAILED) {
		LED = 0;
		return FAIL_INIT_LED;
	}

	printf("LED was initialized.\n");
	return SUCCESS;
}

void closeLED()
{
	if (LED == 0) return;
	munmap(LED, 2);

	printf("LED was closed.\n");
}

void AllLED_On()
{
	if (LED == 0) return;
	*((ushort*)LED) = 0x0000;
}

void AllLED_Off()
{
	if (LED == 0) return;
	*((ushort*)LED) = 0x00FF;
}

void AllLED_Toggle()
{
	if (LED == 0) return;
	*((ushort*)LED) = 0x00FF & ~*((ushort*)LED);
}

void LEDOnFromTop(int count)
{
	ushort led = 1U;
	while (count-- > 0)
	{
		*((ushort*)LED) = 0x00FF & ~led;
		led = (led << 1) | 1U;
	}
}
void TurnOffTopLED() //가장 위에 켜져 있는 LED 하나를 끄는 함수
{
    if (LED == 0) return;

    ushort led_status = *((ushort*)LED);
    ushort led_mask = 1U << (7); // LED_COUNT는 LED 개수를 의미함. 이 경우 8이라 가정.

    // 가장 위에 있는 켜져 있는 LED를 찾기 위해 비트마스크를 오른쪽으로 시프트함.
    while (led_mask > 0) {
        if ((led_status & led_mask) == 0) { // LED가 켜져 있다면
            led_status |= led_mask; // 해당 LED를 끔.
            break;
        }
        led_mask >>= 1; // 다음 LED로 이동.
    }

    // LED 상태 업데이트.
    *((ushort*)LED) = led_status;
}

void LEDOnFromBottom(int count) //아래에서부터 LED 켜는 함수
{
	ushort led = 0x0080U;
	while (count-- > 0)
	{
		*((ushort*)LED) = 0x00FF & ~led;
		led = (led >> 1) | 0x0080;
	}
}

void ALLLED_Blink(){
	if (LED == 0) return;
	
    while(1){
        AllLED_On();
        usleep(500000); // 0.5초 동안 대기합니다.
        AllLED_Off();
        usleep(500000); // 다시 0.5초 동안 대기합니다.
    }
}

void AlternateLEDBlink() { //1,3,5,7 번째와 2,4,6번쨰 LED 번갈아 출력
	if (LED == 0) return;
	int i;
	for (i = 0; i < 10; ++i) {
			// 1, 3, 5, 7 번째 LED 켜기
			LED->LED0 = 1;
			LED->LED2 = 1;
			LED->LED4 = 1;
			LED->LED6 = 1;
			// 2, 4, 6 번째 LED 끄기
			LED->LED1 = 0;
			LED->LED3 = 0;
			LED->LED5 = 0;
			usleep(500000); 

			// 2, 4, 6 번째 LED 켜기
			LED->LED1 = 1;
			LED->LED3 = 1;
			LED->LED5 = 1;
			// 1, 3, 5, 7 번째 LED 끄기
			LED->LED0 = 0;
			LED->LED2 = 0;
			LED->LED4 = 0;
			LED->LED6 = 0;
			sleep(500000); 
		}
}

void LEDOnFromBottomBasedOnLives(int numLives) { // numLives에 따라 아래에서부터 LED 켜는 함수
	if (LED == 0 || numLives < 0 || numLives > 8) return;
	int i;
	ushort led = 0x0080U; // 첫번째 LED 위치
	for (i = 0; i < numLives; i++)
	{
		*((ushort*)LED) = 0x00FF & ~led;
		led = (led >> 1) | 0x0080;
	}
}



//
// 7-Segment functions
//
int initFND(int fd)
{
	int idx = 0;

	pFND[0] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS0);
	pFND[1] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS1);
	pFND[2] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS2);
	pFND[3] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS3);
	pFND[4] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS4);
	pFND[5] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS5);
	pFND[6] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS6);
	pFND[7] = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_FND_CS7);

	if ((pFND[0] == MAP_FAILED) ||
		(pFND[1] == MAP_FAILED) ||
		(pFND[2] == MAP_FAILED) ||
		(pFND[3] == MAP_FAILED) ||
		(pFND[4] == MAP_FAILED) ||
		(pFND[5] == MAP_FAILED) ||
		(pFND[6] == MAP_FAILED) ||
		(pFND[7] == MAP_FAILED)) {
		for (idx = 0; idx < 8; idx++)
			SAFE_UNMAP(pFND[idx]);
		
		return FAIL_INIT_FND;
	}

	isFNDInitialized = TRUE;
	
	printf("FND was initialized.\n");
	return SUCCESS;
}

void closeFND()
{
	if (!isFNDInitialized) return;

	int idx = 0;
	for (idx = 0; idx < 8; idx++) {
		SAFE_UNMAP(pFND[idx]);
	}

	printf("FND was closed.\n");
}


void AllFND_Clear()
{
	if (!isFNDInitialized) return;

	*pFND[0] = 0x00;
	*pFND[1] = 0x00;
	*pFND[2] = 0x00;
	*pFND[3] = 0x00;
	*pFND[4] = 0x00;
	*pFND[5] = 0x00;
	*pFND[6] = 0x00;
	*pFND[7] = 0x00;
}

void FND_Clear(int index)
{
	if (!isFNDInitialized || (index < 0) || (index > 7)) return;
	*pFND[index] = 0x00;
}

void FND_Set(int index, int no)
{
	if (!isFNDInitialized || (no < 0) || (no > 15)) return;	
	*pFND[index] = FND_TABLE[no];
}

void FND_DrawNumber(int index, int val) // 사용자가 입력한 숫자를 fnd에 표시
{
	if (!isFNDInitialized) return;
	FND_Clear(index);
		FND_Set(index, val);
}

// devu
void All_FND_Blink(){
	while(1){
        	AllFND_on();
        	usleep(500000);
        	AllFND_Clear();
        	usleep(500000);
	}
}
<<<<<<< HEAD

void Back4_FND_On()
{
	// if(!isFNDInitialized) return;
	AllFND_Clear();
	*pFND[0] = 0x7F;
	*pFND[1] = 0x7F;
	*pFND[2] = 0x7F;
	*pFND[3] = 0x7F;
}

=======
// 숫자야구용 fnd function -u 

void AllFND_on()
{
	if(!isFNDInitialized) return;

	*pFND[0] = 0x7F;
	*pFND[1] = 0x7F;
	*pFND[2] = 0x7F;
	*pFND[3] = 0x7F;
	*pFND[4] = 0x7F;
	*pFND[5] = 0x7F;
	*pFND[6] = 0x7F;
	*pFND[7] = 0x7F;
}

void All_FND_toggle(){

while (1){
        	AllFND_on();
        	usleep(1000000);
        	AllFND_Clear();        	
        	usleep(1000000);
	}		
}



void FND_BACK4_8()
{
    if (!isFNDInitialized) return;

	int i ;
	
    for (i = 0; i < 4; i++) {

        *pFND[i] = FND_TABLE[8];
	}
}


void FND_shuffle()
{
		if (!isFNDInitialized) return;

    int count ;
    
    for(count = 0;count<20;count++)
    {
        *pFND[0] = FND_TABLE[count%10];
        *pFND[1] = FND_TABLE[count%10];
        *pFND[2] = FND_TABLE[count%10];
        *pFND[3] = FND_TABLE[count%10];
        
        usleep(500000); // 0.5초 딜레이
		
    }
}

void FND_Show_Answere_win()
{
	if (!isFNDInitialized) return;
	
	*pFND[0] = FND_TABLE[1/*answer[3]*/];
	*pFND[1] = FND_TABLE[2/*answer[2]*/];
	*pFND[2] = FND_TABLE[3/*answer[1]*/];
	*pFND[3] = FND_TABLE[4/*answer[0]*/];

	while(1){
	*pFND[4] = FND_TABLE[1/*itput[3]*/];
	*pFND[5] = FND_TABLE[2/*input[2]*/];
	*pFND[6] = FND_TABLE[3/*input[1]*/];
	*pFND[7] = FND_TABLE[4/*input[0]*/];
	usleep(1000000)
	}
}

void FND_Show_Answere_lose()
{
	if (!isFNDInitialized) return;
	
	*pFND[0] = FND_TABLE[1/*answer[3]*/];
	*pFND[1] = FND_TABLE[2/*answer[2]*/];
	*pFND[2] = FND_TABLE[3/*answer[1]*/];
	*pFND[3] = FND_TABLE[4/*answer[0]*/];
	
	*pFND[4] = FND_TABLE[0];
	*pFND[5] = FND_TABLE[0];
	*pFND[6] = FND_TABLE[0];
	*pFND[7] = FND_TABLE[0];
	
}


//
// DOT functions
//
int initDOT(int fd)
{
	pDOT_COL1 = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL1);
	pDOT_COL2 = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL2);
	pDOT_COL3 = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL3);
	pDOT_COL4 = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL4);
	pDOT_COL5 = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL5);

	if ((pDOT_COL1 == MAP_FAILED) ||
		(pDOT_COL2 == MAP_FAILED) ||
		(pDOT_COL3 == MAP_FAILED) ||
		(pDOT_COL4 == MAP_FAILED) ||
		(pDOT_COL5 == MAP_FAILED)) {
		SAFE_UNMAP(pDOT_COL1);
		SAFE_UNMAP(pDOT_COL2);
		SAFE_UNMAP(pDOT_COL3);
		SAFE_UNMAP(pDOT_COL4);
		SAFE_UNMAP(pDOT_COL5);
		return FAIL_INIT_DOT;
	}

	isDOTInitialized = TRUE;
	printf("Dot-matrix was initialized.\n");
	return SUCCESS;
}

void closeDOT()
{
	if (!isDOTInitialized) return;
	SAFE_UNMAP(pDOT_COL1);
	SAFE_UNMAP(pDOT_COL2);
	SAFE_UNMAP(pDOT_COL3);
	SAFE_UNMAP(pDOT_COL4);
	SAFE_UNMAP(pDOT_COL5);
	printf("Dot-matrix was closed.\n");
}

void DOT_Clear()
{
	if (!isDOTInitialized) return;

	*pDOT_COL1 = (ushort)0x00;
	*pDOT_COL2 = (ushort)0x00;
	*pDOT_COL3 = (ushort)0x00;
	*pDOT_COL4 = (ushort)0x00;
	*pDOT_COL5 = (ushort)0x00;
}

void DOT_Write(ushort table[5])
{
	if (!isDOTInitialized || table == 0) return;

	*pDOT_COL1 = table[0];
	*pDOT_COL2 = table[1];
	*pDOT_COL3 = table[2];
	*pDOT_COL4 = table[3];
	*pDOT_COL5 = table[4];
}

void DOT_Write_Decimal(int no)
{
	if (no < 0 || no > 10) return;
	DOT_Write(DOT_TABLE[no]);
}


// 숫자야구 dot matrix 짠거
void DOT_ALL(){
	//DOT 모두 켜짐
	DOT_Clear();
	DOT_Write(DOT_TABLE[36]);
}

void DOT_Timer(){
    int i;
	DOT_Clear();
    for(i=35; i>=0; i--){
        DOT_Write(DOT_TABLE[i]);
        usleep(1000000); //1초마다 타이머
    }
}


void DOT_Display_Baseball() {
	DOT_Clear();
	int i;
    for(i = 36; i <= 43; i++){
        DOT_Write(DOT_TABLE[i]);
        usleep(500000); // 0.5초마다 표시
    }
}



//
// CLCD functions
//
int initCLCD(int fd)
{
	pCLCD_CMD = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_CLCD_WR);
	pCLCD_DATA = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_CLCD_RS);

	if ((pCLCD_CMD == MAP_FAILED) ||
	    (pCLCD_DATA == MAP_FAILED)) {
  	    	SAFE_UNMAP(pCLCD_CMD);
	   	SAFE_UNMAP(pCLCD_DATA);
		return FAIL_INIT_CLCD;
	}

	isCLCDInitialized = TRUE;

	// Initialize default settings.
	
	CLCD_SET_FUNCTION();
	CLCD_SET_DISPLAY();
	CLCD_Clear();
	CLCD_SET_ENTRY_MODE();
	CLCD_ReturnHome();

	printf("CLCD was initialized.\n");
	return SUCCESS;
}

void closeCLCD()
{
	if (!isCLCDInitialized) return;
	SAFE_UNMAP(pCLCD_CMD);
	SAFE_UNMAP(pCLCD_DATA);

	printf("CLCD was closed.\n");
}

void CLCD_SET_COMMAND(ushort command)
{
	command &= 0x00FF;
	*pCLCD_CMD = command;
	usleep(2000);
}

void CLCD_WRITE(char ch)
{
	*pCLCD_DATA = (ushort)(ch & 0x00FF);
	usleep(50);
}

void CLCD_SET_ENTRY_MODE()
{
	ushort command = 0x04;
	if (CLCDInfo.entryMode == CLCD_ENTRY_INCREMENT) command |= 0x02;
	if (CLCDInfo.shiftOn) command |= 0x01;
	CLCD_SET_COMMAND(command);
}

void CLCD_SET_DISPLAY()
{
	ushort command = 0x08;
	if (CLCDInfo.displayOn) command |= 0x04;
	if (CLCDInfo.cursorOn) command |= 0x02;
	if (CLCDInfo.blinking) command |= 0x01;
	CLCD_SET_COMMAND(command);
}

void CLCD_SET_FUNCTION()
{
	ushort command = 0x20;
	if (CLCDInfo.dataLength == CLCD_DATA_8BIT) command |= 0x10;
	if (CLCDInfo.displayLine == CLCD_2_LINE) command |= 0x08;
	if (CLCDInfo.fontSize == CLCD_FONT_5x10) command |= 0x04;
	CLCD_SET_COMMAND(command);
}

void CLCD_SET_SHIFT(bool C_or_D, bool L_or_R)
{
	ushort command = 0x10;
	if (C_or_D) command |= 0x08;
	if (L_or_R) command |= 0x04;
	CLCD_SET_COMMAND(command);
}

void CLCD_Clear()
{
	if (!isCLCDInitialized) return;

	CLCD_SET_COMMAND(0x01);
}

void CLCD_ReturnHome()
{
	if (!isCLCDInitialized) return;

	CLCD_SET_COMMAND(0x02);
}

CLCDINFO CLCD_GetInformation()
{
	if (!isCLCDInitialized) return CLCDEmpty;
	return CLCDInfo;
}

void CLCD_SetInformation(CLCDINFO* info)
{
	if (!isCLCDInitialized) return;
	CLCDInfo = *info;
	CLCD_SET_FUNCTION();
	CLCD_SET_DISPLAY();
	CLCD_SET_SHIFT(1, CLCDInfo.displayShiftMode);
	CLCD_SET_SHIFT(0, CLCDInfo.cursorShiftMode);
	CLCD_SET_ENTRY_MODE();
}

void CLCD_SetEntryMode(CLCD_ENTRY entry)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.entryMode = entry;
	CLCD_SET_ENTRY_MODE();
}

void CLCD_SetDisplayShift(bool shiftOn)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.shiftOn = shiftOn;
	CLCD_SET_ENTRY_MODE();
}

void CLCD_SetDisplayOn(bool value)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.displayOn = value;
	CLCD_SET_DISPLAY();
}

void CLCD_SetCursorOn(bool value)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.cursorOn = value;
	CLCD_SET_DISPLAY();
}

void CLCD_SetBlinking(bool value)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.blinking = value;
	CLCD_SET_DISPLAY(CLCDInfo.displayOn, CLCDInfo.cursorOn, CLCDInfo.blinking);
}

void CLCD_SET_DISPLAYShiftMode(CLCD_SHIFT shiftMode)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.displayShiftMode = shiftMode;
	CLCD_SET_SHIFT(1, shiftMode);
}

void CLCD_SetCursorShiftMode(CLCD_SHIFT shiftMode)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.cursorShiftMode = shiftMode;
	CLCD_SET_SHIFT(0, shiftMode);
}

void CLCD_SetDataLength(CLCD_DATA dataLength)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.dataLength = dataLength;
	CLCD_SET_FUNCTION();
}

void CLCD_SetDisplayLine(CLCD_LINE displayLine)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.displayLine = displayLine;
	CLCD_SET_FUNCTION();
}

void CLCD_SetFontSize(CLCD_FONT fontSize)
{
	if (!isCLCDInitialized) return;
	CLCDInfo.fontSize = fontSize;
	CLCD_SET_FUNCTION();
}

void CLCD_SetUserFont(int no, ushort font[10])
{
	if (!isCLCDInitialized) return;

	ushort command = 0x40;
	command |= (no * 8);
	CLCD_SET_COMMAND(command);

	int idx = 0;
	for (idx = 0; idx < 8; idx++)
	{
		CLCD_WRITE(font[idx]);	
	}
}

void CLCD_SetCursorPos(char pos)
{
	if (!isCLCDInitialized) return;

	ushort command = 0x80;
	command |= pos;
	CLCD_SET_COMMAND(command);
}

void CLCD_Put(char ch)
{
	if (!isCLCDInitialized) return;
	CLCD_WRITE(ch);
}

void CLCD_SetLine(int line)
{
	if (!isCLCDInitialized || (line < 0) || (line > 1)) return;
	CLCD_SetCursorPos(0x40 * line);
}

void CLCD_Print(const char* pszText)
{
	if (!isCLCDInitialized || (pszText == 0)) return;

	char* pPtr = (char *)pszText;
	while (*pPtr != 0)
	{
		CLCD_WRITE(*pPtr);
		pPtr++;
	}
}

void CLCD_PrintFit(const char* pszText)
{
	if (!isCLCDInitialized || (pszText == 0)) return;

	CLCD_SET_COMMAND(0x01);	// CLEAR
	CLCD_SET_COMMAND(0x02);	// RETURN HOME

	int nCnt = 0;
	bool didNewLine = FALSE;

	for (nCnt = 0; nCnt < 16; nCnt++) {
		char ch = *(pszText + nCnt);
		if (ch == 0) return;
		if (ch == '\n') { nCnt++; break; }
		CLCD_WRITE(ch);				
	}

	CLCD_SetCursorPos(0x40);
	for (; nCnt < 32; nCnt++) {
		char ch = *(pszText + nCnt);
		if (ch == 0) return;
		CLCD_WRITE(ch);
	}
}


// 숫자야구 clcd 전용 코드 
void CLCD_Display_Custom(int len1, int len2, int CG_or_DD, char *buf1, char *buf2) {
    int i;
    // Write command to CLCD control register
    CLCD_Clear(); // Clear display

    // Write message to CLCD data register
    for (i = 0; i < len1; i++) {
        CLCD_WRITE(buf1[i]);
    }
    CLCD_SetCursorPos(0x40);
    for (i = 0; i < len2; i++) {
        CLCD_WRITE(buf2[i]);
    }

    // closeCLCD();
}


// Keypad functions
int initKEYPAD(int fd)
{
	pKeyIn = (ushort*)mmap(NULL, 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPGA_KEY_IN);
	pKeyOut = (ushort*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_KEY_OUT);

	if ((pKeyIn == MAP_FAILED) ||
		(pKeyOut == MAP_FAILED)) {
		SAFE_UNMAP(pKeyIn);
		SAFE_UNMAP(pKeyOut);
		return FAIL_INIT_KEYPAD;
	}

	isKeypadInitialized = TRUE;

	printf("Keypad was initialized.\n");
	return SUCCESS;
}

void closeKEYPAD()
{
	if (!isKeypadInitialized) return;
	SAFE_UNMAP(pKeyIn);
	SAFE_UNMAP(pKeyOut);

	printf("Keypad was closed.\n");
}

ushort GetKeypad()
{
	if (!isKeypadInitialized) return 0;

	int col = 0;
	ushort keyvalue = 0;

	for (col = 0; col < 4; col++)
	{
		*pKeyOut = 1 << (3 - col);
		ushort keyin = *pKeyIn;
		keyvalue |= ((keyin & 0x000F) << (col * 4));
	}
	
	return keyvalue;
}

ushort GetKeypadWait()
{
	if (!isKeypadInitialized) return 0;

	int col = 0;
	ushort keyvalue = 0;

	while (keyvalue == 0) {
		for (col = 0; col < 4; col++)
		{
			*pKeyOut = 1 << (3 - col);
			ushort keyin = *pKeyIn;
			keyvalue |= ((keyin & 0x000F) << (col * 4));
		}
		usleep(1000);
	}
	return keyvalue;
}

bool IsKeypadPressed(int col, int row)
{
	if (!isKeypadInitialized ||
		(col < 0 || col > 4) ||
		(row < 0 || row > 4)) return FALSE;
	
	*pKeyOut = (1 << (3 - col));

	ushort keyin = *pKeyIn;
	ushort target = (1 << row);
	return (keyin & target) == target;
}

