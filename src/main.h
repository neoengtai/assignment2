#define SAMPLING_TIME		3000	// milliseconds
#define FLARE_INTENSITY		2000	// lux
#define TIME_UNIT			250		// milliseconds

#define INDICATOR_BASIC_ON() GPIO_SetValue(0, 1<<26)	//Blue led
#define INDICATOR_BASIC_OFF() GPIO_ClearValue(0, 1<<26)

#define INDICATOR_RESTRICTED_ON() GPIO_SetValue(2, 1)		//Red led
#define INDICATOR_RESTRICTED_OFF() GPIO_ClearValue(2, 1)

#define INDICATOR_SAFE_ON() //TODO: led lights
#define INDICATOR_SAFE_OFF()

#define OLED_CHAR_WIDTH 6
#define OLED_CHAR_HEIGHT 8
//#define BASIC_MODE()			\
//	INDICATOR_BASIC_ON();		\
//	INDICATOR_RESTRICTED_OFF();	\
//	INDICATOR_SAFE_ON()
//
//#define RESTRICTED_MODE()		\
//	INDICATOR_BASIC_OFF();		\
//	INDICATOR_RESTRICTED_ON();	\
//	INDICATOR_SAFE_OFF()

void accelerometerTask(void);
uint32_t getMsTicks(void);
void led7SegUpdate(void);
void sample_light(void);
void oledUpdate(void);
void switchMode(void);
void sample_temp(void);
void transmit(char* str);


