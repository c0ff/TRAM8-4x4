/*
 * LEDs.c
 *
 * Created: 23.08.2017 12:40:52
 *  Author: Kay
 */ 
#include "LEDs.h"
#include <avr/io.h>
#ifndef PORTA
#define PORTA           _SFR_IO8(0x1B)
#endif

void do_led_cycle(all_LED_struct * all_leds,uint8_t select){
	uint8_t columns = 0xF0;
	static volatile uint8_t timer = 0;
	
	//timer = (TCNT1 & 0xF0);

	if ((TIFR>>TOV1)&1)
	{
		TIFR |= (1<<TOV1);
		timer++;
	}
	
	if (select == 0)
	{
		if (on_off_decider(&all_leds->muteLED[0],timer))
			columns &= COLUMN0;
		if (on_off_decider(&all_leds->muteLED[1],timer))
			columns &= COLUMN1;		
		if (on_off_decider(&all_leds->muteLED[2],timer))
			columns &= COLUMN2;
		if (on_off_decider(&all_leds->muteLED[3],timer))
			columns &= COLUMN3;	
	
		PORTA = (columns & 0xF0)|ROW0; //OUT
	}
	
	if (select == 1)
	{
		if (on_off_decider(&all_leds->muteLED[4],timer))
			columns &= COLUMN0;
		if (on_off_decider(&all_leds->muteLED[5],timer))
			columns &= COLUMN1;		
		if (on_off_decider(&all_leds->muteLED[6],timer))
			columns &= COLUMN2;
		if (on_off_decider(&all_leds->muteLED[7],timer))
			columns &= COLUMN3;	
	
		PORTA = (columns & 0xF0)|ROW1; //OUT
	}	
	
	if (select == 2)
	{
		if (on_off_decider(&all_leds->syncLED,timer))
			columns &= COLUMN0;
		if (on_off_decider(&all_leds->groupLED,timer))
			columns &= COLUMN1;		
		if (on_off_decider(&all_leds->runLED,timer))
			columns &= COLUMN2;
		if (on_off_decider(&all_leds->boundaryLED,timer))
			columns &= COLUMN3;	
	
		PORTA = (columns & 0xF0)|ROW2; //OUT
	}		
	
	return;
}


uint8_t on_off_decider(uint8_t * LED_stat, uint8_t timer_status){
	volatile uint8_t onoff = 0;
	
	switch (*LED_stat)
	{
	case LED_OFF:
		break;
	case LED_ON:
		onoff = 1;
		break;
	case LED_BLINK1:
		if((timer_status>>3)&1)
			onoff = 1;		
		break;
	case LED_BLINK2:
		if((timer_status>>2)&1)
			onoff = 1;
		break;	
	case LED_BLINK3:
		if((timer_status>>1)&1)
			onoff = 1;
		break;
	case LED_BLINK4:
		if(timer_status&1)
			onoff = 1;
		break;	
	default:
	
		break;				
	}
		
	return onoff;
}