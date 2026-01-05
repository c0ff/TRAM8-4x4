/*
 * general_twi.c
 *
 * Created: 10.09.2013 10:51:44
 *  Author: Kay
 */ 

#include "general_twi.h"
#include <stdio.h>


#define TW_Bautrate 12 //400khz  (no prescaler, F_CPU=16M)
 // #define TW_Bautrate 2 //400khz


uint8_t TWI_WRITE_BYTE(uint8_t ICAddr, uint8_t RegAddr, uint8_t Value){
	
	
	TWSR=0;					//prescaler = 1 = (4^0)
 	TWBR=TW_Bautrate;						
	
	//STOP
	TWCR=TWCR_STOP;

	
	/****Start ****/
	TWCR=TWCR_START;
	while (!(TWCR & (1<<TWINT)))
	{
		//warten bis interupt-flag gesetzt ist
	}
	if (TW_STATUS!=TW_START)
	return 0;
		


	/****Adresse ****/
	TWDR=	ICAddr | TW_WRITE;					//Adresse
	TWCR=TWCR_SEND;
	while (!(TWCR & (1<<TWINT)))
	{
		//warten bis interupt-flag gesetzt ist
	}
	if (TW_STATUS!=TW_MT_SLA_ACK)
	return 0;
		
	
	
	/****Auswahl Register + auto increment=0****/
	TWDR=RegAddr;
	TWCR=TWCR_SEND;
	while (!(TWCR & (1<<TWINT)))
	{
		//warten bis interupt-flag gesetzt ist
	}
	//Ueberpruefen, ob Daten angekommen sind
	if (TW_STATUS!=TW_MT_DATA_ACK)
	return 0;
	
	
	
	/****Übertragen Wert****/
	TWDR=Value;
	//TWDR=37 , PWM0=0,5
	TWCR=TWCR_SEND;
	while (!(TWCR & (1<<TWINT)))
	{
		//warten bis interupt-flag gesetzt ist
	}
	//Ueberpruefen, ob Daten angekommen sind
	if (TW_STATUS!=TW_MT_DATA_ACK)
	return 0;
	

	//STOP
	TWCR=TWCR_STOP;

	return 1;	
	
}



uint8_t TWI_READ_BYTE(uint8_t ICAddr, uint8_t RegAddr){
		
		uint8_t Returnval = 0;
	
		TWSR=0;					//prescaler = 1 = (4^0)
 		TWBR=TW_Bautrate;						
	
		//STOP
		TWCR=TWCR_STOP;

	
		/****Start ****/
		TWCR=TWCR_START;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_START)
		return 0;
		


		/****Adresse ****/
		TWDR=	ICAddr | TW_WRITE;					//Adresse _ peudo write
		TWCR=TWCR_SEND;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_MT_SLA_ACK)
		return 0;
		
	
	
		/****Auswahl Register + auto increment=0****/
		TWDR=RegAddr;
		TWCR=TWCR_SEND;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		//Ueberpruefen, ob Daten angekommen sind
		if (TW_STATUS!=TW_MT_DATA_ACK)
		return 0;
	
		TWCR=TWCR_RESTART;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_REP_START)
		return 0;
	
		/****Übertragen Wert****/
		TWDR=	ICAddr | TW_READ;					//Adresse _ peudo write		
		TWCR=TWCR_GET;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		//Ueberpruefen, ob Daten angekommen sind
		if (TW_STATUS!=TW_MR_SLA_ACK)
		return 0;
		
		TWCR=TWCR_GET;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		//Ueberpruefen, ob Daten angekommen sind
		if (TW_STATUS!=TW_MR_DATA_ACK)
		return 0;
				
	
		Returnval = TWDR;
	
	
		// ENDE 	
		TWCR=TWCR_SEND;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}


		//STOP
		TWCR=TWCR_STOP;

		return Returnval;	

}


uint8_t TWI_READ_BULK(uint8_t ICAddr, uint8_t Offset, uint8_t Length, uint8_t (*Readout)[]){
		
		//uint8_t Returnval = 0;
		
		TWSR=0;					//prescaler = 1 = (4^0)
		TWBR=TW_Bautrate;
		
		//STOP
		TWCR=TWCR_STOP;

		
		/****Start ****/
		TWCR=TWCR_START;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_START)
		{
			TWI_ERROR();
			return 0;
		}

		


		/****Adresse ****/
		TWDR=	ICAddr | TW_WRITE;					//Adresse _ peudo write
		TWCR=TWCR_SEND;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_MT_SLA_ACK)
		{
			TWI_ERROR();
			return 0;
		}
		
		
		/****Auswahl Register + auto increment=0****/
		TWDR=Offset;
		TWCR=TWCR_SEND;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		//Ueberpruefen, ob Daten angekommen sind
		if (TW_STATUS!=TW_MT_DATA_ACK)
		{
			TWI_ERROR();
			return 0;
		}
		
		TWCR=TWCR_RESTART;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_REP_START)
		{
			TWI_ERROR();
			return 0;
		}
		
		/****Übertragen Wert****/
		TWDR=	ICAddr | TW_READ;					//Adresse _ peudo write
		TWCR=TWCR_SEND;//VON THOMAS .. VORHER TWCR_GET
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		//Ueberpruefen, ob Daten angekommen sind
		if (TW_STATUS!=TW_MR_SLA_ACK)
		{
			TWI_ERROR();
			return 0;
		}
	
		uint8_t i = 0;
		
		for (i=0;i<Length-1;i++){
		
			TWCR=TWCR_GET;
			while (!(TWCR & (1<<TWINT)))
			{
				//warten bis interupt-flag gesetzt ist
			}
			//Ueberpruefen, ob Daten angekommen sind
			if (TW_STATUS!=TW_MR_DATA_ACK)
			{
				TWI_ERROR();
				return 0;
			}		
		
			(*Readout)[i] = TWDR;
			
			uint8_t j=0;
			for (j=0;j<20;j++)
			{
					__asm__ volatile ("nop");
			}
			
			
		}
		
		// ENDE
		TWCR=TWCR_SEND;

		
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}

		(*Readout)[Length-1] = TWDR;		
		//STOP
		TWCR=TWCR_STOP;

		return 1;
	
}


uint8_t TWI_WRITE_BULK(uint8_t ICAddr, uint8_t Offset, uint8_t Length, uint8_t (*Filldata)[]){
			
	//	uint8_t Returnval = 0;
		
		TWSR=0;					//prescaler = 1 = (4^0)
		TWBR=TW_Bautrate;
		
		//STOP
		TWCR=TWCR_STOP;

		
		/****Start ****/
		TWCR=TWCR_START;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_START)
		{
			TWI_ERROR();
			return 0;
		}

		


		/****Adresse ****/
		TWDR=	ICAddr | TW_WRITE;					//Adresse  write
		TWCR=TWCR_SEND;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		if (TW_STATUS!=TW_MT_SLA_ACK)
		{
			TWI_ERROR();
			return 0;
		}
		
		
		/****Auswahl Register + auto increment=0****/
		TWDR=Offset;
		TWCR=TWCR_SEND;
		while (!(TWCR & (1<<TWINT)))
		{
			//warten bis interupt-flag gesetzt ist
		}
		//Ueberpruefen, ob Daten angekommen sind
		if (TW_STATUS!=TW_MT_DATA_ACK)
		{
			TWI_ERROR();
			return 0;
		}
		
		
		uint8_t i = 0;
		
		for (i=0;i<Length;i++){
			/****Übertragen Wert****/
			TWDR = (*Filldata)[i];
			TWCR=TWCR_SEND;
			while (!(TWCR & (1<<TWINT)))
			{
				//warten bis interupt-flag gesetzt ist
			}
			//Ueberpruefen, ob Daten angekommen sind
			if (TW_STATUS!=TW_MT_DATA_ACK)
			{
				TWI_ERROR();
				return 0;
			}
			
			//uint8_t j=0;
			//for (j=0;j<20;j++)
			//{
					//__asm__ volatile ("nop");
			//}
			//
			
		}
		
	
		//STOP
		TWCR=TWCR_STOP;

		return 1;
	
	
	
		return 0;
}

void TWI_ERROR(void){
	
		TWCR=TWCR_STOP;
		
	#ifdef MY_DEBUG
		printf("TWI Error - Status Byte: %x \n",TW_STATUS);	
	#endif
	
	return;
}
