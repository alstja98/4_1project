#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

void init_keyboard(void);
void close_keyboard(void);
void kbhit(void);
int readch(void);

static struct termios initial_settings, new_settings;
static int peek_character=-1;
int fd0=0;

int main(void){

}

void init_keyboard(void) {
	tcgetattr(fd0,&initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(fd0,TCSANOW,&new_settings);
}

void close_keyboard(void){ 
	tcsetattr(fd0,TCSANOW,&initial_settings);
}

int kbhit(void){ 
	char ch;  int nread;
	if(peek_character!=-1) return -1;
	new_settings.c_cc[VMIN]=0;
	tcsetattr(0,TCSANOW,&new_settings);
	nread=read(0,&ch,1);
	new_settings.c_cc[VMIN]=1; tcsetattr(0,TCSANOW,&new_settings);
	if(nread==1){peek_character=ch; return 1;}
	return 0;
}

int readch(void){
	char ch;
	if(peek_character!=-1) {ch=peek_character; peek_character=-1; return ch;}
	read(0,&ch,1);
	return ch;
}
