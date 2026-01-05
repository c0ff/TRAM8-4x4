/*
 * general_twi.h
 *
 * Created: 10.09.2013 10:51:59
 *  Author: Kay
 */ 


#ifndef GENERAL_TWI_H_
#define GENERAL_TWI_H_

#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <util/twi.h>

#define TWCR_START 0xA4		//TWINT=1 TWSTA=1 TWEN=1 - Startcondition
#define TWCR_RESTART (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)| (1<<TWEA)
#define TWCR_STOP 0x94		//TWINT=1 TWSTO=1 TWEN=1 - Stopcondition
#define TWCR_SEND 0x84		//TWINT=1 TWEN=1 - Sendeimpuls
#define TWCR_GET (1<<TWINT)|(1<<TWEN)| (1<<TWEA)

uint8_t TWI_WRITE_BYTE(uint8_t ICAddr, uint8_t RegAddr, uint8_t Value);
uint8_t TWI_READ_BYTE(uint8_t ICAddr, uint8_t RegAddr);
uint8_t TWI_READ_BULK(uint8_t ICAddr, uint8_t Offset, uint8_t Length, uint8_t (*Readout)[]);
uint8_t TWI_WRITE_BULK(uint8_t ICAddr, uint8_t Offset, uint8_t Length, uint8_t (*Filldata)[]);
void TWI_ERROR(void);

#endif /* GENERAL_TWI_H_ */