// led_mm.c : LED Control Program using mmap()
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

#define FPGA_LED	0x12400000 
static struct termios initial_settings, new_settings;
static int peek_character=-1;
unsigned short *LED;
int fd, fd0=0;

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

int main(void)
{int dev;		char ch='y', command;
 unsigned int fd; 	unsigned char val;

 if((fd=open("/dev/mem", O_RDWR|O_SYNC)) < 0)
 {perror("failed opening /dev/mem in led_mm \n"); exit(1);}
LED=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_LED);
if(!LED)
 {printf("failed memory mapping in led_mm \n"); return -(!LED);}
printf("\n Press u/d/q for up_shift/down_shift/quit \n");

init_keyboard();
ch='u'; val=0x01;
while(ch=='u' || ch=='d')
 {if(ch=='u') 
   val=((val<<7) & 0x80) | (val>>1);
  else if(ch=='d')
   val=((val>>7) & 0x01) | (val<<1);
  *LED=~val; usleep(200000);
  if(kbhit()) ch=readch();
 }
close_keyboard();
munmap(LED,2); close(fd); return 0;
}

