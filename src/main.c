#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_ssp.h"

#include "led7seg.h"

typedef enum {BASIC, RESTRICTED} OPERATING_MODE;

volatile uint32_t msTicks = 0;
volatile uint8_t led7SegFlag = 0;

void SysTick_Handler(void){
	static int count = 0;

	msTicks++;
	count++;

	// Set led7Flag every second
	if (count > 999){
		led7SegFlag = 1;
		count = 0;
	}
}

void updateLed7Seg(void){
	int num = (msTicks/1000) % 10;
	char numChar = (char)((int)'0' + num); //convert int to char
	led7seg_setChar(numChar, 0);
}

void init_spi(void){
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);
}

int main (void){
	OPERATING_MODE currentMode = BASIC;


	if (SysTick_Config (SystemCoreClock / 1000)) {
	    while (1);
	}

	init_spi();
	led7seg_init();


	while (1){
		// 7 segment counting in all modes
		if (led7SegFlag){
			updateLed7Seg();
			led7SegFlag = 0;
		}

	}
}
