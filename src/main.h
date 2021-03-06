#define START_T_LIGHT_RANGE 4000	//lux
#define TIME_UNIT			250		// milliseconds
#define INDICATOR_BASIC_ON() GPIO_SetValue(0, 1<<26)	//Blue led
#define INDICATOR_BASIC_OFF() GPIO_ClearValue(0, 1<<26)

#define INDICATOR_RESTRICTED_ON() GPIO_SetValue(2, 1)		//Red led
#define INDICATOR_RESTRICTED_OFF() GPIO_ClearValue(2, 1)

#define INDICATOR_SAFE_ON() pca9532_setLeds(-1, 0)
#define INDICATOR_SAFE_OFF() pca9532_setLeds(0, -1)

#define OLED_CHAR_WIDTH 6
#define OLED_CHAR_HEIGHT 8

void sample_accelerometer(void);
uint32_t getMsTicks(void);
void led7SegUpdate(void);
void sample_light(void);
void oledUpdate(int);
void switchMode(void);
void sample_temp(void);
void transmitData(void);
void settingsPaneUpdate(void);
void rotary_change(void);
void joystick_change(void);

