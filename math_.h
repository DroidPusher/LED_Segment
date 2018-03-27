/* Define to prevent recursive inclusion -------------------------------------*/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __math__H
#define __math__H


/* Includes ------------------------------------------------------------------*/



uint32_t pow(uint32_t base, uint32_t power)
{
  if (power == 0)
  {
    return 1;
  }
  uint32_t result = base;
  while (power > 1)
  {
    result = result * base;
    power = power - 1;
  }
  return result;
}
#endif /* __math__H */
