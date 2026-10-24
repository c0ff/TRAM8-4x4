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
//void midi_learn(void);
//void setup_process(void);
//void init_interrupt_pins(void);
void set_velocity(uint8_t ch, uint8_t velo);
//void set_default(void);
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
enum { CONFIGURATION_FORMAT = 7, CONFIGURATION_FORMAT_MIN = 7 };

// --- Outputs config

enum OutputSourceFlags {
   // 0x80 unused, MIDI Note Off
   OutputSource_NoteOn = 0x90, // MIDI Note On
   OutputSource_AfterTouch = 0xA0,
   OutputSource_ControlChange = 0xB0,
   // 0xC0 unused, MIDI Program Change
   OutputSource_ChannelPressure = 0xD0,
   OutputSource_PitchWheel = 0xE0,
   OutputSource_Clock = 0xF0,
   
   OutputSource_MASK = 0xF0,
};

enum OutputModeFlags {
    OutputMode_MASK = 0x0F,
    
    OutputClock_Trigger = 0, // with param==0 outputs RESET trigger
    OutputClock_FlipFlop = 1, // with param==0 outputs RUN Gate
    
    OutputNote_Trigger = 0,
    OutputNote_Gate = 1,
    OutputNote_Velocity = 2,
    OutputNote_DynamicGate = 3,
    OutputNote_Pitch = 4,
};

typedef void (*SetOutputFunc)(uint8_t id, uint16_t value);
struct OutputState {
    uint8_t midi_channel; // 4 bits
    uint8_t flags;
    uint8_t param;
    volatile uint8_t ticks_until_clear;
    volatile uint16_t current_output;
    volatile uint16_t desired_output;
    uint16_t midi_pulse_counter;
    SetOutputFunc set_output;
};

enum { OutputValue_MAX = 0x7F };

void set_output_pin(uint8_t id, uint16_t value);
void set_output_pin_inv(uint8_t id, uint16_t value);

void set_output_cv7bit(uint8_t id, uint16_t value);
//void set_output_cv7bit_8to5(uint8_t id, uint16_t value); // TODO
//void set_output_cv_pitch(uint8_t id, uint16_t value); // TODO
//void set_output_cv14bit(uint8_t id, uint16_t value); // TODO
void set_output_none(uint8_t, uint16_t) {}

enum GlobalFlags {
    GlobalVoltageRangeHigh = 0x10,
    GlobalResetOnContinue = 0x20,
    GlobalMidiChannelMask = 0x0F,
};

// --- default config settings
uint8_t cfg[56] = {
    // head marker
    0x90, 0x0d, 0xf0, 0x0d, // good food

    // Offset: 4
    // common config
    CONFIGURATION_FORMAT, // version
    GlobalVoltageRangeHigh | GlobalResetOnContinue| 9, // default MIDI channel number (0-15)
    10, // default trigger length in ms
    (3 << 1) | 1, // Clock Scale: 1/4 note
        // 4 lower bits: Clock Divider Scale, bit 0: multiply by 3, bits [1:3]: power of 2
       // Example: value b0000 means 1, b0001 = 3x, b0111 = 24x
    0, // TODO: when in 8V mode, each bit tells if a CV output is scaled back to 5V
    0, // TODO: reserved
    // Add global bytes before this line.
    
    // Offset: 10
    // MIDI Channel mappings
    
    // Gate outputs MIDI channel map
    0, // each bit marks which gate uses a non-default MIDI-channel
    0,0,0,0,// each 4-bit group sets custom MIDI channel for a gate out.

    // Voltage outputs MIDI channel map
    0, // each bit marks which CV uses a non-default MIDI-channel
    0,0,0,0,// each 4-bit group sets custom MIDI channel for a CV out.

    // Offset: 20
    // 8 gates (2 bytes per gate)
    OutputSource_NoteOn | OutputNote_Trigger,  36, // TR-8S BD
    OutputSource_NoteOn | OutputNote_Trigger,  38, // TR-8S SD
    OutputSource_NoteOn | OutputNote_Trigger,  42, // TR-8S CH
    OutputSource_NoteOn | OutputNote_Trigger,  46, // TR-8S OH
    OutputSource_NoteOn | OutputNote_Trigger,  39, // TR-8S BD ALT
    OutputSource_NoteOn | OutputNote_Trigger,  35, // TR-8S SD ALT
    OutputSource_NoteOn | OutputNote_Trigger,  40, // TR-8S CP
    OutputSource_Clock  | OutputClock_Trigger,  1, // 1 ppqn
    
    // 8 CVs (2 byte per CV)
    OutputSource_NoteOn | OutputNote_Velocity, 36, // TR-8S BD
    OutputSource_NoteOn | OutputNote_Velocity, 38, // TR-8S SD
    OutputSource_NoteOn | OutputNote_Velocity, 42, // TR-8S CH
    OutputSource_NoteOn | OutputNote_Velocity, 46, // TR-8S OH
    OutputSource_NoteOn | OutputNote_Velocity, 39, // TR-8S BD ALT
    OutputSource_NoteOn | OutputNote_Velocity, 35, // TR-8S SD ALT
    OutputSource_NoteOn | OutputNote_Velocity, 40, // TR-8S CP
    OutputSource_Clock  | OutputClock_Trigger,  0, // RESET trigger
    
    // tail marker
    0xba, 0xad, 0xf0, 0x0d, // baad food
};

enum { NUM_OUTPUTS = NUM_OUT_GATES + NUM_OUT_VOLTAGES };
struct OutputState output[NUM_OUTPUTS] = {};

void on_timer_tick();

void on_midi_start(uint8_t reset_pulse_counters);
void on_midi_pulse();
void on_midi_stop();
void on_note_on(uint8_t chan, uint8_t note, uint8_t vel);
void on_note_off(uint8_t chan, uint8_t note, uint8_t vel);
void on_after_touch(uint8_t chan, uint8_t note, uint8_t val);
void on_control_change(uint8_t chan, uint8_t ctl, uint8_t val);
void on_program_change(uint8_t chan, uint8_t val);
void on_channel_pressure(uint8_t chan, uint8_t val);
void on_pitch_wheel(uint8_t chan, int8_t d1, uint8_t d2);

uint8_t eight_volts = 1;
uint8_t trigger_ticks = 10;

uint8_t midi_clock_mode = 0;
uint8_t midi_reset_on_continue = 1;
uint8_t midi_clock_run = 0;
uint8_t midi_cmd_len = 0;
uint8_t midi_cmd_got = 0;
uint8_t midi_cmd_data[3];

/* MIDI GLOBALS */

//uint8_t midi_note_map[8] = {60,61,62,63,64,65,66,67};
//uint8_t midi_note_map_default[8] = {60,61,62,63,64,65,66,67};

uint8_t global_midi_channel = 9;

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

void parse_channel_mapping(const uint8_t* chmap, struct OutputState* out)
{
    uint8_t chmask = *chmap++;
    for (uint8_t i = 0; i < 8; ++i)
    {
        if (chmask & (1 << i))
            out->midi_channel = (chmap[i >> 1] >> ((i & 1) ? 4 : 0)) & 0x0F;
        else
            out->midi_channel = global_midi_channel;
        ++out;
    }
}

SetOutputFunc get_output_func(uint8_t outid, uint8_t flags, uint8_t old_hw)
{
	const uint8_t src = flags & OutputSource_MASK;
	if (outid < NUM_OUT_GATES)
		return old_hw ? &set_output_pin_inv : &set_output_pin;
    else if (src == OutputSource_PitchWheel)
        return &set_output_none; // TODO: set_output_cv14bit
    else
        return &set_output_cv7bit; // TODO: set_output_cv7bit_8to5, set_output_cv_pitch
}

int parse_settings(uint8_t old_hardware)
{
    if (cfg[0] != 0x90 || cfg[1] != 0x0D || cfg[2] != 0xF0 || cfg[3] != 0x0D)
        return 0;
    if (cfg[4] < CONFIGURATION_FORMAT_MIN || cfg[4] > CONFIGURATION_FORMAT)
        return 0;
    eight_volts = (cfg[5] & GlobalVoltageRangeHigh) ? 1 : 0;
    midi_reset_on_continue = (cfg[5] & GlobalResetOnContinue) ? 1 : 0;
    global_midi_channel = cfg[5] & GlobalMidiChannelMask;
    trigger_ticks = cfg[6];
    midi_clock_mode = cfg[7] & 0x0F;
    // cfg[8] bits 4-7 reserved
    
    const uint8_t chan_map_off = 10;
    parse_channel_mapping(cfg + chan_map_off + 0, &output[0]);
    parse_channel_mapping(cfg + chan_map_off + 5, &output[NUM_OUT_GATES]);
    
    const uint8_t out_cfg_off = 20;
    for (uint8_t i = 0; i < NUM_OUTPUTS; ++i)
    {
        output[i].flags = cfg[out_cfg_off + i*2 + 0];
        output[i].param = cfg[out_cfg_off + i*2 + 1];
        output[i].ticks_until_clear = 0;
        output[i].current_output = 0;
        output[i].desired_output = 0;
        output[i].midi_pulse_counter = 0;
    	output[i].set_output = get_output_func(i, output[i].flags, old_hardware);
    }
    
    const uint8_t tail_off = out_cfg_off + NUM_OUTPUTS * 2;
    if (cfg[tail_off] != 0xBA || cfg[tail_off+1] != 0xAD
        || cfg[tail_off+2] != 0xF0 || cfg[tail_off+3] != 0x0D)
        return 0;
        
    return 1;
}

#define IMMEDIATE_OUTPUT

void set_output(uint8_t i, uint16_t new_value)
{
    output[i].desired_output = new_value;
    if (new_value == 0)
    	output[i].ticks_until_clear = 0;
#ifdef IMMEDIATE_OUTPUT
    if (output[i].current_output != new_value)
    {
		output[i].set_output(i, new_value);
	    output[i].current_output = new_value;
    }
#endif	
}

#define FOR_ALL_OUTPUTS(i) for (int8_t i = NUM_OUTPUTS - 1; i >= 0; --i)
void update_outputs()
{
#ifndef IMMEDIATE_OUTPUT
	FOR_ALL_OUTPUTS(i)
	{
		const uint16_t desired = output[i].desired_output;
	    if (output[i].current_output != desired)
	    {
	    	output[i].set_output(i, desired);
	    	output[i].current_output = desired;
	    }
	}
#endif
}

struct ButtonState {
	uint8_t midi_learn_mode;
	uint8_t midi_learn_current;
	uint8_t setting_wait_flag;
	uint16_t setting_wait_counter;

	uint8_t learn_button;
	uint8_t button_now;
	uint8_t button_bounce;
	uint8_t button_last;
};
void read_button(struct ButtonState* bs);

struct ButtonState the_button = { 0 };

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
#if 0
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
#endif

	
/* CHECK FOR EXPANDERS */
	//uint8_t i2cread[5] = {NULL};
	//TWI_READ_BULK(0x20,0x00,2,&i2cread);
	PORTC = (1 << PC2)|(1 << PC3);	
	DDRC  |= (1 << PC2)|(1 << PC3);
	
	//const uint8_t velocity_out = test_max5825();	//Velocity Out Expander present? (a.k.a. WK4) 

	//GET HARDWARE REVISION
	PORTB |= (1 << PB1); //PULLUP
	DDRB &= ~(1 << PB1); //INPUT
	_delay_ms(100);
	
	const uint8_t old_hw = (PINB >> PB1) & 1;
	if (!parse_settings(old_hw))
	   show_error();
	   
	init_max5825(eight_volts);
	init_max5825(eight_volts);		
	//max5825_set_load_channel(0, 0xFEF0);

	for (uint8_t i = 0; i < NUM_OUTPUTS; ++i)
		output[i].set_output(i, 0);

	//midi_learn_mode = 0;
	midi_cmd_len = 0;
	midi_clock_run = 0;
	
	timer_init();
	
/* INTERRUPTS ENABLE */
	sei();
	
			//if((PINC_TEMP & 0x0F)==3) //GROUP&SYNC Presses
			//boot_main();	
			
	
	the_button.learn_button = BUTTON_UP;
/* MAIN LOOP */	
    while (1)
    {
    	update_outputs();
    	read_button(&the_button);
	}
}

void read_button(struct ButtonState*)
{
#if 0
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
#endif		
	

	/************************************************************************/
	/*                    keyscan                                           */
	/************************************************************************/
		if ((TIFR>>OCF2)&1)
		{
			//keyscan(&buttons);
			TCNT2 = 0; //reset timer
			TIFR |= (1 << OCF2); //reset flag		
#if 0			
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
#endif
		}
}

/************************************************************************/
/*						 MIDI LEARN			                            */
/************************************************************************/
#if 0
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
	global_midi_channel = 9;
	//module_mode = MODE_VELOCITY;
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
#endif

ISR(TIMER2_COMP_vect)
{
    on_timer_tick();
}

void on_midi_realtime(uint8_t msg)
{
	   if (msg == MIDI_MSG_CLOCK)
	       on_midi_pulse();
	   else if (msg == MIDI_MSG_START)
	       on_midi_start(1);
	   else if (msg == MIDI_MSG_CONTINUE)
	       on_midi_start(midi_reset_on_continue);
	   else if (msg == MIDI_MSG_STOP)
	       on_midi_stop();
}

void on_midi_command(uint8_t status, uint8_t d1, uint8_t d2)
{
    const uint8_t msg = status & MIDI_COMMAND_MASK;
    const uint8_t chan = status & MIDI_CHANNEL_MASK;

    if (msg == MIDI_MSG_NOTE_OFF)
        on_note_off(chan, d1, d2);
    else if (msg == MIDI_MSG_NOTE_ON)
    {
        if (d2 != 0)
            on_note_on(chan, d1, d2);
        else
            on_note_off(chan, d1, 0);
    }
    else if (msg == MIDI_MSG_CONTROL_CHANGE)
        on_control_change(chan, d1, d2);
    else if (msg == MIDI_MSG_PROGRAM_CHANGE)
    	on_program_change(chan, d1);
    else if (msg == MIDI_MSG_CHANNEL_PRESSURE)
    	on_channel_pressure(chan, d1);
    else if (msg == MIDI_MSG_AFTER_TOUCH)
    	on_after_touch(chan, d1, d2);
    else if (msg == MIDI_MSG_PITCH_WHEEL)
    	on_pitch_wheel(chan, d1, d2);
}

/* MIDI RECEIVER */
ISR(USART_RXC_vect)
{
	const uint8_t uart_data = UDR;
	
	if (uart_data & MIDI_STATUS_MASK) // command byte
	{
	   if ((uart_data & MIDI_REALTIME_MASK) == MIDI_REALTIME_MASK)
	   {
	       on_midi_realtime(uart_data);
	   }
	   else
	   {
    	   const uint8_t cmd = uart_data & MIDI_COMMAND_MASK;
    	   midi_cmd_got = 0; // reset Running Status
    	   if (cmd == MIDI_MSG_PROGRAM_CHANGE
    	       || cmd == MIDI_MSG_CHANNEL_PRESSURE)
    	   {
	           midi_cmd_data[0] = uart_data;
	           midi_cmd_got = 1;
	           midi_cmd_len = 2;
    	   }
    	   else if (cmd == MIDI_MSG_NOTE_ON
    	       || cmd == MIDI_MSG_NOTE_OFF
    	       || cmd == MIDI_MSG_CONTROL_CHANGE
    	       || cmd == MIDI_MSG_AFTER_TOUCH
    	       || cmd == MIDI_MSG_PITCH_WHEEL)
    	   {
	           midi_cmd_data[0] = uart_data;
	           midi_cmd_got = 1;
	           midi_cmd_len = 3;
    	   }
	   }
	}
	else if (midi_cmd_got > 0)// && midi_cmd_got < 3)
	{
	   midi_cmd_data[midi_cmd_got++] = uart_data;
	   if (midi_cmd_got == midi_cmd_len)
	   {
	       on_midi_command(midi_cmd_data[0], midi_cmd_data[1], midi_cmd_data[2]);
	       midi_cmd_got = 1; // Running Status support
	   }
	}
}

void on_timer_tick()
{
    FOR_ALL_OUTPUTS(i)
        if (output[i].ticks_until_clear && output[i].current_output)
            if (--output[i].ticks_until_clear == 0)
            	set_output(i, 0);
}

uint16_t clock_param_to_pulses(uint16_t param)
{
    const uint8_t power2 = (midi_clock_mode >> 1) & 0x07;
    if (midi_clock_mode & 1)
        return ((param << 1) + param) << power2; // x3
    else
        return param << power2;
}

void on_midi_start(uint8_t reset_pulse_counters)
{
    FOR_ALL_OUTPUTS(i)
        if ((output[i].flags & OutputSource_MASK) == OutputSource_Clock)
        {
        	const uint8_t clkmode = output[i].flags & OutputMode_MASK;
            const uint16_t pulse_count = clock_param_to_pulses(output[i].param);
            if (pulse_count)
            {
            	if (reset_pulse_counters)
            	{
                	output[i].midi_pulse_counter = pulse_count - 1; // fire on the next pulse
                	output[i].ticks_until_clear = 0;
                	set_output(i, 0);
            	}
            }
            else if (clkmode == OutputClock_Trigger) // RESET trigger
            {
                output[i].ticks_until_clear = trigger_ticks;
                set_output(i, OutputValue_MAX);
            }
            else if (reset_pulse_counters) // RUN gate
            {
                output[i].midi_pulse_counter = 0;
                //output[i].ticks_until_clear = 0;
                set_output(i, 0);
            }
        }
    midi_clock_run = 1;
}

void on_midi_pulse()
{
    FOR_ALL_OUTPUTS(i)
        if ((output[i].flags & OutputSource_MASK) == OutputSource_Clock)
        {
        	const uint8_t clkmode = output[i].flags & OutputMode_MASK;
            const uint16_t pulse_count = clock_param_to_pulses(output[i].param);
            if (pulse_count)
            {
                if (++output[i].midi_pulse_counter >= pulse_count)
                {
                    output[i].midi_pulse_counter = 0;
                    if (clkmode == OutputClock_Trigger)
                    {
                        output[i].ticks_until_clear = trigger_ticks;
                        set_output(i, OutputValue_MAX);
                    }
                    else // OutputClock_FlipFlop
                    {
                    	//output[i].ticks_until_clear = 0;
                        set_output(i, output[i].current_output ? 0 : OutputValue_MAX);
                    }
                }
            }
            else if (clkmode == OutputClock_FlipFlop) // RUN gate
            {
                if (midi_clock_run && output[i].midi_pulse_counter == 0)
                {
                    output[i].midi_pulse_counter = 1;
                    set_output(i, OutputValue_MAX);
                }
            }
        }
}

void on_midi_stop()
{
    FOR_ALL_OUTPUTS(i)
        if (output[i].flags == (OutputSource_Clock | OutputClock_FlipFlop) && output[i].param == 0) // RUN gate
        {
            if (output[i].midi_pulse_counter)
            {
                output[i].midi_pulse_counter = 0;
                set_output(i, 0);
            }
        }
    midi_clock_run = 0;
}

void on_cc_alloff()
{
    FOR_ALL_OUTPUTS(i)
    {
    	const uint8_t src = output[i].flags & OutputSource_MASK;
        if (src != OutputSource_Clock)
        {
        	if (src != OutputSource_NoteOn)
        		set_output(i, 0); // All Controllers Off
        	else if (output[i].flags & OutputNote_Gate)
        		set_output(i, 0); // All Notes Off
        }
    }
}

void on_note_off(uint8_t chan, uint8_t note, uint8_t)
{
    FOR_ALL_OUTPUTS(i)
    	if (output[i].midi_channel == chan && (output[i].flags & OutputSource_MASK) == OutputSource_NoteOn)
    	{
    		const uint8_t mode = output[i].flags & OutputMode_MASK;
    		if (mode == OutputNote_Gate || mode == OutputNote_DynamicGate)
        		if (output[i].param == note)
    				set_output(i, 0);
    	}
}

void on_note_on(uint8_t chan, uint8_t note, uint8_t vel)
{
    FOR_ALL_OUTPUTS(i)
        if (output[i].midi_channel == chan && (output[i].flags & OutputSource_MASK) == OutputSource_NoteOn)
        {
        	const uint8_t mode = output[i].flags & OutputMode_MASK;
        	if (mode == OutputNote_Pitch)
        		set_output(i, note & 0x7F);
        	else if (output[i].param == note)
        	{
        		if (mode == OutputNote_Trigger)
        		{
                   	output[i].ticks_until_clear = trigger_ticks;
        			set_output(i, OutputValue_MAX);
        		}
        		else if (mode == OutputNote_Gate)
        			set_output(i, OutputValue_MAX);
        		else if (mode == OutputNote_Velocity || mode == OutputNote_DynamicGate)
        			set_output(i, vel & 0x7F);
        	}
    	}	
}

void on_after_touch(uint8_t chan, uint8_t note, uint8_t val)
{
    FOR_ALL_OUTPUTS(i)
        if (output[i].midi_channel == chan && (output[i].flags & OutputSource_MASK) == OutputSource_AfterTouch)
            if (output[i].param == note)
            	set_output(i, val & 0x7F);
}

void on_control_change(uint8_t chan, uint8_t ctl, uint8_t val)
{
    if (ctl == MIDI_CC_ALL_NOTES_OFF || ctl == MIDI_CC_ALL_SOUNDS_OFF)
    {
        on_cc_alloff();
        return;
    }

    FOR_ALL_OUTPUTS(i)
        if (output[i].midi_channel == chan && (output[i].flags & OutputSource_MASK) == OutputSource_ControlChange)
            if (output[i].param == ctl)
            	set_output(i, val & 0x7F);
}

void on_program_change(uint8_t, uint8_t)
{
	// TODO: switch presets?
}

void on_channel_pressure(uint8_t chan, uint8_t val)
{
    FOR_ALL_OUTPUTS(i)
        if (output[i].midi_channel == chan && (output[i].flags & OutputSource_MASK) == OutputSource_ChannelPressure)
            set_output(i, val & 0x7F);
}

void on_pitch_wheel(uint8_t, int8_t, uint8_t)
{
	// TODO
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

void set_output_pin(uint8_t id, uint16_t value)
{
	if (value < 0x40)
	    set_pin_inv(id);
	else
	    clear_pin_inv(id);
}

void set_output_pin_inv(uint8_t id, uint16_t value)
{
	if (value < 0x40)
	    clear_pin_inv(id);
	else
	    set_pin_inv(id);
}

void set_output_cv7bit(uint8_t id, uint16_t value)
{
	if (id >= NUM_OUT_GATES)
    	max5825_set_load_channel((7 + NUM_OUT_GATES) - id, velocity_lookup[(uint8_t)value & 0x7F]);
}