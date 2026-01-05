/*
 * buttons.h
 *
 * Created: 23.08.2017 12:17:19
 *  Author: Kay
 */ 


#ifndef BUTTONS_H_
#define BUTTONS_H_

	#include <avr/io.h>
    #ifndef PORTA
    #define PORTA           _SFR_IO8(0x1B)
    #endif

	#define BUTTON_UP		1
	#define BUTTON_DOWN		2	
	#define BUTTON_PRESSED	3
	#define BUTTON_RELEASED	4

	#define DIRECT_BUTTONS_PORT PORTF
	#define DIRECT_BUTTONS_DDR	DDRF
	#define DIRECT_BUTTONS_PIN	PIN0
	
	#define DIRECT_GROUPBUTTON_bit	1
	#define DIRECT_SYNCBUTTON_bit	0
	
	#define SCAN_GROUPBUTTON_DIRECT
	
	typedef struct {
		uint8_t mutebutton[8];
		uint8_t groupbutton;
		uint8_t syncbutton;
	} all_buttons_struct;
	
	

uint8_t keyscan(all_buttons_struct * all_buttons); //routine gets called ca every 10ms via ISR or polling 


#endif /* BUTTONS_H_ */