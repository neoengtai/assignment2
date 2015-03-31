#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_ssp.h"
#include "main.h"
#include "led7seg.h"
#include "oled.h"
#include "light.h"
#include "acc.h"
#include "temp.h"
#include "rgb.h"

#include "stdio.h"

typedef enum {
	BASIC, RESTRICTED
} OPERATING_MODE;

OPERATING_MODE currentMode;

volatile uint32_t msTicks = 0;
volatile uint32_t lightVal = 0;
volatile uint8_t accVal_X = 0;
volatile uint8_t accVal_Y = 0;
volatile uint8_t accVal_Z = 0;
volatile uint32_t tempVal = 0;

void SysTick_Handler(void) {
	msTicks++;
}

/***************	Misc functions	********************/
void switchMode(void) {
	if (currentMode == BASIC) {
		currentMode = RESTRICTED;

		INDICATOR_BASIC_OFF();
		INDICATOR_RESTRICTED_ON();

		oledTask();

	} else {
		currentMode = BASIC;
		INDICATOR_BASIC_ON();
		INDICATOR_RESTRICTED_OFF();
	}

}

void transmit(char* str) {

}

uint32_t getMsTicks(void) {
	return msTicks;
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
		acc_read(&accVal_X, &accVal_Y, &accVal_Z);

		break;
	case RESTRICTED:

		break;
	}
}

void lightSensorTask(void) {
	lightVal = light_read();
	switch (currentMode) {
	case BASIC:
		if (lightVal > FLARE_INTENSITY) {
			switchMode();
		}

		break;
	case RESTRICTED:
		if (lightVal <= FLARE_INTENSITY) {
			switchMode();
		}

		break;
	}
}

void tempSensorTask(void) {
	switch (currentMode) {
	case BASIC:
		tempVal = temp_read();

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
		sprintf(&data[1][3], "%.1f", (float) tempVal / 10.0);
		sprintf(&data[2][3], "%u", accVal_X);
		sprintf(&data[3][3], "%u", accVal_Y);
		sprintf(&data[4][3], "%u", accVal_Z);
		break;
	case RESTRICTED:
		//Do nothing, since R is the initialized value
		break;
	}
	oled_clearScreen(OLED_COLOR_BLACK);
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

static void i2c_init(void) {
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

static void init_GPIO(void) {

	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, 1 << 2, 0);

}

int main(void) {
	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1)
			;
	}

	int led7segTimer = 0;
	int sampleTimer = 0;

	init_spi();
	i2c_init();
	init_GPIO();

	oled_init();
	led7seg_init();
	acc_init();
	light_init();
	rgb_init();
	temp_init(getMsTicks);
	light_enable();
	light_setRange(LIGHT_RANGE_4000);

	oled_clearScreen(OLED_COLOR_BLACK);

	currentMode = BASIC; //Start in basic mode

	INDICATOR_BASIC_ON();
	INDICATOR_RESTRICTED_OFF();

	while (1) {
		if (msTicks >= led7segTimer+1000) {
			led7segTimer = msTicks;
			led7SegTask();
		}

		if (msTicks >= sampleTimer + SAMPLING_TIME) {
			sampleTimer = msTicks;
			accelerometerTask();
			lightSensorTask();
			tempSensorTask();
			oledTask();
		}
	}
}
