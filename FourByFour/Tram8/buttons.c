/*
 * CFile1.c
 *
 * Created: 24.08.2017 14:39:01
 *  Author: Kay
 */ 

#include "buttons.h"

#include "LEDs.h" //ROW & COLUMN DEFINITIONEN HIER
/*
 #define ROW0 (1 << 0)
 #define ROW1 (1 << 1)
 #define ROW2 (1 << 2)
 #define ROW3 (1 << 3)

 #define COLUMN0 (0b1110 << 4)
 #define COLUMN1 (0b1101 << 4)
 #define COLUMN2 (0b1011 << 4)
 #define COLUMN3 (0b0111 << 4)

 #define NOROW 0
 #define NOCOLUMN (0xF << 4)
*/


//routine gets called ca every 10ms via ISR or polling
uint8_t keyscan(all_buttons_struct * all_buttons){
	uint8_t anykey = 0;
	
	static uint8_t buttons_now[3]; //array für 3 rows istwert
	static uint8_t buttons_bounce[3]; //array für 3 rows debounce variable	
	static uint8_t buttons_last[3]; //array für 3 rows vergangenheit	
	
	uint8_t temp_porta;
	
	temp_porta = PORTA;

	uint8_t i=0;
	for (i=0;i<3;i++)
	{
		PORTA = NOCOLUMN|NOROW;
		
		DDRC |= 0x0F; //kurz mal die eingänge niederohmig gegen GND
		PORTC &= 0xF0;
		asm volatile ("nop");	
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
		DDRC &= 0xF0;
		
		PORTA = (NOCOLUMN | (ROW0 << i));
			
		asm volatile ("nop");
		asm volatile ("nop");			
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");

				
		buttons_now[i] = PINC & 0x0F;
			
		PORTA = temp_porta; //set LEDs again
		
#ifdef SCAN_GROUPBUTTON_DIRECT
		if(i == 2){ //ONLY DO THIS WHEN IT'S DUE - DON'T MESS UP THE TIMING 
			buttons_now[2] &= 0xFD; //MASK GROUP BUTTON
			if(!((DIRECT_BUTTONS_PIN>>DIRECT_GROUPBUTTON_bit)&1)) //WHEN PIN 0 BUTTON PRESSED
				buttons_now[2] |= 0x02;		
		}
#endif
		
		if (buttons_now[i] != buttons_bounce[i])
			buttons_bounce[i]=buttons_now[i];
		else{
			if (buttons_now[i] != buttons_last[i]){
				anykey = 1; //something changed!! 

				//ROW0
				if (i==0){
					uint8_t j=0; //find changed pos
					for (j=0;j<4;j++){
						if (((buttons_now[i]>>j)&1) > ((buttons_last[i]>>j)&1))
							all_buttons->mutebutton[j] = BUTTON_PRESSED;
						if (((buttons_now[i]>>j)&1) < ((buttons_last[i]>>j)&1))
							all_buttons->mutebutton[j] = BUTTON_RELEASED;
					}
				}
				//ROW1
				if (i==1){
					uint8_t j=0; //find changed pos
					for (j=0;j<4;j++){
						if (((buttons_now[i]>>j)&1) > ((buttons_last[i]>>j)&1))
							all_buttons->mutebutton[(j+4)] = BUTTON_PRESSED;
						if (((buttons_now[i]>>j)&1) < ((buttons_last[i]>>j)&1))
							all_buttons->mutebutton[(j+4)] = BUTTON_RELEASED;
					}
				}
				//ROW2
				if (i==2){
						if ((buttons_now[i]&1) > (buttons_last[i]&1))
							all_buttons->syncbutton = BUTTON_PRESSED;
						if ((buttons_now[i]&1) < (buttons_last[i]&1))
							all_buttons->syncbutton = BUTTON_RELEASED;

						if ((buttons_now[i]&2) > (buttons_last[i]&2))
							all_buttons->groupbutton = BUTTON_PRESSED;
						if ((buttons_now[i]&2) < (buttons_last[i]&2))
							all_buttons->groupbutton = BUTTON_RELEASED;																
				}
			//save change last
			buttons_last[i] = buttons_now[i];			
			} 
		}
	}			
	return anykey;
}
