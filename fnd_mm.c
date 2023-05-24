#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define FND_CS0 0x11000000
#define FND_CS1 0x11100000
#define FND_CS2 0x11200000
#define FND_CS3 0x11300000
#define FND_CS4 0x11400000
#define FND_CS5 0x11500000
#define FND_CS6 0x11600000
#define FND_CS7 0x11700000

static struct termios initial_settings, new_settings;
static int peek_character=-1; 
unsigned short *FND0, *FND1, *FND2, *FND3, *FND4, *FND5, *FND6, *FND7; 

int fd, fd0=0;

int fnd_init(void); 
void fnd_clear(void);
void fnd_exit(void);
void init_keyboard(void);
int kbhit(void);
int readch(void);
void close_keyboard(void);
unsigned char hexn2fnd(char ch);
void fnd_display(char *hexadecimal, int N);

int main(void){
	int dev,i;   char cmd;
	unsigned char count[8]={0,0,0,0,0,0,0,0}; unsigned short tmp1, tmp2;
	time_t start_time, current_time;

	if((fd=open("/dev/mem",O_RDWR|O_SYNC))<0){ 
		perror("failed Opening FND (using mmap)! \n"); exit(1);
	}

	if(fnd_init()<0){ 
		close(fd); printf("mmap error in fnd_init()\n"); return -1;
	}

	init_keyboard();
	    cmd='r';
	fnd_clear();

	//step 1 : 게임시작 시
	while(cmd!='q'){   

		if (cmd == 'a') { 				
    	count[0] = count[1] = count[2] = count[3] = count[4] = count[5] = count[6] = count[7] = 8 ;
        fnd_display(count, 8);
        usleep(1000000);
        fnd_clear();
        usleep(1000000);
		}

		usleep(10);
	
//step 2  :  step1 에서 Enter 입력시 
		if (cmd == 'b') {
				start_time = time(NULL);
			while(1){
				current_time = time(NULL);
            		

				fnd_display(count, 4);
				usleep(5000000); 
				count[0]++; 
				if (count[0] > 1) { 
					count[1]++; 
					if (count[1] > 2) {
						count[2]++;
						if(count[2] > 3){
							count[3]++;
							if(count[3] > 4){
								count[4]++;
							}
						}
					}
				}
					if (current_time - start_time >= 3){
					count[0]=count[1]=count[2]=count[3]= 8 ;
					fnd_display(count,4) ;
				} 
			}
			 
		} 
		


		usleep(10); 
		if (kbhit()) { 
			cmd = readch(); 
			if (cmd == 'r') {
				fnd_clear();
				count[0] = count[1] = 0;
			}
		}
	}
	close_keyboard();
	fnd_exit();
}

int fnd_init(void){
	int ierr=0;
	FND0=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS0); ierr+=(int)FND0;
	FND1=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS1); ierr+=(int)FND1;
	FND2=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS2); ierr+=(int)FND2;
	FND3=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS3); ierr+=(int)FND3;
	FND4=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS4); ierr+=(int)FND4;
	FND5=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS5); ierr+=(int)FND5;
	FND6=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS6); ierr+=(int)FND6;
	FND7=mmap(NULL,2,PROT_WRITE,MAP_SHARED,fd,FND_CS7); ierr+=(int)FND7;
	return ierr;
}

void init_keyboard(void){ 
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

unsigned char hexn2fnd(char ch){ 
	unsigned char code;
	switch(ch){
		case 0x00: code=0x3f; break; 
		case 0x01: code=0x06; break; 
		case 0x02: code=0x5b; break; 
		case 0x03: code=0x4f; break;
		case 0x04: code=0x66; break;
		case 0x05: code=0x6d; break;
		case 0x06: code=0x7d; break;
		case 0x07: code=0x07; break;
		case 0x08: code=0x7f; break;
		case 0x09: code=0x67; break;
		case 0x0A: code=0x77; break; 
		case 0x0B: code=0x7c; break;
		case 0x0C: code=0x39; break;
		case 0x0D: code=0x5e; break;
		case 0x0E: code=0x79; break;
		case 0x0F: code=0x71; break;
		default  : code=0x00;
	}
	return code;
}

void fnd_display(char *hexadecimal, int N){ 
	if(N>=1) *FND0=hexn2fnd(hexadecimal[0]);
	if(N>=2) *FND1=hexn2fnd(hexadecimal[1]);
	if(N>=3) *FND2=hexn2fnd(hexadecimal[2]);
	if(N>=4) *FND3=hexn2fnd(hexadecimal[3]);
	if(N>=5) *FND4=hexn2fnd(hexadecimal[4]);
	if(N>=6) *FND5=hexn2fnd(hexadecimal[5]);
	if(N>=7) *FND6=hexn2fnd(hexadecimal[6]);
	if(N>=8) *FND7=hexn2fnd(hexadecimal[7]);
}

void fnd_clear(void){
	*FND0=0x00; *FND1=0x00; *FND2=0x00; *FND3=0x00;
	*FND4=0x00; *FND5=0x00; *FND6=0x00; *FND7=0x00;
}

void fnd_exit(void){
	fnd_clear();
	munmap(FND0,2); munmap(FND1,2); munmap(FND2,2); munmap(FND3,2);
	munmap(FND4,2); munmap(FND5,2); munmap(FND6,2); munmap(FND7,2);
	close(fd);
}
