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
// void initKeyboard();
// void closeKeyboard();

// int initFND(int fd);
// void closeFND();

// int initKEYPAD(int fd);
// void closeKEYPAD();

// led functions declare
int initLED(int fd);
void closeLED();
void updateLEDs(int lives);

// clcd functions declare
void displayCLCDMessage(int len1, int len2, int CG_or_DD, char *buf1, char *buf2);
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
int         clcd_init(int fd);

// dot_matrix functions declare
int dot_init(int fd);
void dot_write(int);
void dot_clear(void);
void dot_exit(void);
void displayDotMatrixAnimation();


// dev/mem file descriptor.
int fdMem = 0;

// led variables
LEDPTR *LED = 0;

// clcd variables
unsigned short *CLCD_CMD, *CLCD_DATA;

// dot_matrix variables
unsigned short *dot;
unsigned short *DOT_COL1, *DOT_COL2, *DOT_COL3, *DOT_COL4, *DOT_COL5;
unsigned short dot_table[36][5] = {
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


// devices functions
INIT_RESULT	initDevices(DEVICE devices)
{
	if ((fdMem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) return FAIL_MEMORY_OPEN;

	initKeyboard();

	int deviceResult = 0;
	if ((devices & DEVICE_LED) == DEVICE_LED) deviceResult |= initLED(fdMem);
	// if ((devices & DEVICE_FND) == DEVICE_FND) deviceResult |= initFND(fdMem);
	if ((devices & DEVICE_DOT) == DEVICE_DOT) deviceResult |= dot_init(fdMem);
	if ((devices & DEVICE_CLCD) == DEVICE_CLCD) deviceResult |= clcd_init(fdMem);
	// if ((devices & DEVICE_KEYPAD) == DEVICE_KEYPAD) deviceResult |= initKEYPAD(fdMem);

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


// clcd functions -> 수정 완료
int clcd_init(int fd){
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


// dot matrix functions -> 수정 완료
int dot_init(int fd){
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

void displayDotMatrixAnimation() {
    int i;
    dot_init();
    for(i=34; i>=0; i--){
        dot_write(i);
        usleep(1000000); //1초마다 타이머
    }
    dot_exit();
}

// led functions
int initLED(int fd)
{
	LED = (LEDPTR*)mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_LED);
	printf("LED was initialized.\n");
	return SUCCESS;
}

void closeLED()
{
	if (LED == 0) return;
	munmap(LED, 2);

	printf("LED was closed.\n");
}

void updateLEDs(int lives) {

}

