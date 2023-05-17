#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>

#define FPGA_DOT_COL1   0x11800000
#define FPGA_DOT_COL2	0x11900000
#define FPGA_DOT_COL3	0x11A00000
#define FPGA_DOT_COL4	0x11B00000
#define FPGA_DOT_COL5	0x11C00000

unsigned short *dot;
unsigned int    fd;
unsigned short *DOT_COL1, *DOT_COL2, *DOT_COL3, *DOT_COL4, *DOT_COL5;

unsigned short dot_table[10][5] = {
	{0x7F, 0x41, 0x41, 0x41, 0x7F },
	{0x00, 0x00, 0x7F, 0x00, 0x00 },
	{0x4F, 0x49, 0x49, 0x49, 0x79 },
	{0x49, 0x49, 0x49, 0x49, 0x7F },
	{0x78, 0x08, 0x08, 0x7F, 0x08 },
	{0x79, 0x49, 0x49, 0x49, 0x4F },
	{0x7F, 0x49, 0x49, 0x49, 0x4F },
	{0x40, 0x40, 0x40, 0x40, 0x7F },
	{0x7F, 0x49, 0x49, 0x49, 0x7F },
	{0x78, 0x48, 0x48, 0x48, 0x7F },

};

int dot_init(void);
void dot_write(int);
void dot_clear(void);
void dot_exit(void);

int main(void){
	int i;
	if ((fd=open("/dev/mem", O_RDWR|O_SYNC))<0){
		perror("failed memory opening in dot_mm.c \n");
		exit(1);
	}

	dot_init();
	for (i=0; i<=9; i++){
		dot_write(i);
		usleep(1000000);
	}
	dot_exit();
	return 0;
}


int dot_init(void){ // mmap(
	int ierr=0;
	DOT_COL1 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL1);
	DOT_COL2 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL2);
	DOT_COL3 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL3);
	DOT_COL4 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL4);
	DOT_COL5 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL5);
	ierr=(int)DOT_COL1+(int)DOT_COL2+(int)DOT_COL3+(int)DOT_COL4+(int)DOT_COL5;
	return ierr;
}


void dot_write(int decimal){
	*DOT_COL1 = dot_table[decimal][0];
	*DOT_COL2 = dot_table[decimal][1];
	*DOT_COL3 = dot_table[decimal][2];
	*DOT_COL4 = dot_table[decimal][3];
	*DOT_COL5 = dot_table[decimal][4];
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
