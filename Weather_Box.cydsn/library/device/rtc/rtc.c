#include "rtc.h"

#define RTC_SOURCE_COUNTER                          (0u)
#define RTC_COUNTER_ENABLE                          (1u)
#define RTC_COUNT_PERIOD                            ((uint32)32767)
#define RTC_INTERRUPT_SOURCE                        CY_SYS_WDT_COUNTER0_INT

static
uint32  rtc_tick_value;


CY_ISR(WDT_Handler)
{
  if(CySysWdtGetInterruptSource() & RTC_INTERRUPT_SOURCE){
    rtc_tick_value++;
    CySysWdtClearInterrupt(RTC_INTERRUPT_SOURCE);
  }  
}

void rtc_initial(void)
{
    return;
}

uint32 rtc_get_current_tick(void)
{
   return(0);
}
