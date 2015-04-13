#include "LPC17xx.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_uart.h"
#include "main.h"
#include "led7seg.h"
#include "oled.h"
#include "light.h"
#include "acc.h"
#include "temp.h"
#include "pca9532.h"
#include "rotary.h"
#include "joystick.h"

#include "stdio.h"

typedef enum {
	BASIC, RESTRICTED
} OPERATING_MODE;

typedef enum {
	PERIOD, FLARE
} SETTINGS_MODE;

char *MSG_FLARE =
		"Solar Flare Detected. Scheduled Telemetry is Temporarily Suspended.\r\n";
char *MSG_NORMAL =
		"Space Weather is Back to Normal. Scheduled Telemetry Will Now Resume.\r\n";

OPERATING_MODE currentMode;
SETTINGS_MODE settingsInFocus;

static const int adjustmentVal = 100;
static const int samplingTimeUpperBound = 5000;
static const int samplingTimeLowerBound = 2000;
static const int flareLowerBound = 800;
static const int flareUpperBound = START_T_LIGHT_RANGE;

volatile uint32_t SAMPLING_TIME = 3000; // Initial value of 3000
volatile uint32_t FLARE_INTENSITY = 2000; // Initial value of 2000

volatile uint32_t msTicks = 0;
volatile uint32_t lightVal = 0;
volatile int8_t accVal_X = 0;
volatile int8_t accVal_Y = 0;
volatile int8_t accVal_Z = 0;
volatile int32_t tempVal = 0;
volatile uint32_t flare_flag = 0;
volatile uint32_t sw3_flag = 0;

void SysTick_Handler(void) {
	msTicks++;
}
void EINT3_IRQHandler(void) {
// Determine whether GPIO Interrupt P2.5 has occurred
	if ((LPC_GPIOINT ->IO2IntStatF >> 5) & 0x1) {
		flare_flag = 1;
		light_clearIrqStatus();
		LPC_GPIOINT ->IO2IntClr = 1 << 5;
	}
}

void EINT0_IRQHandler(void) {
	sw3_flag = 1;
	LPC_SC ->EXTINT |= 1; //Reset EINT0
}

/***************	Misc functions	********************/
void switchMode(void) {
	if (currentMode == BASIC) {
		currentMode = RESTRICTED;
		INDICATOR_BASIC_OFF();
		INDICATOR_RESTRICTED_ON();
		INDICATOR_SAFE_OFF();
		oledUpdate(0);
		UART_SendString(LPC_UART3, (uint8_t *) MSG_FLARE);
	} else {
		currentMode = BASIC;
		INDICATOR_BASIC_ON();
		INDICATOR_RESTRICTED_OFF();
		INDICATOR_SAFE_ON();
		UART_SendString(LPC_UART3, (uint8_t *) MSG_NORMAL);
	}
}

void transmitData(void) {
	char buf[32] = "";
	sprintf(buf, "L%lu_T%.1f_AX%d_AY%d_AZ%d\r\n", lightVal,
			((float) tempVal / 10.0), accVal_X, accVal_Y, accVal_Z);
	UART_SendString(LPC_UART3, (uint8_t *) buf);
}

uint32_t getMsTicks(void) {
	return msTicks;
}

/***************	Peripheral tasks	******************/
void led7SegUpdate(void) {
	static int led7SegTimer = 0;
	int num;

	if (msTicks >= led7SegTimer + 1000) {
		led7SegTimer = msTicks;
		num = (msTicks / 1000) % 10;
		char numChar = (char) ((int) '0' + num); //convert int to char
		led7seg_setChar(numChar, 0);
	}
}

void sample_accelerometer(void) {
	acc_read(&accVal_X, &accVal_Y, &accVal_Z);
}

void sample_light(void) {
	lightVal = light_read();
}

void sample_temp(void) {
	tempVal = temp_read();
}

/******************************************************************************
 *
 * Description:
 *    Update OLED with either sampled values or 'R'
 *
 * Params:
 *   [in] displayMode - 1 prints sampled values
 *   					0 prints 'R'
 *
 *****************************************************************************/
void oledUpdate(int displayMode) {
	// 5 rows of data
	char data[5][OLED_DISPLAY_WIDTH / OLED_CHAR_WIDTH] = { "", "", "", "", "" };

	if (displayMode) {
		sprintf(&data[0][0], "L : %-4lu", lightVal);
		sprintf(&data[1][0], "T : %-4.1f", (float) tempVal / 10.0);
		sprintf(&data[2][0], "AX: %-4d", accVal_X);
		sprintf(&data[3][0], "AY: %-4d", accVal_Y);
		sprintf(&data[4][0], "AZ: %-4d", accVal_Z);
	} else {
		sprintf(&data[0][0], "L : %-4s", "R");
		sprintf(&data[1][0], "T : %-4s", "R");
		sprintf(&data[2][0], "AX: %-4s", "R");
		sprintf(&data[3][0], "AY: %-4s", "R");
		sprintf(&data[4][0], "AZ: %-4s", "R");
	}

	int i;
	for (i = 0; i < 5; i++) {
		oled_putString(0, i * OLED_CHAR_HEIGHT, (uint8_t *) data[i],
				OLED_COLOR_WHITE, OLED_COLOR_BLACK);
	}

}

void rotary_change(void) {
	uint8_t rotaryStatus = rotary_read();

	if (rotaryStatus == ROTARY_RIGHT) {
		switch (settingsInFocus) {
		case PERIOD:
			if (SAMPLING_TIME > samplingTimeLowerBound) {
				// Faster
				SAMPLING_TIME -= adjustmentVal;
				// Display current SAMPLING_TIME value on OLED
				settingsPaneUpdate();
			}
			break;
		case FLARE:
			if (FLARE_INTENSITY > flareLowerBound) {
				FLARE_INTENSITY -= adjustmentVal;
				settingsPaneUpdate();
				light_setHiThreshold(FLARE_INTENSITY);
			}
			break;
		}
	}

	if (rotaryStatus == ROTARY_LEFT) {
		switch (settingsInFocus) {
		case PERIOD:
			if (SAMPLING_TIME < samplingTimeUpperBound) {
				// Slower
				SAMPLING_TIME += adjustmentVal;
				// Display current SAMPLING_TIME value on OLED
				settingsPaneUpdate();
			}
			break;
		case FLARE:
			if (FLARE_INTENSITY < flareUpperBound) {
				FLARE_INTENSITY += adjustmentVal;
				settingsPaneUpdate();
				light_setHiThreshold(FLARE_INTENSITY);
			}
			break;
		}
	}
}

void settingsPaneUpdate(void) {
	int row = 5; //row 0-4 is used for sampled values.
	char buf[OLED_DISPLAY_WIDTH / OLED_CHAR_WIDTH] = "";
	oled_color_t periodFB, periodBG, flareFB, flareBG;

	if (settingsInFocus == PERIOD) {
		periodFB = OLED_COLOR_BLACK;
		periodBG = OLED_COLOR_WHITE;
		flareFB = OLED_COLOR_WHITE;
		flareBG = OLED_COLOR_BLACK;
	} else {
		periodFB = OLED_COLOR_WHITE;
		periodBG = OLED_COLOR_BLACK;
		flareFB = OLED_COLOR_BLACK;
		flareBG = OLED_COLOR_WHITE;
	}

	//cnt draw a line :(
//	oled_line(0,row*OLED_CHAR_HEIGHT,OLED_DISPLAY_WIDTH, row*OLED_CHAR_HEIGHT, OLED_COLOR_WHITE);
//	row++;

	sprintf(buf, "PERIOD: %-4lu", SAMPLING_TIME);
	oled_putString(0, row * OLED_CHAR_HEIGHT, (uint8_t *) buf, periodFB,
			periodBG);
	row++;
	sprintf(buf, "FLARE: %-4lu", FLARE_INTENSITY);
	oled_putString(0, row * OLED_CHAR_HEIGHT, (uint8_t *) buf, flareFB,
			flareBG);
}

void joystick_change(void) {
	uint8_t state = joystick_read();

	if (state == JOYSTICK_UP) {
		if (settingsInFocus == FLARE) {
			settingsInFocus = PERIOD;
			settingsPaneUpdate();
		}
	} else if (state == JOYSTICK_DOWN) {
		if (settingsInFocus == PERIOD) {
			settingsInFocus = FLARE;
			settingsPaneUpdate();
		}
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

static void init_i2c(void) {
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

	// For GPIO interrupt (light sensor)
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 5;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, 1 << 5, 0);

	LPC_GPIOINT ->IO2IntEnF |= 1 << 5;
}

void init_UART(void) {
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);

	UART_CFG_Type uartCfg;
	uartCfg.Baud_rate = 115200;
	uartCfg.Databits = UART_DATABIT_8;
	uartCfg.Parity = UART_PARITY_NONE;
	uartCfg.Stopbits = UART_STOPBIT_1;

	//supply power & setup working par.s for uart3
	UART_Init(LPC_UART3, &uartCfg);
	//enable transmit for uart3
	UART_TxCmd(LPC_UART3, ENABLE);
}

void init_EINT0(void) {
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&PinCfg);

	LPC_SC ->EXTMODE |= 1; //Set EINT0 to edge sensitive
	LPC_SC ->EXTPOLAR &= ~1; //Set EINT0 to falling edge sensitive
	LPC_SC ->EXTINT |= 1; //Reset EINT0
}

void rgb_init(void) {
	GPIO_SetDir(2, 1, 1);
	GPIO_SetDir(0, (1 << 26), 1);
}

void STAR_T_init(void) {
	//Determine starting mode
	if (light_read() <= FLARE_INTENSITY) {
		currentMode = BASIC;
	} else {
		currentMode = RESTRICTED;
	}

	settingsInFocus = PERIOD;

	led7seg_setChar('0', 0);
	oled_clearScreen(OLED_COLOR_BLACK);
	settingsPaneUpdate();

	switch (currentMode) {
	case BASIC:
		//Default is BASIC mode
	default:
		INDICATOR_BASIC_ON();
		INDICATOR_RESTRICTED_OFF();

		INDICATOR_SAFE_ON();
		break;
	case RESTRICTED:
		INDICATOR_BASIC_OFF();
		INDICATOR_RESTRICTED_ON();

		INDICATOR_SAFE_OFF();

		oledUpdate(0);
		break;
	}

}

//Routines
void routine_BASIC(void) {
	static int sampleTimer = 0;

	if (flare_flag) {
		switchMode();
		return;
	}

	if (msTicks >= sampleTimer + SAMPLING_TIME) {
		sampleTimer = msTicks;
		sample_accelerometer();
		sample_light();
		sample_temp();
		oledUpdate(1);
		transmitData();
	}
}

void routine_RESTRICTED(void) {
	static uint32_t startTime, count;

	if (flare_flag) {
		startTime = msTicks;
		count = 0;
		INDICATOR_SAFE_OFF();
		flare_flag = 0;
	}
	if ((msTicks >= startTime + TIME_UNIT * (count + 1))) {
		count = (msTicks - startTime) / TIME_UNIT;
		pca9532_setLeds(((1 << count) - 1), 0);
		if (count >= 16) {
			switchMode();
		}
	}
}

void routine_SW3(void) {
	sample_accelerometer();
	sample_light();
	sample_temp();
	oledUpdate(1);
	transmitData();
}

int main(void) {
	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1)
			;
	}

	init_spi();
	init_i2c();
	init_GPIO();
	init_UART();
	init_EINT0();

	oled_init();
	led7seg_init();
	rotary_init();
	joystick_init();
	acc_init();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	light_setLoThreshold(0);
	light_setHiThreshold(FLARE_INTENSITY);
	light_clearIrqStatus();
	rgb_init();
	temp_init(getMsTicks);

	NVIC_EnableIRQ(EINT3_IRQn);
	NVIC_EnableIRQ(EINT0_IRQn);

	STAR_T_init();

	while (1) {
		led7SegUpdate();
		rotary_change();
		joystick_change();

		switch (currentMode) {
		case BASIC:
			routine_BASIC();
			break;
		case RESTRICTED:
			routine_RESTRICTED();
			break;
		}
		if (sw3_flag) {
			routine_SW3();
			sw3_flag = 0;
		}
	}
}
