#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/mman.h>
#include <asm/fcntl.h>
#include <string.h>

#define FPGA_CLCD_WR    0x12300000  //command
#define FPGA_CLCD_RS    0x12380000  //data

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
void        init_keyboard(void);
int         kbhit(void);
int        readch(void);
void        close_keyboard(void);

unsigned short *CLCD_CMD, *CLCD_DATA;
int            fd, fd0 = 0;
static struct termios initial_settings, new_settings;
static int peek_character=-1;

int main(int argc, char **argv){ 
    int i, CG_or_DD = 1, N = 2;
    unsigned char cmd = 'r', buf[2] = "00";
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    clcd_init();
    init_keyboard();
    initialize_clcd();
    printf("\n   CLCD Counter\n");
    printf("   r/s/c/q to restart/stop/continue/quit the counter\n");
    while( cmd != 'q'){

        if (cmd != 's'){ 
            return_home(); 

            for (i = 0; i < N; i++) write_byte(buf[i]); 
            usleep(50000); 
            buf[1]++; 
            if (buf[1] > '9'){
                buf[1] = '0'; 
                buf[0]++;
                if (buf[0] > '9') { 
                    buf[0]=buf[1]='0';
                }
            }
        }
        usleep(10); 
        if (kbhit()){
            cmd = readch(); 
            if (cmd == 'r'){ 
                strcpy(buf, "00"); 
                cmd = 'c';
            }
        }
    } 
    close_keyboard(); 
    clcd_exit();
}

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

void init_keyboard(void) {
	tcgetattr(fd0, &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(fd0, TCSANOW, &new_settings);
}

void close_keyboard(void) {
	tcsetattr(fd0, TCSANOW, &initial_settings);
}

int kbhit(void) {
	char ch;
	int nread;
	if (peek_character != -1) return -1;
	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN] = 1; tcsetattr(0, TCSANOW, &new_settings);
	if (nread == 1) {
		peek_character = ch;
		return 1;
	}
    
	return 0;
}

int readch(void) {
	char ch;
	if (peek_character != -1) {
		ch = peek_character;
		peek_character = -1;
		return ch;
	}
	read(0, &ch, 1);
	return ch;
}
