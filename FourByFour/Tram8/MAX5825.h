/*
 * MAX5825.h
 *
 * Created: 08.03.18 08:57:13
 *  Author: kay
 */ 


#ifndef MAX5825_H_
#define MAX5825_H_

#define MAX58_TOP_ADDR 0x20
#define MAX58_SELECT_ADDR 0x0

#define MAX5825_ADDR (MAX58_TOP_ADDR | (MAX58_SELECT_ADDR << 1))

#define MAX5825_REG_WDOG			0x10
#define MAX5825_REG_REF				0x20
#define MAX5825_REG_GATE_CLR		0x30
#define MAX5825_REG_GATE_SET		0x31
#define MAX5825_REG_WDR				0x32
#define MAX5825_REG_WDRESET			0x33
#define MAX5825_REG_SW_CLR			0x34
#define MAX5825_REG_SW_RESET		0x35
#define MAX5825_REG_POWER			0x40
#define MAX5825_REG_CONFIG			0x50
#define MAX5825_REG_DEFAULT			0x60

#define MAX5825_REG_RETURNn			0x70
#define MAX5825_REG_CODEn			0x80
#define MAX5825_REG_LOADn			0x90
#define MAX5825_REG_CODEn_LOADall	0xA0
#define MAX5825_REG_CODEn_LOADn		0xB0
#define MAX5825_REG_CODEALL			0xC0
#define MAX5825_REG_LOADALL			0xC1
#define MAX5825_REG_CODELOADALL		0xC2
#define MAX5825_REG_RETURNALL		0xC3




void init_max5825(void);
uint8_t test_max5825(void);
void max5825_set_load_channel(uint8_t ch, uint16_t value);




#endif /* MAX5825_H_ */