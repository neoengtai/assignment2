#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_ssp.h"
#include "main.h"
#include "led7seg.h"

typedef enum {
	BASIC, RESTRICTED
} OPERATING_MODE;

volatile uint32_t msTicks = 0;
volatile uint8_t led7SegFlag = 0;
volatile uint8_t sampleFlag = 0;
volatile uint32_t lightVal = 0;
volatile uint32_t accVal_X = 0;
volatile uint32_t accVal_Y = 0;
volatile uint32_t accVal_Z = 0;
volatile uint32_t tempVal = 0;

OPERATING_MODE currentMode = BASIC; //Start in basic mode

void SysTick_Handler(void) {
	msTicks++;

	// Set led7Flag every second
	if ((msTicks % 1000) == 0) {
		led7SegFlag = 1;
	}
	// Set sample flag every SAMPLING_TIME
	if ((msTicks % SAMPLING_TIME) == 0) {
		sampleFlag = 1;
	}
}

/***************	Misc functions	********************/
void switchMode(void) {

}

void transmit(char* str) {

}

/***************	Peripheral tasks	******************/
void led7SegTask(void) {
	int num = (msTicks / 1000) % 10;
	char numChar = (char) ((int) '0' + num); //convert int to char
	led7seg_setChar(numChar, 0);
}

void accelerometerTask(void) {
	switch (currentMode) {
	case BASIC:

		break;
	case RESTRICTED:

		break;
	}
}

void lightSensorTask(void) {
	switch (currentMode) {
	case BASIC:

		break;
	case RESTRICTED:

		break;
	}
}

void tempSensorTask(void) {
	switch (currentMode) {
	case BASIC:

		break;
	case RESTRICTED:

		break;
	}
}

void oledTask(void) {
	switch (currentMode) {
	case BASIC:

		break;
	case RESTRICTED:

		break;
	}
}

/***************	Init functions	********************/
void init_spi(void) {
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

int main(void) {
	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1)
			;
	}

	init_spi();
	led7seg_init();

	while (1) {
		if (led7SegFlag) {
			led7SegTask();
			led7SegFlag = 0;
		}

		if (sampleFlag) {
			accelerometerTask();
			lightSensorTask();
			sampleFlag = 0;
		}
	}
}
