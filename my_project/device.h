#ifndef __LIB_DEVICE__
#define __LIB_DEVICE__

#define	TRUE			1
#define FALSE			0

#define LED_ON			0
#define LED_OFF			1

#define KEYPAD_0		0x000F
#define KEYPAD_0_0		0x0001
#define KEYPAD_0_1		0x0002
#define KEYPAD_0_2		0x0004
#define KEYPAD_0_3		0x0008

#define KEYPAD_1		0x00F0
#define KEYPAD_1_0		0x0010
#define KEYPAD_1_1		0x0020
#define KEYPAD_1_2		0x0040
#define KEYPAD_1_3		0x0080

#define KEYPAD_2		0x0F00
#define KEYPAD_2_0		0x0100
#define KEYPAD_2_1		0x0200
#define KEYPAD_2_2		0x0400
#define KEYPAD_2_3		0x0800

#define KEYAPD_3		0xF000
#define KEYPAD_3_0		0x1000
#define KEYPAD_3_1		0x2000
#define KEYPAD_3_2		0x4000
#define KEYPAD_3_3		0x8000

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

typedef enum {
	CLCD_ENTRY_DECREMENT = 0,
	CLCD_ENTRY_INCREMENT = 1
} CLCD_ENTRY;

typedef enum {
	CLCD_SHIFT_LEFT 	= 0,
	CLCD_SHIFT_RIGHT 	= 1
} CLCD_SHIFT;

typedef enum {
	CLCD_DATA_4BIT		= 0,
	CLCD_DATA_8BIT		= 1
} CLCD_DATA;

typedef enum {
	CLCD_1_LINE			= 0,
	CLCD_2_LINE			= 1
} CLCD_LINE;

typedef enum {
	CLCD_FONT_5x7		= 0,
	CLCD_FONT_5x10		= 1
} CLCD_FONT;

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

typedef struct {
	CLCD_ENTRY	entryMode : 1;
	bool		shiftOn : 1;
	bool		displayOn : 1;
	bool		cursorOn : 1;
	bool		blinking : 1;
	CLCD_SHIFT	displayShiftMode : 1;
	CLCD_SHIFT	cursorShiftMode : 1;
	CLCD_DATA	dataLength : 1;
	CLCD_LINE	displayLine : 1;
	CLCD_FONT	fontSize : 1;
	bool		reserved : 6;
} CLCDINFO;

extern INIT_RESULT	initDevices(DEVICE systems);
extern void			closeDevices();

// Keyboard functions
extern bool			kbhit();
extern char			getch();

// LED functions
extern LEDPTR*		LED;
extern void			AllLED_On();
extern void			AllLED_Off();
extern void			ALLLED_Blink();
extern void 		TurnOffTopLED();
extern void 		LEDOnFromBottomBasedOnLives(int numLives);

// 7-Segments functions
extern void			AllFND_Clear();
extern void			Back4_FND_On();
extern void 		All_FND_Blink();
extern void			FND_Clear(int index);
extern void			FND_Set(int index, int no);
extern void			FND_DrawNumber(int index, int val);
extern void 		FND_Show_Answer_win(int *answer, int *input);
extern void 		FND_Show_Answer_lose(int *answer);

// DOT-Matrix functions
extern void			DOT_Clear();
extern void			DOT_Write(ushort table[5]);
extern void 		DOT_Inning(int inning);
extern void 		DOT_Display_Baseball();

// CLCD functions
extern void			CLCD_Clear();
extern void			CLCD_ReturnHome();
extern void			CLCD_SetCursorPos(char pos);
extern void 		CLCD_Display_Custom(int len1, int len2, int CG_or_DD, char *buf1, char *buf2);

// Keypad functions
extern ushort		GetKeypad();
extern ushort		GetKeypadWait();
extern bool			IsKeypadPressed(int col, int row);
#endif
