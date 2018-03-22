#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include <stm32f0xx.h>
#include "delay.h"

void SystemClock_Config(void);

int timerTick, refreshTick;
uint32_t time = 0x00000000, sec = 0x00000000, min = 0x00000000, hour = 0x00000000, digit = 0x00000000, sec_min = 0x00000001;

/* LED pin connections:
  1-6 - PB7-PB3, PD2
  7-12 - PB15, PC6-PC9, PF6
  LED pin desctiption:
  A(11), B(7), C(4), D(2), E(1), F(10), G(5), DP(3), D1(12), D2(9), D3(8), D4(6)
 */

#define PINA		LL_GPIO_PIN_9
#define PORTA		GPIOC
#define AHBENA		LL_AHB1_GRP1_PERIPH_GPIOC 
#define PINB		LL_GPIO_PIN_15
#define PORTB		GPIOB
#define AHBENB		LL_AHB1_GRP1_PERIPH_GPIOB
#define PINC		LL_GPIO_PIN_2
#define PORTC		GPIOB
#define AHBENC		LL_AHB1_GRP1_PERIPH_GPIOB
#define PIND		LL_GPIO_PIN_6
#define PORTD		GPIOB
#define AHBEND		LL_AHB1_GRP1_PERIPH_GPIOB
#define PINE		LL_GPIO_PIN_7
#define PORTE		GPIOB
#define AHBENE		LL_AHB1_GRP1_PERIPH_GPIOB
#define PINF		LL_GPIO_PIN_8
#define PORTF		GPIOC
#define AHBENF		LL_AHB1_GRP1_PERIPH_GPIOC
#define PING		LL_GPIO_PIN_3
#define PORTG		GPIOB
#define AHBENG		LL_AHB1_GRP1_PERIPH_GPIOB
#define PINDP		LL_GPIO_PIN_5
#define PORTDP		GPIOB
#define AHBENDP		LL_AHB1_GRP1_PERIPH_GPIOB
#define PIND2		LL_GPIO_PIN_7
#define PORTD2		GPIOC
#define AHBEND2		LL_AHB1_GRP1_PERIPH_GPIOC
#define PIND3		LL_GPIO_PIN_6
#define PORTD3		GPIOC
#define AHBEND3		LL_AHB1_GRP1_PERIPH_GPIOC
#define PIND4		LL_GPIO_PIN_2
#define PORTD4		GPIOD
#define AHBEND4		LL_AHB1_GRP1_PERIPH_GPIOD

typedef struct {
  GPIO_TypeDef *PORT;
  uint32_t PIN;
} Pin;

void LED_GPIO_INIT(Pin pins_anode[], Pin pins_cathode[])
{
  uint8_t i=0;
  while (pins_anode[i].PIN != 0xFFFFFFFF)
  {
    LL_GPIO_WriteLow(pins_anode[i].PORT, pins_anode[i].PIN);
    i = i + 1;
  }
  i = 0;
  while (pins_cathode[i].PIN != 0xFFFFFFFF)
  {
    LL_GPIO_WriteHigh(pins_cathode[i].PORT, pins_cathode[i].PIN);
    i = i + 1;
  }
}

void LED_TEST(Pin pins_anode[], Pin pins_cathode[]) {
  uint8_t i = 0;
  while (pins_anode[i].PIN != 0xFFFFFFFF)
  {
    LL_GPIO_SetOutputPin(pins_anode[i].PORT, pins_anode[i].PIN);
    i = i + 1;
  }
  i = 0;
  while (pins_cathode[i].PIN != 0xFFFFFFFF)
  {
    LL_GPIO_ResetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
    i = i + 1;
  }
}

/*void writeValue(uint32_t value, uint32_t LED_ID) {
	uint32_t decoder[10][4] = {
	{A+B+C+D+E+F+Digit_1, // 0
	B+C+Digit_1, // 1
	A+B+G+E+D+Digit_1, // 2
	A+B+G+C+D+Digit_1, // 3
	F+G+B+C+Digit_1, // 4
	A+F+G+C+D+Digit_1, // 5
	A+F+E+D+C+G+Digit_1, // 6
	A+B+G+C+Digit_1, // 7
	A+B+C+D+E+F+G+Digit_1, // 8
	A+B+G+F+C+D+Digit_1}, // 9
	
	{A+B+C+D+E+F+Digit_2,
	B+C+Digit_2,
	A+B+G+E+D+Digit_2,
	A+B+G+C+D+Digit_2,
	F+G+B+C+Digit_2,
	A+F+G+C+D+Digit_2,
	A+F+E+D+C+G+Digit_2,
	A+B+G+C+Digit_2,
	A+B+C+D+E+F+G+Digit_2,
	A+B+G+F+C+D+Digit_2},

	{A+B+C+D+E+F+Digit_3,
	B+C+Digit_3,
	A+B+G+E+D+Digit_3,
	A+B+G+C+D+Digit_3,
	F+G+B+C+Digit_3,
	A+F+G+C+D+Digit_3,
	A+F+E+D+C+G+Digit_3,
	A+B+G+C+Digit_3,
	A+B+C+D+E+F+G+Digit_3,
	A+B+G+F+C+D+Digit_3},

	{A+B+C+D+E+F+Digit_4,
	B+C+Digit_4,
	A+B+G+E+D+Digit_4,
	A+B+G+C+D+Digit_4,
	F+G+B+C+Digit_4,
	A+F+G+C+D+Digit_4,
	A+F+E+D+C+G+Digit_4,
	A+B+G+C+Digit_4,
	A+B+C+D+E+F+G+Digit_4,
	A+B+G+F+C+D+Digit_4}

};


	LL_GPIO_WriteOutputPort(GPIOC, decoder[value][LED_ID]);
}

void updateTimer(void)
{
	sec = sec + 1;
	if ( sec == 60 ) {
		min = min + 1;
		sec = 0;
		if (min == 60) {
			hour = hour + 1;
			min = 0;
		}
	}
}

void refreshDigit(uint32_t number)
{
	if ( sec_min == 1 ) {
		switch (number) {
			case 0: writeValue(sec%10,0);
			break;
			case 1: writeValue(sec/10,1);
			break;
			case 2: writeValue(min%10,2);
			break;
			case 3: writeValue(min%10,3);
		}
	} else {
		switch (number) {
			case 0: writeValue(min%10,0);
			break;
			case 1: writeValue(min/10,1);
			break;
			case 2: writeValue(hour%10,2);
			break;
			case 3: writeValue(hour%10,3);
		}
	}
}
*/

void
SysTick_Handler(void) 
{
  TimingDelay_Decrement();
//	timerTick++;
//	refreshTick++;
        if (timerTick == 1000) {
        	//updateTimer();
		timerTick = 0;
	}
	if (refreshTick == 10) {
		//refreshDigit(digit);
		digit = (digit + 1)%4;
		refreshTick = 0;
	}
}


int main(void) {
  Pin pins_anode[9] =
  {
  {PORTA, PINA},
  {PORTB, PINB},
  {PORTC, PINC},
  {PORTD, PIND},
  {PORTE, PINE},
  {PORTF, PINF},
  {PORTG, PING},
  {PORTDP, PINDP},
  {0x0,0xFFFFFFFF}
  };
  Pin pins_cathode[5] =
  {
  {PORTD1, PIND1},
  {PORTD2, PIND2},
  {PORTD3, PIND3},
  {PORTD4, PIND4},
  {0x0,0xFFFFFFFF}
  }; 
  SystemClock_Config();
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  LED_GPIO_INIT(pins_anode, pins_cathode);
  LED_TEST(pins_anode, pins_cathode);
  while (1);
}

/**
  * System Clock Configuration
  * The system Clock is configured as follow :
  *    System Clock source            = PLL (HSI/2)
  *    SYSCLK(Hz)                     = 48000000
  *    HCLK(Hz)                       = 48000000
  *    AHB Prescaler                  = 1
  *    APB1 Prescaler                 = 1
  *    HSI Frequency(Hz)              = 8000000
  *    PLLMUL                         = 12
  *    Flash Latency(WS)              = 1
  */

void
SystemClock_Config() {
        /* Set FLASH latency */
        LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

        /* Enable HSI and wait for activation*/
        LL_RCC_HSI_Enable();
        while (LL_RCC_HSI_IsReady() != 1);

        /* Main PLL configuration and activation */
        LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2,
                                    LL_RCC_PLL_MUL_12);

        LL_RCC_PLL_Enable();
        while (LL_RCC_PLL_IsReady() != 1);

        /* Sysclk activation on the main PLL */
        LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
        LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
        while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

        /* Set APB1 prescaler */
        LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

        /* Set systick to 1ms */
	/*RCC_ClocksTypeDef RCC_Clocks;
   	RCC_GetClocksFreq (&RCC_Clocks);*/
   	(void) SysTick_Config (48000000/1000);
	/*RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq (&RCC_Clocks);
        SysTick_Config(RCC_Clocks.HCLK_Frequency);*/

        /* Update CMSIS variable (which can be updated also
         * through SystemCoreClockUpdate function) */
        SystemCoreClock = 168000000;
}

void
NMI_Handler(void) {

}

void
HardFault_Handler(void) {	 
        while (1);
}

void
SVC_Handler(void) {

}

void
PendSV_Handler(void) {

}

