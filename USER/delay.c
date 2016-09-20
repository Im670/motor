#include "stm8s.h"

#define TIM6_PERIOD       124

static u16 gtime_delay = 0;
static u16 gtime_out = 0;
static u32 time_tick = 0;
void delay_init(void)
{
  /* TIM6 configuration:
   - TIM6CLK is set to 16 MHz, the TIM4 Prescaler is equal to 128 so the TIM1 counter
   clock used is 16 MHz / 128 = 125 000 Hz
  - With 125 000 Hz we can generate time base:
      max time base is 2.048 ms if TIM4_PERIOD = 255 --> (255 + 1) / 125000 = 2.048 ms
      min time base is 0.016 ms if TIM4_PERIOD = 1   --> (  1 + 1) / 125000 = 0.016 ms
  - In this example we need to generate a time base equal to 1 ms
   so TIM4_PERIOD = (0.001 * 125000 - 1) = 124 */
   
  /* Time base configuration */
  TIM6_TimeBaseInit(TIM6_PRESCALER_128, TIM6_PERIOD);
  /* Clear TIM4 update flag */
  TIM6_ClearFlag(TIM6_FLAG_UPDATE);
  /* Enable update interrupt */
  TIM6_ITConfig(TIM6_IT_UPDATE, ENABLE);
  
  /* enable interrupts */
  enableInterrupts();

  /* Enable TIM4 */
  TIM6_Cmd(ENABLE);
}


void delay_ms(u16 t_ms)
{
    gtime_delay = t_ms;
    while(gtime_delay);
}

void delay_irq_proc(void)
{
    if(gtime_delay)
    {
        gtime_delay -- ;
    }

	if(gtime_out)
    {
        gtime_out -- ;
    }

	time_tick ++;
}

u32 get_time_tick(void)
{
	return time_tick;
}

void set_time_out(u16 t_ms)
{
	gtime_out = t_ms;
}

u16 get_time_out(void)
{
	return gtime_out;
}

