/*
 * LPZW TRAM8 4x4 formware
 * Created Jan 2026
 * by Dmitry S. Baikov
 * 
 * Based on the stock 1.3 firmware
 *
 * Original header:
 *
 * toggler.c
 *
 * Created: 22.08.2017 16:49:13
 * Author : Kay Knofe
 * CC BY-NC-ND 4.0
 *
 * Version 1.3
 * 
 */ 


#define BUTTON_LED_DEBUG

#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>

#include <avr/eeprom.h>
//#include <avr/boot.h>

#define F_CPU 16000000UL

#define BAUD 31250UL

#define MYUBRR (F_CPU/(16*BAUD)-1)

#include <util/delay.h>
#include "MIDI.h"
#include "general_twi.h"
#include "MAX5825.h"
//#include "bootloader.h"

enum HardwareConfigValues {
    NUM_OUT_GATES = 8,
    NUM_OUT_VOLTAGES = 8,
};

//PROTOTYPES
//void startupblink(void);
void midi_learn(void);
//void setup_process(void);
//void init_interrupt_pins(void);
void set_velocity(uint8_t ch, uint8_t velo);
void set_default(void);
void set_LED(uint8_t var);


void set_pin_inv(uint8_t pinnr);
void clear_pin_inv(uint8_t pinnr);


#define ENABLE 1
#define DISABLE 0
#define TOGGLE 2

#define TRUE 1
#define FALSE 0

#define MODE_CC 2
#define MODE_VELOCITY 1

#define EEPROM_CHANNEL_ADDR	(uint8_t *) 0x100 
#define EEPROM_MAP_ADDR		(uint8_t *) 0x101
#define EEPROM_MODE_ADDR	(uint8_t *) 0x110
#define	EEPROM_PRESET_ADDR	(uint8_t *) 0x111

#define LED_pin PC0
#define BUTTON_PIN PC1

	#define BUTTON_UP		1
	#define BUTTON_DOWN		2
	#define BUTTON_PRESSED	3
	#define BUTTON_RELEASED	4

// Timer configuration
// Adopted from https://github.com/thorinf/tram8-plus
#define TIMER_PRESCALER 64
#define TIMER_FREQ_HZ 1000 // 1kHz = 1ms tick
#define TIMER_OCR ((F_CPU / TIMER_PRESCALER / TIMER_FREQ_HZ) - 1)
#define TIMER_TICK_MS (1000 / TIMER_FREQ_HZ)


void timer_init(void) {
  TCCR2 = (1 << WGM21) | (1 << CS22);
  OCR2 = TIMER_OCR;
  TCNT2 = 0;
  TIMSK |= (1 << OCIE2);
}

/* state globals */
//uint16_t clock_tick = 0;

enum { CONFIGURATION_FORMAT = 1 };

// --- Gate outputs config

enum GateFlags {
    GateMode_Gate = 0,
    GateMode_Trigger = 1,
    GateMode_MASK = 0x01,
    
    GateSource_Clock = 0, // param: pulse count
    GateSource_Note = 2,  // param: note number
    GateSource_MASK = 0xFE,
};

struct GateState {
    uint8_t flags;
    uint8_t param;
    union {
        uint8_t ticks_until_clear;
        uint8_t current_output;
    };
    uint8_t midi_pulse_counter;
};

// --- CV outputs config

enum VoltageFlags {
    VoltageSource_NoteVelocity = 0,
    VoltageSource_ControlChange = 0x80,
    VoltageSource_MASK = 0x80,
    
    VoltageSource_Number_MASK = 0x7F,
};

struct VoltageState {
    uint8_t note_or_cc; // 0x00 | Note num or 0x80 | CC num
};

// --- default config settings (36 bytes))
uint8_t cfg[4+ 4+8*2+8*1 +4] = {
    // head marker
    0x90, 0x0d, 0xf0, 0x0d, // good food

    // common config
    CONFIGURATION_FORMAT, // version
    (1 << 4) | 9, // CV range 8V, (0 = 5V), MIDI channel number (0-15)
    10, // default trigger length in ms
    0, // reserved
    
    // 8 gates (2 bytes per gate)
    GateMode_Trigger | GateSource_Note,  36, // TR-8S BD
    GateMode_Trigger | GateSource_Note,  38, // TR-8S SD
    GateMode_Trigger | GateSource_Note,  42, // TR-8S CH
    GateMode_Trigger | GateSource_Note,  46, // TR-8S OH
    GateMode_Trigger | GateSource_Clock, 16, // 2/3 ppqn
    GateMode_Trigger | GateSource_Clock, 24, // 1 ppqn
    GateMode_Gate    | GateSource_Clock,  0, // RUN gate
    GateMode_Trigger | GateSource_Clock,  0, // RESET trigger
    
    // 8 CVs (1 byte per CV)
    VoltageSource_NoteVelocity | 36, // TR-8S BD
    VoltageSource_NoteVelocity | 38, // TR-8S SD
    VoltageSource_NoteVelocity | 42, // TR-8S CH
    VoltageSource_NoteVelocity | 46, // TR-8S OH
    VoltageSource_ControlChange | 24, // TR-8S BD Level
    VoltageSource_ControlChange | 29, // TR-8S SD Level
    VoltageSource_ControlChange | 63, // TR-8S CH Level
    VoltageSource_ControlChange | 82, // TR-8S OH Level
    
    // tail markrer
    0xba, 0xad, 0xf0, 0x0d, // baad food
};

struct GateState gates[NUM_OUT_GATES] = {};
struct VoltageState voltages[NUM_OUT_VOLTAGES] = {};

void show_error();
int parse_settings();

void gates_tick_update();

void gates_midi_reset(uint8_t reset_pulse_counters);
void gates_midi_pulse();
void gates_midi_stop();
void gates_noteon(uint8_t note);
void gates_noteoff(uint8_t note);
void gates_allnotesoff();

void voltage_note_or_cc(uint8_t note_or_cc, uint8_t val);

uint8_t eight_volts = 1;
uint8_t trigger_ticks = 10;

uint8_t midi_clock_run = 0;
uint8_t midi_cmd_len = 0;
uint8_t midi_cmd_data[3];

/* MIDI GLOBALS */

uint8_t midi_note_map[8] = {60,61,62,63,64,65,66,67};
uint8_t midi_note_map_default[8] = {60,61,62,63,64,65,66,67};

uint8_t midi_channel = 9;
uint8_t midi_learn_mode = 0;
uint8_t midi_learn_current = 0;

uint8_t module_mode = MODE_VELOCITY;

uint8_t velocity_out = 0;

uint16_t setting_wait_counter = 0;
uint8_t setting_wait_flag = 0;

//POINTER MANIPULATION 
void  (*set_pin_ptr)(uint8_t ) = & set_pin_inv;
void  (*clear_pin_ptr)(uint8_t ) = & clear_pin_inv;

void show_error()
{
    while(true)
    {
    	set_LED(ENABLE);
	   _delay_ms(100);
	   set_LED(DISABLE);
	   _delay_ms(100);
    }
}

int parse_settings()
{
    if (cfg[0] != 0x90 || cfg[1] != 0x0D || cfg[2] != 0xF0 || cfg[3] != 0x0D)
        return 0;
    if (cfg[4] != CONFIGURATION_FORMAT)
        return 0;
    eight_volts = (cfg[5] >> 4) & 0x01;
    midi_channel = cfg[5] & 0x0F;
    trigger_ticks = cfg[6];
    // cfg[7] reserved
    
    const uint8_t gate_cfg_off = 8;
    for (uint8_t i = 0; i < NUM_OUT_GATES; ++i)
    {
        gates[i].flags = cfg[gate_cfg_off + i*2 + 0];
        gates[i].param = cfg[gate_cfg_off + i*2 + 1];
        gates[i].current_output = 0;
        gates[i].midi_pulse_counter = 0;
    }
    
    const uint8_t cv_cfg_off = gate_cfg_off + NUM_OUT_GATES * 2;
    for (uint8_t i = 0; i < NUM_OUT_VOLTAGES; ++i)
        voltages[i].note_or_cc = cfg[cv_cfg_off + i];
        
    const uint8_t tail_off = cv_cfg_off + NUM_OUT_VOLTAGES;
    if (cfg[tail_off] != 0xBA || cfg[tail_off+1] != 0xAD
        || cfg[tail_off+2] != 0xF0 || cfg[tail_off+3] != 0x0D)
        return 0;
        
    return 1;
}

int main(void)
{

/* GPIO INIT */
	//DDRA = 0xFF; //ROW&COLUMN FOR LED/BUTTON	
	DDRC = 0x0C | (1 << LED_pin) | (1 << BUTTON_PIN); //LDAC & CLEAR & LED
	DDRB = 0x01; // Trigger Out 0	
	DDRD = 0xFE; //Trigger outs 1-7
	//DDRE = 0xFC; //ENABLE outs 7-2
	//DDRG = 0x03; //ENABLE outs 1,0
    //DDRF = 0x0C; // 0,1 Button Ins 2,3 Clock outs 
   
	//PORTC |= (1 << BUTTON_PIN);
    PORTD |= 0xFE; //ALL GATES LOW (Inverter Out)
    PORTB |= 0x01; // 
	
	set_LED(ENABLE);
	_delay_ms(300);
	set_LED(DISABLE);

	#define BUTTONFIXVARIABLE (uint8_t *) 0x07
	//THIS PART IS FOR HARDWARE VERSION 1.5 and up so Code is backwards compatible
	uint8_t buttonfix_flag = 0;

	do {} while (!eeprom_is_ready());
	//load Channel
	buttonfix_flag = eeprom_read_byte(BUTTONFIXVARIABLE);

	if (buttonfix_flag == 0xAA)
	{
		DDRC &= ~((1 << BUTTON_PIN));//INPUT
	}


	
	//DIRECT_BUTTONS_DDR &= ~(1 << DIRECT_GROUPBUTTON_bit); //GROUP BUTTON IS INPUT
	//DIRECT_BUTTONS_PORT |= (1 << DIRECT_GROUPBUTTON_bit); //PULLUP SWITCH TO GND
	
/* GUI INIT */ //GLOABL NOW
	/* LED INIT */
	//LEDs = { {LED_ON,LED_ON,LED_ON,LED_ON,LED_ON,LED_ON,LED_ON,LED_ON},LED_OFF,LED_OFF,LED_OFF,LED_OFF };
	//row_select = 0;				
	/* BUTTONS INIT */
	//buttons = { {BUTTON_UP,BUTTON_UP,BUTTON_UP,BUTTON_UP,BUTTON_UP,BUTTON_UP,BUTTON_UP,BUTTON_UP},BUTTON_UP,BUTTON_UP };	
				
	/* Blink Timer Init */
//	TCCR1B = 0b010; //FREE RUNNING, PORT disconnected, /64
	/* Button Poll Timer */
	TCCR2 = (1 << WGM20)|(0 << WGM21)|(0b111<<CS20);//CTC, PORT disconnected, /1024
	OCR2 = 157;//ca. 10ms@16MHz
	
	/* MIDI INIT */
	UCSRB = (1<<RXCIE)|(1<<RXEN);
	UCSRC = (1<<UCSZ0)|(1<<UCSZ1);
	UBRRH = (unsigned char)(MYUBRR>>8);
	UBRRL = (unsigned char) MYUBRR;
	
/* LOAD MIDI MAP FROM EEPROM*/
	do {} while (!eeprom_is_ready());
	//load Channel 
	midi_channel = eeprom_read_byte(EEPROM_CHANNEL_ADDR);
	do {} while (!eeprom_is_ready());
	//load map
	eeprom_read_block(&midi_note_map,EEPROM_MAP_ADDR,8);	
	do {} while (!eeprom_is_ready());
	//load Channel
	module_mode = eeprom_read_byte(EEPROM_MODE_ADDR);	
	
	
	
	do {} while (!eeprom_is_ready());
	//set map to default if never learned:
	if (midi_note_map[0]==0xFF)
		set_default();

	
/* CHECK FOR EXPANDERS */
	//uint8_t i2cread[5] = {NULL};
	//TWI_READ_BULK(0x20,0x00,2,&i2cread);
	PORTC = (1 << PC2)|(1 << PC3);	
	DDRC  |= (1 << PC2)|(1 << PC3);
	
	velocity_out = test_max5825();	//Velocity Out Expander present? (a.k.a. WK4) 

	if (!parse_settings())
	   show_error();
	   
//	if(velocity_out){
		init_max5825(eight_volts);
		init_max5825(eight_volts);		
//		max5825_set_load_channel(0,0xFfff);
//	}

	//GET HARDWARE REVISION
	PORTB |= (1 << PB1); //PULLUP
	DDRB &= ~(1 << PB1); //INPUT
	_delay_ms(100);
	
	if ((PINB >> PB1) &1){
		//OLD HARDWARE
		//SET PINS INVERSE
		set_pin_ptr = & set_pin_inv;
		clear_pin_ptr = & clear_pin_inv;
		
		} else {
		//NEW HARDWARE - PIN TO GND
		//SET PINS NORMAL
		set_pin_ptr = & clear_pin_inv;
		clear_pin_ptr = & set_pin_inv;
	}

		(*clear_pin_ptr)(0);
		(*clear_pin_ptr)(1);
		(*clear_pin_ptr)(2);
		(*clear_pin_ptr)(3);
		(*clear_pin_ptr)(4);
		(*clear_pin_ptr)(5);
		(*clear_pin_ptr)(6);
		(*clear_pin_ptr)(7);


	midi_learn_mode = 0;
	midi_cmd_len = 0;
	midi_clock_run = 0;
	
	timer_init();
	
/* INTERRUPTS ENABLE */
	sei();
	
			//if((PINC_TEMP & 0x0F)==3) //GROUP&SYNC Presses
			//boot_main();	
			
	
/* MAIN LOOP */	
    while (1) {
	
	static uint8_t learn_button = BUTTON_UP;
	static uint8_t button_now = 0;
	static uint8_t button_bounce = 0;
	static uint8_t button_last = 0;
	

	//CHECK FOR GOTO SETTINGS MENU - LEARN BUTTON HAS TO BE DOWN FOR 3 SEC 
	if ((learn_button == BUTTON_RELEASED)){
		setting_wait_counter = 0;
		setting_wait_flag = 0;
		learn_button = BUTTON_UP;
	//	set_LED(DISABLE);
	}

	if (learn_button == BUTTON_PRESSED){
		learn_button = BUTTON_DOWN;
		//set_LED(ENABLE);		
		if(setting_wait_flag == 0){	//START THE WAIT
			setting_wait_counter = 0;
			setting_wait_flag = 1;
	
			//LEDs.groupLED = LED_BLINK2;

		}
	}
		
	if(learn_button == BUTTON_DOWN){
		if(setting_wait_flag == 1){
			if (setting_wait_counter >=200){
			//	set_LED(ENABLE);
				setting_wait_counter = 0;
				setting_wait_flag = 0;				
				learn_button = BUTTON_UP;
				//midi_learn();			
			}
		}
	}
		
	

	/************************************************************************/
	/*                    keyscan                                           */
	/************************************************************************/
		if ((TIFR>>OCF2)&1)
		{
			//keyscan(&buttons);
			TCNT2 = 0; //reset timer
			TIFR |= (1 << OCF2); //reset flag		
			
			button_now = PINC & (1 << BUTTON_PIN);
			
			if (button_now != button_bounce){
				button_bounce = button_now;			
			}else{			
										
				if (button_now != button_last){			
					
					if(button_now == 0){
						learn_button = BUTTON_RELEASED;
						//set_LED(DISABLE);
					}else{
						learn_button = BUTTON_PRESSED;		
					//	set_LED(ENABLE);
					}
						
					button_last = button_now;		
				}			
			}

			if(setting_wait_flag == 1)
				setting_wait_counter++; 
		}
	 }
}

/************************************************************************/
/*						 MIDI LEARN			                            */
/************************************************************************/


void midi_learn(void)
{	
	static uint8_t learn_button = BUTTON_UP;
	static uint8_t button_now = 0;
	static uint8_t button_bounce = 0;
	static uint8_t button_last = 0;
	uint8_t blink_counter = 1;
	uint8_t firstrelease_flag = 1;
		
	midi_learn_mode = 1; //set global flag for MIDI RX ISR
	midi_learn_current = 0;
	//ALL OFF
	(*clear_pin_ptr)(0);
	(*clear_pin_ptr)(1);
	(*clear_pin_ptr)(2);
	(*clear_pin_ptr)(3);
	(*clear_pin_ptr)(4);
	(*clear_pin_ptr)(5);
	(*clear_pin_ptr)(6);
	(*clear_pin_ptr)(7);

	set_LED(ENABLE);
	
	while(midi_learn_current<8){

			//CHECK FOR LEAVE SETTINGS MENU - LEARN BUTTON HAS TO BE DOWN FOR 3 SEC
			if ((learn_button == BUTTON_RELEASED)){
				setting_wait_counter = 0;
				setting_wait_flag = 0;
				learn_button = BUTTON_UP;
	
				if (!firstrelease_flag){
					if (module_mode==MODE_VELOCITY)
						module_mode = MODE_CC;
					else
						module_mode = MODE_VELOCITY;
				}
			
				firstrelease_flag = 0;
			}

			if (learn_button == BUTTON_PRESSED){
				learn_button = BUTTON_DOWN;
				//set_LED(ENABLE);
				if(setting_wait_flag == 0){	//START THE WAIT
					setting_wait_counter = 0;
					setting_wait_flag = 1;
				}
			}
			
			if(learn_button == BUTTON_DOWN){
				if(setting_wait_flag == 1){
					if (setting_wait_counter >=200){
						setting_wait_counter = 0;
						setting_wait_flag = 0;			
						midi_learn_current = 8; // leave 
					}
				}
			}//END BUTTON DOWN 

		
		
		
		
		
		/************************************************************************/
		/*                    keyscan                                           */
		/************************************************************************/
		if ((TIFR>>OCF2)&1)
		{
			//keyscan(&buttons);
			TCNT2 = 0; //reset timer
			TIFR |= (1 << OCF2); //reset flag
				
			button_now = PINC & (1 << BUTTON_PIN);
			
			blink_counter++;
			
			if ((blink_counter&0xF) == 0){
				if (module_mode==MODE_VELOCITY)
					set_LED(ENABLE);
				else
					set_LED(TOGGLE);
			}	
					
			if (button_now != button_bounce){
				button_bounce = button_now;
				}else{
					
				if (button_now != button_last){
						
					if(button_now == 0){
						learn_button = BUTTON_RELEASED;
						//set_LED(DISABLE);
						}else{
						learn_button = BUTTON_PRESSED;
						//	set_LED(ENABLE);
					}
						
					button_last = button_now;
				}
			}

			if(setting_wait_flag == 1)
			setting_wait_counter++;
		} //KEYSCAN END

	} //STAY HERE LOOP END
	
	midi_learn_mode=0;//DONE LEARNING!
	
	set_LED(DISABLE);


	//SAVE TO EEPROM 
	do {} while (!eeprom_is_ready());
	//save Channel
	//midi_channel = eeprom_read_byte(EEPROM_CHANNEL_ADDR);
	eeprom_write_byte(EEPROM_CHANNEL_ADDR,midi_channel);
	
	do {} while (!eeprom_is_ready());
	//save Channel
	//midi_channel = eeprom_read_byte(EEPROM_CHANNEL_ADDR);
	eeprom_write_byte(EEPROM_MODE_ADDR,module_mode);
			
	do {} while (!eeprom_is_ready());
	//load map
	//	eeprom_read_block(&midi_note_map,EEPROM_MAP_ADDR,8);
	eeprom_write_block(&midi_note_map,EEPROM_MAP_ADDR,8);

	//ALL OFF
	(*clear_pin_ptr)(0);
	(*clear_pin_ptr)(1);
	(*clear_pin_ptr)(2);
	(*clear_pin_ptr)(3);
	(*clear_pin_ptr)(4);
	(*clear_pin_ptr)(5);
	(*clear_pin_ptr)(6);
	(*clear_pin_ptr)(7);

	//end midi learn mode in MIDI RX ISR
	
	return;
	
}


/************************************************************************/

void set_default(void){
	//MIDI
	midi_channel = 9;
	module_mode = MODE_VELOCITY;
	memcpy(&midi_note_map,&midi_note_map_default,8);
	////clock divider
	////reset pulse edge
	//presets.clk_prescaler = DISABLE;
	//presets.reset_invert = DISABLE;
	//presets.velocity_mute = DISABLE;
	//presets.midi_conv_en = ENABLE;
	
	//SAVE TO EEPROM
	do {} while (!eeprom_is_ready());
	//save Channel
	//midi_channel = eeprom_read_byte(EEPROM_CHANNEL_ADDR);
	eeprom_write_byte(EEPROM_CHANNEL_ADDR,midi_channel);
	
	do {} while (!eeprom_is_ready());/*				SET DEFAULTS                                            */
/************************************************************************/


	//load map
	//	eeprom_read_block(&midi_note_map,EEPROM_MAP_ADDR,8);
	eeprom_write_block(&midi_note_map,EEPROM_MAP_ADDR,8);
	
	//SAVE TO EEPROM
	do {} while (!eeprom_is_ready());
	//save Channel
	//midi_channel = eeprom_read_byte(EEPROM_CHANNEL_ADDR);
	eeprom_write_byte(EEPROM_MODE_ADDR,module_mode);	
	
	//do {} while (!eeprom_is_ready());
	////load map
	////	eeprom_read_block(&midi_note_map,EEPROM_MAP_ADDR,8);
	//eeprom_write_block(&presets,EEPROM_PRESET_ADDR,sizeof(sys_presets));
	
	return;
}

ISR(TIMER2_COMP_vect)
{
    gates_tick_update();
}

void midi_realtime(uint8_t msg)
{
	   if (msg == MIDI_MSG_CLOCK)
	       gates_midi_pulse();
	   else if (msg == MIDI_MSG_START)
	       gates_midi_reset(1);
	   else if (msg == MIDI_MSG_CONTINUE)
	       gates_midi_reset(0);
	   else if (msg == MIDI_MSG_STOP)
	       gates_midi_stop();
}

void midi_command(uint8_t msg, uint8_t ctl, uint8_t val)
{
    if (msg == MIDI_MSG_NOTE_OFF)
    {
        //voltage_note_or_cc(ctl, val);
        gates_noteoff(ctl);
    }
    else if (msg == MIDI_MSG_NOTE_ON)
    {
        if (val != 0)
        {
            voltage_note_or_cc(ctl, val);
            gates_noteon(ctl);
        }
        else
            gates_noteoff(ctl);
    }
    else if (msg == MIDI_MSG_CONTROL_CHANGE) // Control Change
    {
        if (ctl == MIDI_CC_ALL_NOTES_OFF || ctl == MIDI_CC_ALL_SOUNDS_OFF)
            gates_allnotesoff();
        else
            voltage_note_or_cc(ctl | VoltageSource_ControlChange, val);
    }
}

/* MIDI RECEIVER */
ISR(USART_RXC_vect)
{
	uint8_t uart_data;
	uart_data = UDR;
	
	if (uart_data & MIDI_STATUS_MASK) // command byte
	{
	   if ((uart_data & MIDI_REALTIME_MASK) == MIDI_REALTIME_MASK)
	   {
	       midi_realtime(uart_data);
	   }
	   else
	   {
    	   const uint8_t msg_type = uart_data & MIDI_COMMAND_MASK;
    	   midi_cmd_len = 0;
    	   if (msg_type == MIDI_MSG_NOTE_ON
    	       || msg_type == MIDI_MSG_NOTE_OFF
    	       || msg_type == MIDI_MSG_CONTROL_CHANGE)
    	   {
    	       const uint8_t channel = uart_data & MIDI_CHANNEL_MASK;
    	       if (channel == midi_channel)
    	       {
    	           midi_cmd_data[0] = msg_type;
    	           midi_cmd_len = 1;
    	       }
    	   }
	   }
	}
	else if (midi_cmd_len > 0)// && midi_cmd_len < 3)
	{
	   midi_cmd_data[midi_cmd_len++] = uart_data;
	   if (midi_cmd_len == 3)
	   {
	       midi_command(midi_cmd_data[0], midi_cmd_data[1], midi_cmd_data[2]);
	       midi_cmd_len = 1; // Running Status support
	   }
	}
}

void gates_tick_update()
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_GATES; ++i)
        if ((gates[i].flags & GateMode_MASK) == GateMode_Trigger)
            if (gates[i].ticks_until_clear)
                if (--gates[i].ticks_until_clear == 0)
                    (*clear_pin_ptr)(i);           
}

void gates_midi_reset(uint8_t reset_pulse_counters)
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_GATES; ++i)
    {
        const uint8_t gate_flags = gates[i].flags;
        if ((gate_flags & GateSource_MASK) == GateSource_Clock)
        {
            const uint16_t pulse_count = gates[i].param;
            const uint8_t needs_reset = (pulse_count > 0)
                ? reset_pulse_counters
                : (gate_flags & GateMode_MASK) == GateMode_Gate;
            if (needs_reset)
            {
                gates[i].midi_pulse_counter = 0;
                gates[i].ticks_until_clear = 0; // gates[i].current_output = 0;
                (*clear_pin_ptr)(i);
            }
            else if (pulse_count == 0 && (gate_flags & GateMode_MASK) == GateMode_Trigger)
            {
                // output MIDI RESET trigger
                gates[i].ticks_until_clear = trigger_ticks;
                (*set_pin_ptr)(i);
            }
        }
    }
    midi_clock_run = 1;
}

void gates_midi_pulse()
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_GATES; ++i)
    {
        const uint8_t gate_flags = gates[i].flags;
        if ((gate_flags & GateSource_MASK) == GateSource_Clock)
        {
            const uint16_t pulse_count = gates[i].param;
            if (pulse_count)
            {
                if (++gates[i].midi_pulse_counter >= pulse_count)
                {
                    gates[i].midi_pulse_counter = 0;
                    if ((gate_flags & GateMode_MASK) == GateMode_Trigger)
                    {
                        gates[i].ticks_until_clear = trigger_ticks;
                        (*set_pin_ptr)(i);
                    }
                    else // GateMode_Gate
                    {
                        const uint8_t new_output = (gates[i].current_output ^= 1);
                        if (new_output)
                            (*set_pin_ptr)(i);
                        else
                            (*clear_pin_ptr)(i);
                    }
                }
            }
            else if (gate_flags == (GateSource_Clock | GateMode_Gate))
            {
                if (midi_clock_run && gates[i].midi_pulse_counter == 0)
                {
                    gates[i].midi_pulse_counter = 1;
                    (*set_pin_ptr)(i);
                }
            }
        }
    }
}

void gates_midi_stop()
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_GATES; ++i)
    {
        const uint8_t gate_flags = gates[i].flags;
        if (gate_flags == (GateSource_Clock | GateMode_Gate))
        {
            if (gates[i].midi_pulse_counter > 0)
            {
                gates[i].midi_pulse_counter = 0;
                (*clear_pin_ptr)(i);
            }
        }
    }
    midi_clock_run = 0;
}

void gates_allnotesoff()
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_GATES; ++i)
        if (gates[i].flags == (GateMode_Gate | GateSource_Note))
            (*clear_pin_ptr)(i);           
}


void gates_noteoff(uint8_t note)
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_GATES; ++i)
        if (gates[i].flags == (GateMode_Gate | GateSource_Note))
            if (gates[i].param == note)
                (*clear_pin_ptr)(i);           
}

void gates_noteon(uint8_t note)
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_GATES; ++i)
    {
        const uint8_t gate_flags = gates[i].flags;
        if ((gates[i].flags & GateSource_MASK) == GateSource_Note)
            if (gates[i].param == note)
            {
                (*set_pin_ptr)(i);
                if ((gate_flags & GateMode_MASK) == GateMode_Trigger)
                    gates[i].ticks_until_clear = trigger_ticks;
            }
    }
}

void voltage_note_or_cc(uint8_t note_or_cc, uint8_t val)
{
    uint8_t i;
    for (i = 0; i < NUM_OUT_VOLTAGES; ++i)
        if (voltages[i].note_or_cc == note_or_cc)
            max5825_set_load_channel(7 - i, velocity_lookup[val & 0x7F]);
}

void set_LED(uint8_t var){
	
	if (var==ENABLE)
		DDRC |= (1<<LED_pin);
	
	if(var==DISABLE)
		DDRC &= 0xFF^(1 << LED_pin);
	
	if(var==TOGGLE)
		DDRC ^= (1<<LED_pin);
	
	return;
}



void set_pin_inv(uint8_t pinnr){
	//inverted cos of 74HC1G14 inverter
	switch(7 - pinnr){
		case 0: PORTB &= 0xFE;
		break;
		case 1: PORTD &= 0xFD;
		break;
		case 2: PORTD &= 0xFB;
		break;
		case 3: PORTD &= 0xF7;
		break;
		case 4: PORTD &= 0xEF;
		break;
		case 5: PORTD &= 0xDF;
		break;
		case 6: PORTD &= 0xBF;
		break;
		case 7: PORTD &= 0x7F;
		break;
		default: break;
	}
	
	return;
}

void clear_pin_inv(uint8_t pinnr){
	switch(7 - pinnr){
		case 0: PORTB |= 0x01;
		break;
		case 1: PORTD |= 0x02;
		break;
		case 2: PORTD |= 0x04;
		break;
		case 3: PORTD |= 0x08;
		break;
		case 4: PORTD |= 0x10;
		break;
		case 5: PORTD |= 0x20;
		break;
		case 6: PORTD |= 0x40;
		break;
		case 7: PORTD |= 0x80;
		break;
		default: break;
	}
	
	return;
}