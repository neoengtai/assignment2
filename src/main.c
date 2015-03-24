#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_ssp.h"
#include "main.h"
#include "led7seg.h"
#include "oled.h"
//Test commit by weikang

#include "stdio.h"

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
	// 5 rows of data, each row max of OLED_DISPLAY_WIDTH/6 (char width is 6 px in this case)
	char data[5][OLED_DISPLAY_WIDTH / OLED_CHAR_WIDTH] = { "L :R", "T :R",
			"AX:R", "AY:R", "AZ:R" };

	switch (currentMode) {
	case BASIC:
		sprintf(&data[0][3], "%lu", lightVal);
		sprintf(&data[1][3], "%lu", tempVal);
		sprintf(&data[2][3], "%lu", accVal_X);
		sprintf(&data[3][3], "%lu", accVal_Y);
		sprintf(&data[4][3], "%lu", accVal_Z);
		break;
	case RESTRICTED:
		//Do nothing, since R is the initialized value
		break;
	}

	int i;
	for (i = 0; i < 5; i++) {
		oled_putString(0, i * OLED_CHAR_HEIGHT, (uint8_t *) data[i],
				OLED_COLOR_WHITE, OLED_COLOR_BLACK);
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

	oled_init();
	led7seg_init();

	oled_clearScreen(OLED_COLOR_BLACK);

	while (1) {
		if (led7SegFlag) {
			led7SegTask();
			led7SegFlag = 0;
		}

		if (sampleFlag) {
			accelerometerTask();
			lightSensorTask();
			oledTask();
			sampleFlag = 0;
		}
	}
}
