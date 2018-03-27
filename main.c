#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include <stm32f0xx.h>
#include "delay.h"
#include "math_.h"

void SystemClock_Config(void);

uint32_t timeCounter1, timeCounter2;
uint32_t ms, sec, min, hours, days, months, years, digit;
uint32_t anode_size, cathode_size;
uint32_t LED_FPS, LED_ClockCycle, LED_RefreshCycle; /* in miliseconds */
uint32_t LED_ERROR;

/* LED pin connections:
  1-6 - PB7-PB3, PD2
  7-12 - PB15, PC6-PC9, PF6
  LED pin desctiption:
  A(11), B(7), C(4), D(2), E(1), F(10), G(5), DP(3), D1(12), D2(9), D3(8), D4(6)
  These parameters should be set manualy
 */

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

/* mask for anode_pins[] */
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
#define END	{0xFFFFFFFF,0xFFFFFFFF, 0x4}
#define D1	{PORTD1, PIND1, AHBEND1}
#define D2	{PORTD2, PIND2, AHBEND2}
#define D3	{PORTD3, PIND3, AHBEND3}
#define D4	{PORTD4, PIND4, AHBEND4}

Digit LED_Digits[11] =
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
  { {A, B, G, F, C, D, END}, 0x6F },
  /* error */
  { {B, C, E, F, G, END}, 0x76 }
}; 
Pin pins_anode[9] = {A, B, C, D, E, F, G, DP, END};
Pin pins_cathode[5] = {D4, D3, D2, D1, END};

typedef struct {
  uint8_t name;
  uint8_t pointPos;
} Mode;

Mode LED_MODE_TIME_sec_ms = {0x00, 0x08};
Mode LED_MODE_TIME_min_sec = {0x01, 0x04};
Mode LED_MODE_TIME_hours_min = {0x02, 0x04};
Mode LED_MODE_DATE_mon_days = {0x03, 0x04};
Mode LED_MODE_DATE_years = {0x04, 0x10};
Mode LED_MODE_test = {0x05, 0x0F};
Mode LED_MODE_default = {0x06, 0x00}; /*nothing displayed*/
Mode LED_Mode;

void LED_GPIO_INIT()
{
  uint8_t i=0;
  while (pins_anode[i].AHBEN != ((Pin)END).AHBEN)
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
  while (pins_cathode[i].AHBEN != ((Pin)END).AHBEN)
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

void LED_INIT(uint8_t fps_, uint32_t clock_cycle_, uint32_t current_time_, uint32_t current_date_)
{
  LED_ERROR = 0x0;
  LED_Mode = LED_MODE_default;

  LED_FPS = fps_;
  LED_RefreshCycle = 1000/ (LED_FPS*4);
  LED_ClockCycle = clock_cycle_;

  ms = current_time_%1000;
  sec = current_time_/1000%100;
  min = current_time_/100000%100;
  hours = current_time_/10000000;

  days = current_date_%100;
  months = current_date_/100%100;
  years = current_date_/10000;
}


void LED_STATIC_TEST()
{
  uint8_t i = 0;
  while (pins_anode[i].AHBEN != ((Pin)END).AHBEN)
  {
    LL_GPIO_SetOutputPin(pins_anode[i].PORT, pins_anode[i].PIN);
    i = i + 1;
  }
  i = 0;
  while (pins_cathode[i].AHBEN != ((Pin)END).AHBEN)
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
    while (pins_cathode[i].AHBEN != ((Pin)END.AHBEN))
    {
      LL_GPIO_ResetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      j = 0;
      while (pins_anode[j].AHBEN != ((Pin)END).AHBEN )
      {
        LL_GPIO_SetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        Delay(LED_RefreshCycle);
        LL_GPIO_ResetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        j = j + 1;
      }
      LL_GPIO_SetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      i = i + 1;
    }
    i = i - 1;
    while (i != 0xFF)
    {
      LL_GPIO_ResetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      j = anode_size - 1;
      while (j != 0xFF)
      {
        LL_GPIO_SetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        Delay(LED_RefreshCycle);
        LL_GPIO_ResetOutputPin(pins_anode[j].PORT, pins_anode[j].PIN);
        j = j - 1;
      }
      LL_GPIO_SetOutputPin(pins_cathode[i].PORT, pins_cathode[i].PIN);
      i = i - 1;
    } 
    i = i + 1;
  }
}

void LED_SET_MODE(Mode mode_)
{
  LED_Mode = mode_;
}

void LED_Refresh(void)
{
  uint32_t value;
  uint8_t i = 0;
  /* set the value to show according to the mode */
  if (LED_Mode.name == LED_MODE_test.name) { value = 88888; /*all anodes are ON*/ }
  else if (LED_Mode.name == LED_MODE_TIME_sec_ms.name) { value =  ms + sec*1000; }
  else if (LED_Mode.name == LED_MODE_TIME_min_sec.name) { value = sec + min*100; }
  else if (LED_Mode.name == LED_MODE_TIME_hours_min.name) { value = min + hours*100; }
  else if (LED_Mode.name == LED_MODE_DATE_mon_days.name) { value = days + months*100; }
  else if (LED_Mode.name == LED_MODE_DATE_years.name) { value = years; }
  else if (LED_Mode.name == LED_MODE_default.name) { return; }
  else {return;}
  /* turn off previous LED digit */
  if (!LL_GPIO_IsOutputPinSet(pins_cathode[digit].PORT, pins_cathode[digit].PIN))
  {
    LL_GPIO_SetOutputPin(pins_cathode[digit].PORT, pins_cathode[digit].PIN);
  }
  digit = (digit+1)%4;
  /*show the value on the digit*/
  if (LED_ERROR & 0x01)
  {
    value = 0xA;
  } else {
    value = value/pow(10,digit)%10; 
  }
  while (pins_anode[i].AHBEN != ((Pin)END).AHBEN)
  {
    if ( LED_Digits[value].mask & (1<<i) )
    {
      LL_GPIO_SetOutputPin(pins_anode[i].PORT, pins_anode[i].PIN);
    } else if (LL_GPIO_IsOutputPinSet(pins_anode[i].PORT, pins_anode[i].PIN))
    {
      LL_GPIO_ResetOutputPin(pins_anode[i].PORT, pins_anode[i].PIN);
    }
    i = i + 1;
  }
  if ((1<<digit) & LED_Mode.pointPos)
  {
    LL_GPIO_SetOutputPin(((Pin)DP).PORT, ((Pin)DP).PIN); /* show points of the mode */
  }
  LL_GPIO_ResetOutputPin(pins_cathode[digit].PORT, pins_cathode[digit].PIN); /* ground cathode of current LED digit */
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
        hours = hours + min / 60;
        min = min % 60;
        if (hours > 23)
        {
          days = days + hours/24;
          hours = hours % 24;
          if (days > 31)
          {
            months = months + days/32;
            days = days%32;
            if (days == 0) { days = 1; }
            if (months > 11)
            {
              years = years + months/12;
              months = months%12;
              if (months == 0) { months = 1; }
              if (years > 9999)
              {
                LED_ERROR = 0x1;
              }
            }
          }
        }
      }
    }
  }
}

void
SysTick_Handler(void) 
{
  TimingDelay_Decrement();
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
    LED_Refresh();
  }
} 

int main(void)
{
  SystemClock_Config();
  LED_GPIO_INIT();
  LED_INIT(60, 1001, 235959987, 920181131); /*FPS, ClockCycle, CurrTime(hh:mm:ss:msx3), CurrDate(yyyy:mm:dd)*/
  //LED_STATIC_TEST();
  LED_SET_MODE(LED_MODE_DATE_years);
  //LED_DYNAMIC_TEST();
  // 
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

