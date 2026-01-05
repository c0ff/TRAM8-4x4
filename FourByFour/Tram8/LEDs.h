/*
 * LEDs.h
 *
 * Created: 22.08.2017 16:54:36
 *  Author: Kay
 */ 


#ifndef LEDS_H_
#define LEDS_H_

#include <avr/io.h>

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


#define LED_OFF		1
#define LED_ON		2
#define LED_BLINK1	3 //ultraslow ca. 1s p cycle
#define LED_BLINK2	4 //slow ca. 500ms p cycle
#define LED_BLINK3	5 //normal ca. 250ms p cycle
#define LED_BLINK4	6 //fast ca. 125ms p cycle

typedef struct {
	uint8_t muteLED[8];
	uint8_t groupLED;
	uint8_t syncLED;
	uint8_t runLED;				
	uint8_t boundaryLED;					
} all_LED_struct;


void do_led_cycle(all_LED_struct * all_leds,uint8_t select);
uint8_t on_off_decider(uint8_t * LED_status, uint8_t timer_status);

#endif /* LEDS_H_ */