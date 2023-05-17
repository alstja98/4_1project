#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <asm/fcntl.h>				

#define KEY_OUT_ADDR   0x11D00000			
#define KEY_IN_ADDR    0x11E00000
#define FND_CS0        0x11000000
#define SCAN_NUM       4

 
unsigned char    *keyin, *keyout, *FND0;		
unsigned char keypad_matrix[4][4]= {{'0','1','2','3'},{'4','5','6','7'},
                                    {'8','9','A','B'},{'C','D','E','F'}};	

unsigned char keypad_code_matrix[4][4]=
        {{0x3f,0x06,0x5b,0x4f},{0x66,0x6d,0x7d,0x07},
         {0x7f,0x67,0x77,0x7c},{0x39,0x5e,0x79,0x71}};		


int main(void){
    unsigned char  i, *keyin0, Key_pressed; 
    int  fd, col_no;                        
    
    if ((fd=open("/dev/mem", O_RDWR|O_SYNC)) < 0)	{ 
       perror("mem open failed\n");	exit(1);        
    }
      
    keyin  = mmap(NULL, 2, PROT_READ|PROT_WRITE, MAP_SHARED, fd, KEY_IN_ADDR); 
    keyout = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, KEY_OUT_ADDR);          

    if ((int)keyin < 0 || (int)keyout < 0){      
       keyin = NULL;  keyout = NULL;   close(fd); 
       printf("mmap error!\n");   return -1;      
    }
    
    while (1){ 
          for (col_no=0; col_no<SCAN_NUM; col_no++){ 
              *keyout = (1<<(SCAN_NUM-1-col_no));    
              *keyin0 = *keyin;                      
              switch (*keyin0 & 0x0f){ 
                     case 0x01: Key_pressed = keypad_matrix[0][col_no]; break; 
                     case 0x02: Key_pressed = keypad_matrix[1][col_no]; break; 
                     case 0x04: Key_pressed = keypad_matrix[2][col_no]; break; 
                     case 0x08: Key_pressed = keypad_matrix[3][col_no]; break; 
                     default  : Key_pressed = '?'; break;	                   
              }
              if (Key_pressed!='?')break; 
          }
          if (Key_pressed!='?'){  
             printf("Key %c is pressed\n", Key_pressed); usleep(100000);  
          }
      
          usleep(10000); 
  
    }
    
    munmap(keyin, 2);  munmap(keyout, 2); 
    close(fd); 
  
    return 0; 
} 
