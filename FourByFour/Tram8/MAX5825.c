/*
 * MAX5825.c
 *
 * Created: 08.03.18 08:57:40
 *  Author: kay
 */ 

#include <avr/io.h>
#include "MAX5825.h"
#include "general_twi.h"

void init_max5825(uint8_t eightVolts)
{
	uint8_t data[2] = {0};
	// INT REF 0b101 = 2.5V 0b111 = 4.1V ALL DAC PWR ON
	const uint8_t ref_value = eightVolts ? 0b111 : 0b101;
	TWI_WRITE_BULK(MAX5825_ADDR,(MAX5825_REG_REF | ref_value), 2, &data);	
	TWI_WRITE_BULK(MAX5825_ADDR,MAX5825_REG_CODELOADALL,2,&data); // all to zero 
	
	return;
}

uint8_t test_max5825(void){
	uint8_t retval = 0;
	uint8_t buffer[2] = {0};
	
	retval = TWI_READ_BULK(MAX5825_ADDR,0,2,&buffer);
	
	return retval;
}


void max5825_set_load_channel(uint8_t ch, uint16_t value){
	
	uint8_t cmd_addr = (MAX5825_REG_CODEn_LOADn | (ch & 0x0F));
	uint8_t data[2];
	
	data[0]= (uint8_t) ((value>>8) & 0xFF);
	data[1]= (uint8_t) (value & 0x00F0);

	TWI_WRITE_BULK(MAX5825_ADDR,cmd_addr,2,&data);
	
	return;
}