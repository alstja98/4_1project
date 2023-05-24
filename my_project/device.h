#ifndef __LIB_DEVICE__
#define __LIB_DEVICE__

#define	TRUE			1
#define FALSE			0

#define LED_ON			0
#define LED_OFF			1

typedef unsigned char	bool;
typedef unsigned short  ushort;
typedef unsigned int	uint;

typedef enum {
	SUCCESS				= 0,
	FAIL				= 0x80000000,
	FAIL_MEMORY_OPEN	= FAIL,
	FAIL_INIT_DEVICES	= 0x00010000,
	FAIL_INIT_LED		= 0x80010001,
	FAIL_INIT_FND		= 0x80010002,
	FAIL_INIT_DOT		= 0x80010004,
	FAIL_INIT_CLCD		= 0x80010008,
	FAIL_INIT_KEYPAD	= 0x80010010,
} INIT_RESULT;

typedef enum {
	DEVICE_LED		= 0x01,
	DEVICE_FND		= 0x02,
	DEVICE_DOT		= 0x04,
	DEVICE_CLCD		= 0x08,
	DEVICE_KEYPAD	= 0x10,
	DEVICE_ALL		= 0x1F
} DEVICE;

typedef struct {
	bool	LED0 : 1;
	bool	LED1 : 1;
	bool	LED2 : 1;
	bool	LED3 : 1;
	bool	LED4 : 1;
	bool	LED5 : 1;
	bool	LED6 : 1;
	bool	LED7 : 1;
	bool	reserved : 8;
} LEDPTR;


extern INIT_RESULT	initDevices(DEVICE systems);
extern void			closeDevices();