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

#include "stdio.h"

typedef enum {
	BASIC, RESTRICTED
} OPERATING_MODE;

char *MSG_FLARE =
		"Solar Flare Detected. Scheduled Telemetry is Temporarily Suspended.\r\n";
char *MSG_NORMAL =
		"Space Weather is Back to Normal. Scheduled Telemetry Will Now Resume.\r\n";

OPERATING_MODE currentMode;

volatile uint32_t msTicks = 0;
volatile uint32_t lightVal = 0;
volatile int8_t accVal_X = 0;
volatile int8_t accVal_Y = 0;
volatile int8_t accVal_Z = 0;
volatile int32_t tempVal = 0;
volatile uint32_t flare_flag = 0;

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
	if ((LPC_GPIOINT ->IO2IntStatF >> 10) & 0x1) {
		sample_accelerometer();
		sample_light();
		sample_temp();
		oledUpdate();
		transmitData();
		LPC_GPIOINT ->IO2IntClr = 1 << 10;
	}
}

/***************	Misc functions	********************/
void switchMode(void) {
	if (currentMode == BASIC) {
		currentMode = RESTRICTED;
		INDICATOR_BASIC_OFF();
		INDICATOR_RESTRICTED_ON();
		INDICATOR_SAFE_OFF();
		oledUpdate();
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

void oledUpdate(void) {
	// 5 rows of data, each row max of OLED_DISPLAY_WIDTH/6 (char width is 6 px in this case)
	char data[5][OLED_DISPLAY_WIDTH / OLED_CHAR_WIDTH] = { "L :R", "T :R",
			"AX:R", "AY:R", "AZ:R" };

	switch (currentMode) {
	case BASIC:
		sprintf(&data[0][3], "%lu", lightVal);
		sprintf(&data[1][3], "%.1f", (float) tempVal / 10.0);
		sprintf(&data[2][3], "%d", accVal_X);
		sprintf(&data[3][3], "%d", accVal_Y);
		sprintf(&data[4][3], "%d", accVal_Z);
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

void led16Task(void) {

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

	// For GPIO interrupt (light sensor)
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 5;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, 1 << 5, 0);

	// For GPIO interrupt (SW3)
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, 1 << 10, 0);

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

	led7seg_setChar('0', 0);
	oled_clearScreen(OLED_COLOR_BLACK);

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

		oledUpdate();
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
		oledUpdate();
		transmitData();
	}
}

void routine_RESTRICTED(void) {
	static int led16Timer;
	static uint16_t led16state;

	if (flare_flag) {
		led16Timer = msTicks;
		led16state = 0;
		INDICATOR_SAFE_OFF();
		flare_flag = 0;
	}
	if ((msTicks >= led16Timer + TIME_UNIT)) {
		led16Timer = msTicks;
		led16state = (led16state << 1) | 1;
		pca9532_setLeds(led16state, 0);
		if (led16state == ((uint16_t) -1)) {
			switchMode();
		}
	}
}

int main(void) {
	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1)
			;
	}

	init_spi();
	i2c_init();
	init_GPIO();
	init_UART();

	oled_init();
	led7seg_init();
	acc_init();
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	light_setLoThreshold(0);
	light_setHiThreshold(FLARE_INTENSITY);
	light_clearIrqStatus();
	rgb_init();
	temp_init(getMsTicks);

	LPC_GPIOINT ->IO2IntEnF |= 1 << 5;
	LPC_GPIOINT ->IO2IntEnF |= 1 << 10;
	NVIC_EnableIRQ(EINT3_IRQn);

	STAR_T_init();

	while (1) {
		led7SegUpdate();

		switch (currentMode) {
		case BASIC:
			routine_BASIC();
			break;
		case RESTRICTED:
			routine_RESTRICTED();
			break;
		}

	}
}
