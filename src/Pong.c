/*
 * Pong.c
 *
 * Created: 5/29/2015 10:49:45 PM
 *  Author: Mark
 */ 


#include <avr/io.h>
#include "C:\Users\Amin\Documents\Atmel Studio\6.2\Pong\includes\io.c" 
#include <avr/interrupt.h>
#include <avr/eeprom.h>

// macro for easier usage
#define read_eeprom_word(address) eeprom_read_word ((const uint16_t*)address)
#define write_eeprom_word(address,value) eeprom_write_word ((uint16_t*)address,(uint16_t)value)
#define update_eeprom_word(address,value) eeprom_update_word ((uint16_t*)address,(uint16_t)value)



//TIMER
// TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	TCCR1B = 0x0B;
	// AVR output compare register OCR1A.
	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	OCR1A = 125;
	// AVR timer interrupt mask register
	// bit1: OCIE1A -- enables compare match interrupt
	TIMSK1 = 0x02;
	//Initialize avr counter
	TCNT1=0;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;
	//Enable global interrupts: 0x80: 1000000
	SREG |= 0x80;
}
void TimerOff() {
	// bit3bit1bit0=000: timer off
	TCCR1B = 0x00;
}
void TimerISR() {
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1
	// (every 1 ms per TimerOn settings)
	// Count down to 0 rather than up to TOP (results in a more efficient comparison)
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		// Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
//END TIMER


//INPUT BUTTONs
unsigned char tmpD;

//SAVED MEMORY
unsigned char EEMEM my_eeprom_array[6];

//GLOBAL VARIABLES
unsigned char FINISH = 0;
unsigned char WRITE = 0;
unsigned char READY = 0;
unsigned char SINGLE = 0;
unsigned char MULT = 0;
unsigned char DIFF = 0;

unsigned char X = 0; //P1 score. //start at -1 because of +2 at first p2 score bug
unsigned char Y = 0; //P2 score
unsigned char XSCORE = 0; //boolean
unsigned char YSCORE = 0; //boolean

unsigned char P1ROW_MOVEMENT[6] = {0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0};
unsigned char P1index = 4;
unsigned char P1COL = 0xFE;

unsigned char P2ROW_MOVEMENT[6] = {0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0};
unsigned char P2index = 4;
unsigned char P2COL = 0x7F;

unsigned char BALLROW_MOVEMENT[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
unsigned char BALLCOL_MOVEMENT[8] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};
unsigned char BRindex = 3;
unsigned char BCindex = 3;

//MENU
enum Menus {home, wait, difficulty, play, wait_score, writeX, writeY, winX, winY, 
	holdwin, rwait, rwait1, rpage1, rwait3, rpage3, rwait4} Menu;
unsigned short counterwin = 0;
void MenuScreen()
{
	switch(Menu)
	{
		case home:
			if((tmpD & 0x1F) == 0x00)
			{
				MULT = 0;
				SINGLE = 0;
				DIFF = 0;
				X = 0;
				Y = 0;
				Menu = home;
			}
			else if((tmpD & 0x1F) == 0x01)
			{
				SINGLE = 1;
				MULT = 0;
				Menu = wait;
			}
			else if((tmpD & 0x1F) == 0x02)
			{
				MULT = 1;
				SINGLE = 0;
				Menu = wait;
			}
			else if((tmpD & 0x1F) == 0x04)
			{
				Menu = rwait;
			}
			break;
		case wait:
			if((tmpD & 0x1F) != 0)
			{
				if((tmpD & 0x1F) == 0x01)
				{
					SINGLE = 1;
					MULT = 0;
				}
				else if((tmpD & 0x1F) == 0x02)
				{
					MULT = 1;
					SINGLE = 0;
				}
					
				Menu= wait;
			}
			else
				Menu = difficulty;
			break;
		case difficulty:
			if((tmpD & 0x1F) == 0)
				Menu = difficulty;
			else if((tmpD & 0x1F) == 0x01)
			{
				DIFF = 2;
				Menu = play;
				WRITE = 0;
			}
			else if((tmpD & 0x1F) == 0x02)
			{	
				DIFF = 1;
				Menu = play;
				WRITE = 0;
			}
			else
				Menu = difficulty;
			break;
		case play:
			Menu = wait_score;
			break;
		case wait_score:
			if(!XSCORE && !YSCORE)
				Menu = wait_score;
			else if(XSCORE)
				Menu = writeX;
			else if(YSCORE)
				Menu = writeY;
			break;
		case writeX:
			if(X == 4) //X WINS
				Menu = winX;
			else
				Menu = wait_score;
			break;
		case writeY:
			if(Y == 4) //Y WINS
				Menu = winY;
			else
				Menu = wait_score;
			break;
		case winX:
			Menu = holdwin;
			break;
		case winY:
			Menu = holdwin;
			break;
		case holdwin:
			MULT = 0;
			SINGLE = 0;
			if(counterwin == 50000)
			{
				counterwin = 0;
				Menu = home;
			}
			else
				Menu = holdwin;
			break;
		
		case rwait:
			if((tmpD & 0x1F) != 0)
			{
				Menu = rwait;
			}
			else
				Menu = rwait1;
			break;
		
		case rwait1:
			Menu = rpage1;
			break;
		case rpage1:
			if((tmpD & 0x1F) == 0)
			{
				Menu = rpage1;
			}
			else if((tmpD & 0x1F) == 0x04)
				Menu = rwait3;
			break;
		
		case rwait3:
			Menu = rpage3;
			break;
		case rpage3:
			if((tmpD & 0x1F) == 0)
			{
				Menu = rpage3;
			}
			else if((tmpD & 0x1F) == 0x04)
				Menu = rwait4;
			break;
		case rwait4:
			if((tmpD & 0x1F) != 0)
			{
				Menu = rwait4;
			}
			else
				Menu = home;
			break;
			
	}
	switch(Menu)
	{
		case home:
			FINISH = 0;
			READY = 0;
			if(WRITE == 0)
			{
				LCD_ClearScreen();
				LCD_DisplayString(1, "Play: 1P)  2P)  Records:");
				WRITE = 1;
			}
			break;
		case wait:
			WRITE = 0;
			break;
		case difficulty:
			if(WRITE == 0)
			{
				LCD_ClearScreen();
				LCD_DisplayString(1, "Difficulty:     1)Easy   2)Hard");
				WRITE = 1;			
			}
			break;
		case play:
			READY = 1;
			if(WRITE == 0)
			{
				LCD_DisplayString(1, "Score:          P1)0       P2)0");
				WRITE = 1;
			}
			break;
		case wait_score:
			break;
		case writeX:
			LCD_Cursor(20);
			LCD_WriteData(X + '0');
			break;
		case writeY:
			LCD_Cursor(31);
			LCD_WriteData(Y + '0');
			break;
		case winX:
			MULT = 0;
			SINGLE = 0;
			READY = 0;
			LCD_DisplayString(1, "    P1 WINS!");
			write_eeprom_word(&my_eeprom_array[0], read_eeprom_word(&my_eeprom_array[0]) + 1);
			write_eeprom_word(&my_eeprom_array[3], read_eeprom_word(&my_eeprom_array[3]) + 1);
			break;
		case winY:
			MULT = 0;
			SINGLE = 0;
			READY = 0;
			LCD_DisplayString(1, "    P2 WINS!");
			write_eeprom_word(&my_eeprom_array[0], read_eeprom_word(&my_eeprom_array[0]) + 1);
			write_eeprom_word(&my_eeprom_array[5], read_eeprom_word(&my_eeprom_array[5]) + 1);
			break;
		case holdwin:
			MULT = 0;
			SINGLE = 0;
			READY = 0;
			counterwin++;
			WRITE = 0;
			break;
			
		case rwait:
			break;	
		case rwait1:
			LCD_DisplayString(1, "    RECORDS:    # Played: 0");
			LCD_Cursor(27);
			LCD_WriteData(read_eeprom_word(&my_eeprom_array[0]) + '0');
			break;
		case rpage1:
			
			break;
		
		case rwait3:
			LCD_DisplayString(1, "WP1: 0   WP2: 0       END");
			LCD_Cursor(6);
			LCD_WriteData(read_eeprom_word(&my_eeprom_array[3]) + '0');
			LCD_Cursor(15);
			LCD_WriteData(read_eeprom_word(&my_eeprom_array[5]) + '0');
			break;
		case rpage3:
			break;
		case rwait4:
			WRITE = 0;
			break;
	}	
}


//UPDATE LED MATRIX
enum MatrixUpdate {wait_Ready, wait_Ready2, updateP1, updateP2, updateBall} Matrix;
unsigned char a = 3;

unsigned char up = 1;
unsigned short countermatrix = 0;
void MatrixPlay()
{
	switch(Matrix)
	{
		case wait_Ready:
			if(!READY)
			{
				if(countermatrix == 1000)
				{
					if(a == 0)
						up = 1;
					if(a == 5)
						up = -1;
					a = a + up;
					
					countermatrix = 0;
					Matrix = wait_Ready2;	
				}
				else
					Matrix = wait_Ready;
			}
			else if(READY)
				Matrix = updateP1;
			break;
		case wait_Ready2:
			if(!READY)
			{
				if(countermatrix == 1000)
				{
					if(a == 0)
						up = 1;
					if(a == 5)
					up = -1;
						a = a + up;
				
					countermatrix = 0;
					Matrix = wait_Ready;
				}
				else
					Matrix = wait_Ready2;
			}
			else if(READY)
				Matrix = updateP1;
			break;
		case updateP1:
			if(!READY)
				Matrix = wait_Ready;
			else if(READY)
				Matrix = updateP2;
			break;
		case updateP2:
			if(!READY)
				Matrix = wait_Ready;
			else if(READY)
				Matrix = updateBall;
			break;
		case updateBall:
			if(!READY)
				Matrix = wait_Ready;
			else if(READY)
				Matrix = updateP1;
			break;
	}
	switch(Matrix)
	{
		case wait_Ready:
			countermatrix++;
			PORTA = P1ROW_MOVEMENT[a];
			PORTB = 0x7E;
			P1COL = 0xFE;
			P2COL = 0x7F;
			break;
		case wait_Ready2:
			countermatrix++;
			PORTA = P1ROW_MOVEMENT[a];
			PORTB = 0x7E;
			P1COL = 0xFE;
			P2COL = 0x7F;
		break;
		case updateP1:
			PORTA = P1ROW_MOVEMENT[P1index];
			PORTB = P1COL;
			break;
		case updateP2:
			PORTA = P2ROW_MOVEMENT[P2index];
			PORTB = P2COL;
			break;
		case updateBall:
			PORTA = BALLROW_MOVEMENT[BRindex];
			PORTB = BALLCOL_MOVEMENT[BCindex];
			break;	
	}
}


//Movement P1
enum Movement_P1 {wait_Ready_p1, move_p1, up_p1, down_p1, holdp1} MovementP1;
unsigned short counter1 = 0;
void MoveP1()
{
	switch(MovementP1)
	{
		case wait_Ready_p1:
			if(!READY)
				MovementP1 = wait_Ready_p1;
			else if (READY)
				MovementP1 = move_p1;
			break;
		case move_p1:
			if(READY)
			{
				if((!(tmpD & 0x01) && !(tmpD & 0x02)) || ((tmpD & 0x01) && (tmpD & 0x02)))
					MovementP1 = move_p1;
				else if(tmpD & 0x01)
					MovementP1 = up_p1;
				else if(tmpD & 0x02)
					MovementP1 = down_p1;
			}
			else
				MovementP1 = wait_Ready_p1;
			break;
		case up_p1:
			MovementP1 = holdp1;
			break;
		case down_p1:
			MovementP1 = holdp1;
			break;
		case holdp1:
			if(counter1 == 2500)
				MovementP1 = move_p1;
			else
				MovementP1 = holdp1;
			break;
	}	
	switch(MovementP1)
	{
		case wait_Ready_p1:
			counter1 = 0;
			break;
		case move_p1:
			counter1 = 0;
			break;
		case up_p1:
			if(P1index < 5)
				P1index++;
			else
				P1index = P1index;
			break;
		case down_p1:
			if(P1index > 0)
				P1index--;
			else
				P1index = P1index;
			break;
		case holdp1:
			counter1++;
			break;
	}
}


//Movement P2
enum Movement_P2 {wait_Ready_p2, move_p2, up_p2, down_p2, holdp2} MovementP2;
unsigned short counter2 = 0;
void MoveP2()
{
	switch(MovementP2)
	{
		case wait_Ready_p2:
			if(!READY && !MULT)
				MovementP2 = wait_Ready_p2;
			else if (READY && MULT && !SINGLE)
				MovementP2 = move_p2;
			else
				MovementP2 = wait_Ready_p2;
			break;
		case move_p2:
			if(!READY)
				MovementP2 = wait_Ready_p2;
			else if(READY && MULT)
			{
				if((!(tmpD & 0x04) && !(tmpD & 0x08)) || ((tmpD & 0x04) && (tmpD & 0x08)))
					MovementP2 = move_p2;
				else if(tmpD & 0x04)
					MovementP2 = up_p2;
				else if(tmpD & 0x08)
					MovementP2 = down_p2;
			}
			break;
		case up_p2:
			MovementP2 = holdp2;
			break;
		case down_p2:
			MovementP2 = holdp2;
			break;
		case holdp2:
			if(counter2 == 2500)
				MovementP2 = move_p2;
			else
				MovementP2 = holdp2;
			break;
	}
	switch(MovementP2)
	{
		case wait_Ready_p2:
			counter2 = 0;
			break;
		case move_p2:
			counter2 = 0;
			break;
		case up_p2:
			if(P2index < 5)
				P2index++;
			else
				P2index = P2index;
			break;
		case down_p2:
			if(P2index > 0)
				P2index--;
			else
				P2index = P2index;
			break;
		case holdp2:
			counter2++;
			break;
	}
}



//Movement P2BOT
enum Movement_P2BOT {wait_Ready_p2bot, move_p2bot, up_p2bot, down_p2bot, holdp2bot} MovementP2bot;
unsigned short counter2bot = 0;
void MoveP2bot()
{
	switch(MovementP2bot)
	{
		case wait_Ready_p2bot:
			if(!READY && !SINGLE)
				MovementP2bot = wait_Ready_p2bot;
			else if (READY && SINGLE && !MULT)
				MovementP2bot = move_p2bot;
			else
				MovementP2bot = wait_Ready_p2bot;
			break;
		case move_p2bot:
			if(!READY)
				MovementP2 = wait_Ready_p2bot;
			else if(READY && SINGLE)
			{
				if((BRindex < P2index))
					MovementP2bot = down_p2bot;
				else if((BRindex > P2index))
					MovementP2bot = up_p2bot;
				else
					MovementP2bot = move_p2bot;	
			}
			break;
		case up_p2bot:
			MovementP2bot = holdp2bot;
			break;
		case down_p2bot:
			MovementP2bot = holdp2bot;
			break;
		case holdp2bot:
			if(counter2bot == 3000)
				MovementP2bot = move_p2bot;
			else
				MovementP2bot = holdp2bot;
			break;
	}
	switch(MovementP2bot)
	{
		case wait_Ready_p2bot:
			counter2bot = 0;
			break;
		case move_p2bot:
			counter2bot = 0;
			break;
		case up_p2bot:
			if(P2index < 5)
				P2index++;
			else
				P2index = P2index;
			break;
		case down_p2bot:
			if(P2index > 0)
				P2index--;
			else
				P2index = P2index;
			break;
		case holdp2bot:
			counter2bot++;
			break;
	}
}


//BALL MOVEMENT
enum BallMoves {wait_Ready_ball, start, start_wait, move_ball, holdball} BallMove;
unsigned short counterball = 0;
unsigned short counterstart = 0;
char hit = -1;
char wall = -1;
void BallPlay()
{
	switch(BallMove)
	{
		case wait_Ready_ball:
			if(!READY)
				BallMove = wait_Ready_ball;
			else if(READY)
				BallMove = start;
			break;
		case start:
			if(!READY)
				BallMove = wait_Ready_ball;
			else
				BallMove = start_wait;
			break;
		case start_wait:
			if(counterstart != 10000)
				BallMove = start_wait;
			else if(counterstart == 10000)
			{
				counterstart = 0;
				BallMove = move_ball;
			}	
			break;
		case move_ball:
			counterstart = 0;
			if(BCindex == 0)
			{
				YSCORE = 1;
				BallMove = start;
			}
			else if(BCindex == 7)
			{
				XSCORE = 1;
				BallMove = start;
			}
			else
				BallMove = holdball;
			break;
		case holdball:
			if(counterball != (1000 * DIFF))
				BallMove = holdball;
			else
			{
				counterball = 0;
				if(XSCORE)
					BallMove = start;
				else if(YSCORE)
					BallMove = start;
				else
					BallMove = move_ball;
			}
			break;
	}	
	switch(BallMove)
	{
		case wait_Ready_ball:
			XSCORE = 0;
			YSCORE = 0;
			break;
		case start:
			if(!XSCORE && !YSCORE)
			{
				hit = -1;
				wall = 0;
				BRindex = 4;
				BCindex = 4;
				P1index = 3;
				P2index = 3;
				XSCORE = 0;
				YSCORE = 0;
			}
			else if(XSCORE)
			{
				hit = 1;
				wall = 0;
				BRindex = 4;
				BCindex = 2;
				P1index = 3;
				P2index = 3;
				X++;
			}
			else if(YSCORE)
			{
				hit = -1;
				wall = 0;
				BRindex = 4;
				BCindex = 5;
				P1index = 3;
				P2index = 3;
				Y++;
			}
			break;
		case start_wait:
			XSCORE = 0;
			YSCORE = 0;
			counterstart++;
			break;
		case move_ball:
			counterstart = 0;
			BRindex = BRindex + wall;
			BCindex = BCindex + hit;
			
			if(BRindex > 6 || BRindex < 1)
				wall = wall * -1;
			
			if(BCindex < 2) //P1 side
			{	
				if(BRindex == 0)
				{
					if(P1index == 0)
					{
						hit = 1;
						wall = 1;
					}
				}
				else if(BRindex == 1)
				{
					if(P1index == 0)
					{
						hit = 1;
						wall = 0;
					}
					else if(P1index == 1)
					{
						hit = 1;
						wall = -1;
					}
				}
				else if(BRindex == 2)
				{
					if(P1index == 0)
					{
						hit = 1;
						wall = 1;
					}
					else if(P1index == 1)
					{
						hit = 1;
						wall = 0;
					}
					else if(P1index == 2)
					{
						hit = 1;
						wall = -1;
					}
				}
				else if(BRindex == 3)
				{
					if(P1index == 1)
					{
						hit = 1;
						wall = 1;
					}
					else if(P1index == 2)
					{
						hit = 1;
						wall = 0;
					}
					else if(P1index == 3)
					{
						hit = 1;
						wall = -1;
					}
				}
				else if(BRindex == 4)
				{
					if(P1index == 2)
					{
						hit = 1;
						wall = 1;
					}
					else if(P1index == 3)
					{
						hit = 1;
						wall = 0;
					}
					else if(P1index == 4)
					{
						hit = 1;
						wall = -1;
					}
				}
				else if(BRindex == 5)
				{
					if(P1index == 3)
					{
						hit = 1;
						wall = 1;
					}
					else if(P1index == 4)
					{
						hit = 1;
						wall = 0;
					}
					else if(P1index == 5)
					{
						hit = 1;
						wall = -1;
					}
				}
				else if(BRindex == 6)
				{
					if(P1index == 4)
					{
						hit = 1;
						wall = 0;
					}
					else if(P1index == 5)
					{
						hit = 1;
						wall = -1;
					}
				}
				else if(BRindex == 7)
				{
					if(P1index == 5)
					{
						hit = 1;
						wall = -1;
					}
				}
				else
				{
					YSCORE = 1;
					
				}
			}
			
			
			else if(BCindex > 5) //on P2 side
			{
				if(BRindex == 0)
				{
					if(P2index == 0)
					{
						hit = -1;
						wall = -1;
					}
				}
				else if(BRindex == 1)
				{
					if(P2index == 0)
					{
						hit = -1;
						wall = 0;
					}
					else if(P2index == 1)
					{
						hit = -1;
						wall = -1;
					}
				}
				else if(BRindex == 2)
				{
					if(P2index == 0)
					{
						hit = -1;
						wall = 1;
					}
					else if(P2index == 1)
					{
						hit = -1;
						wall = 0;
					}
					else if(P2index == 2)
					{
						hit = -1;
						wall = -1;
					}
				}
				else if(BRindex == 3)
				{
					if(P2index == 1)
					{
						hit = -1;
						wall = 1;
					}
					else if(P2index == 2)
					{
						hit = -1;
						wall = 0;
					}
					else if(P2index == 3)
					{
						hit = -1;
						wall = -1;
					}
				}
				else if(BRindex == 4)
				{
					if(P2index == 2)
					{
						hit = -1;
						wall = 1;
					}
					else if(P2index == 3)
					{
						hit = -1;
						wall = 0;
					}
					else if(P2index == 4)
					{
						hit = -1;
						wall = -1;
					}
				}
				else if(BRindex == 5)
				{
					if(P2index == 3)
					{
						hit = -1;
						wall = 1;
					}
					else if(P2index == 4)
					{
						hit = -1;
						wall = 0;
					}
					else if(P2index == 5)
					{
						hit = -1;
						wall = -1;
					}
				}
				else if(BRindex == 6)
				{
					if(P2index == 4)
					{
						hit = -1;
						wall = 0;
					}
					else if(P2index == 5)
					{
						hit = -1;
						wall = -1;
					}
				}
				else if(BRindex == 7)
				{
					if(P2index == 5)
					{
						hit = -1;
						wall = -1;
					}
				}
				else
				{
					XSCORE = 1;
				}
			}
			break;
		case holdball:
			counterball++;
			break;
	}
}


int main(void)
{
    DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xE0; PORTD = 0x1F; // LCD control lines
	LCD_init();
	
	TimerSet(1);
	TimerOn();
	
	if(read_eeprom_word(&my_eeprom_array[0]) + '0' == '/')
		write_eeprom_word(&my_eeprom_array[0], read_eeprom_word(&my_eeprom_array[0]) + 1);
	if(read_eeprom_word(&my_eeprom_array[5]) + '0' == '/')
		write_eeprom_word(&my_eeprom_array[5], read_eeprom_word(&my_eeprom_array[5]) + 1);
	if(read_eeprom_word(&my_eeprom_array[3]) + '0' == '/')
		write_eeprom_word(&my_eeprom_array[3], read_eeprom_word(&my_eeprom_array[3]) + 1);

	while(1)
    {
		
		tmpD = PIND;
		tmpD = ~tmpD;
		
		MenuScreen();
		MatrixPlay();
		MoveP1();
		MoveP2();
		MoveP2bot();
		BallPlay();
	

    }
}
