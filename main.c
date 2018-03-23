#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include <stm32f0xx.h>
#include "delay.h"

void SystemClock_Config(void);

uint32_t timeCounter1, timeCounter2;
uint32_t ms, sec, min, hour, day, digit, mode;
uint32_t anode_size, cathode_size;
uint32_t LED_FPS, LED_ClockCycle, LED_RefreshCycle; /* in seconds */
uint8_t pointPos;

/* LED pin connections:
  1-6 - PB7-PB3, PD2
  7-12 - PB15, PC6-PC9, PF6
  LED pin desctiption:
  A(11), B(7), C(4), D(2), E(1), F(10), G(5), DP(3), D1(12), D2(9), D3(8), D4(6)
 */
#define LED_MODE_ms	0x00
#define LED_MODE_MS	0x01
#define LED_MODE_HM	0x02

#define PINA		LL_GPIO_PIN_9
#define PORTA		GPIOC
#define AHBENA		LL_AHB1_GRP1_PERIPH_GPIOC 
#define PINB		LL_GPIO_PIN_15
#define PORTB		GPIOB
#define AHBENB		LL_AHB1_GRP1_PERIPH_GPIOB
#define PINC		LL_GPIO_PIN_4
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
#define PIND1		LL_GPIO_PIN_6
#define PORTD1		GPIOF
#define AHBEND1		LL_AHB1_GRP1_PERIPH_GPIOF
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
  uint32_t AHBEN;
} Pin;

typedef struct {
  Pin *pins[9];
  uint8_t mask;
} Digit;

#define A	{PORTA, PINA, AHBENA}
#define B	{PORTB, PINB, AHBENB}
#define C	{PORTC, PINC, AHBENC}
#define D	{PORTD, PIND, AHBEND}
#define E	{PORTE, PINE, AHBENE}
#define F	{PORTF, PINF, AHBENF}
#define G	{PORTG, PING, AHBENG}
#define DP	{PORTDP, PINDP, AHBENDP}
#define END	{0x0,0x0, 0x4}
#define D1	{PORTD1, PIND1, AHBEND1}
#define D2	{PORTD2, PIND2, AHBEND2}
#define D3	{PORTD3, PIND3, AHBEND3}
#define D4	{PORTD4, PIND4, AHBEND4}

Digit LED_Digits[10] =
{
  /*0*/
  { {A, B, C, D, E, F, END}, 0x3F },  
  /*1*/
  { {B, C, END}, 0x06 },
  /*2*/
  { {A, B, G, E, D, END}, 0x5B }, 
  /*3*/
  { {A, B, G, C, D, END}, 0x4F }, 
  /*4*/
  { {F, G, B, C, END}, 0x66 }, 
  /*5*/
  { {A, F, G, C, D, END}, 0x6D },
  /*6*/
  { {A, F, E, D, C, G, END}, 0x7D }, 
  /*7*/
  { {A, B, C, END}, 0x07 },
  /*8*/
  { {A, B, C, D, E, F, G, END}, 0x7F }, 
  /*9*/
  { {A, B, G, F, C, D, END}, 0x6F }
}; 
Pin pins_anode[9] = {A, B, C, D, E, F, G, DP, END};
Pin pins_cathode[5] = {D1, D2, D3, D4, END};

void LED_GPIO_INIT()
{
  uint8_t i=0;
  while (pins_anode[i].AHBEN != 0x4)
  {
    if ( ~LL_AHB1_GRP1_IsEnabledClock(pins_anode[i].AHBEN) )
    {
      LL_AHB1_GRP1_EnableClock(pins_anode[i].AHBEN);
    }
    LL_GPIO_WriteLow(pins_anode[i].PORT, pins_anode[i].PIN);
    i = i + 1;
  }
  anode_size = i;
  i = 0;
  while (pins_cathode[i].AHBEN != 0x4)
  {
    if ( ~LL_AHB1_GRP1_IsEnabledClock(pins_cathode[i].AHBEN) )
    {
      LL_AHB1_GRP1_EnableClock(pins_cathode[i].AHBEN);
    }
    LL_GPIO_WriteHigh(pins_cathode[i].PORT, pins_cathode[i].PIN);
    i = i + 1;
  }
  cathode_size = i;
}

void LED_STATIC_TEST()
{
  uint8_t i = 0;
  while (i < anode_size)
  {
    LL_GPIO_SetOutputPin(pins_anode[i].PORT, pins_anode[i].PIN);
    i = i + 1;
  }
  i = 0;
  while (i < cathode_size)
  {
    LL_GPIO_ResetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
    i = i + 1;
  }
}

void LED_DYNAMIC_TEST()
{
  uint8_t i = 0, j = 0;
  while (1)
  {
    while (i < cathode_size)
    {
      LL_GPIO_ResetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      j = 0;
      while (j < anode_size)
      {
        LL_GPIO_SetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        Delay(1);
        LL_GPIO_ResetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        j = j + 1;
      }
      LL_GPIO_SetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      i = i + 1;
    }
    i = cathode_size - 2;
    while (i > 0)
    {
      LL_GPIO_ResetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      j = anode_size - 1;
      while (j != 0)
      {
        LL_GPIO_SetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        Delay(1);
        LL_GPIO_ResetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        j = j - 1;
      }
      LL_GPIO_SetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      i = i - 1;
    } 
  }
}

void LED_TIME_INIT(uint8_t fps_, uint32_t clock_cycle_,  uint64_t current_time_,uint8_t mode_)
{
  mode = mode_;
  LED_FPS = fps_;
  LED_RefreshCycle = 1000/ (LED_FPS*4);
  LED_ClockCycle = clock_cycle_;
  ms = current_time_%1000;
  sec = current_time_%100000/1000;
  min = current_time_/100000%100;
  hour = current_time_/10000000;
}

void writeValue(uint8_t value)
{
  uint8_t i = 0;
  while (i < 8)
  {
    if ( LED_Digits[value].mask & (1<<i) )
    {
      LL_GPIO_SetOutputPin(pins_anode[i].PORT, pins_anode[i].PIN);
    } else {   
      LL_GPIO_ResetOutputPin(pins_anode[i].PORT, pins_anode[i].PIN);
    }
    i = i + 1;
  } 
}

void refreshLED(uint8_t digit)
{
  uint32_t value;
  if ( mode == LED_MODE_ms )
  { value = ms + sec%10*1000; }
  else if (mode == LED_MODE_MS)
  { value = sec + min*100; }
  else if (mode == LED_MODE_HM)
  { value = min + hour*100; }
  switch (digit)
  {
    case 0:
      LL_GPIO_SetOutputPin(((Pin)D1).PORT, ((Pin)D1).PIN);
      writeValue(value%10);
      LL_GPIO_ResetOutputPin(((Pin)D4).PORT, ((Pin)D4).PIN);
      break;
    case 1:
      LL_GPIO_SetOutputPin(((Pin)D4).PORT, ((Pin)D4).PIN);
      writeValue(value%100/10);
      LL_GPIO_ResetOutputPin(((Pin)D3).PORT, ((Pin)D3).PIN);
      break;
    case 2:
      LL_GPIO_SetOutputPin(((Pin)D3).PORT, ((Pin)D3).PIN);
      writeValue(value/100%10);
      if (mode == LED_MODE_MS || mode == LED_MODE_HM)
      {
        LL_GPIO_SetOutputPin(((Pin)DP).PORT, ((Pin)DP).PIN);
      }
      LL_GPIO_ResetOutputPin(((Pin)D2).PORT, ((Pin)D2).PIN);
      break;
    case 3:
      LL_GPIO_SetOutputPin(((Pin)D2).PORT, ((Pin)D2).PIN);
      writeValue(value/1000);
      if (mode == LED_MODE_ms)
      {
        LL_GPIO_SetOutputPin(((Pin)DP).PORT, ((Pin)DP).PIN);
      }
      LL_GPIO_ResetOutputPin(((Pin)D1).PORT, ((Pin)D1).PIN);
  }
}

void updateTime()
{
  ms = ms + LED_ClockCycle;
  if ( ms > 999 )
  {
    sec = sec + ms / 1000;
    ms = ms % 1000;
    if ( sec > 59)
    {
      min = min + sec / 60;
      sec = sec % 60;
      if ( min > 59 )
      {
        hour = hour + min / 60;
        min = min % 60;
        if (hour > 23)
        {
          day = day + hour/24;
          hour = hour % 24;
        }
      }
    }
  }
}

void
SysTick_Handler(void) 
{
  //TimingDelay_Decrement();
  timeCounter1 = timeCounter1 + 1;
  timeCounter2 = timeCounter2 + 1;
  if ( timeCounter1 == LED_ClockCycle )
  {
    timeCounter1 = 0;
    updateTime();
  }
  if (timeCounter2 == LED_RefreshCycle)
  {
    timeCounter2 = 0;
    refreshLED(digit);
    digit = (digit + 1) %4;
  }
} 

int main(void)
{
  SystemClock_Config();
  LED_GPIO_INIT();
  //LED_STATIC_TEST();
  //LED_DYNAMIC_TEST();
  LED_TIME_INIT(120, 1001, 235956789, LED_MODE_HM);
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

