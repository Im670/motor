/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "stm8s.h"
#include "delay.h"
#include "usr_usart.h"
#include "usr_digital.h"
#include "usr_adc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "simulate_uart.h"
#include "usr_motor.h"
#include "public.h"
#include "cmd_proto.h"


#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

int sort_buff(u16 * pbuff,u16 len);

#define SET_SPEED 1

#define _ONLY_TEST_


#define TEST_CHN (MOTOR_CHN6)


#define _TEST_NORMAL_ADD_SPEED_     //测试2秒一次自动增速

#define ADD_STEP  (1)  //占空比微调步长



main()
{

    u16 num = 0;
	int i = 0;
    u16 adc_value = 0;	

	u8 cur_speed = 0;
	u8 last_speed = 0;
	u16 cnt = 0;

	u16 adc_average = 0;
	u16 time_out = 0;
	u16 cur_ccr = 0;
	
	 /* Clock divider to HSI/1 */
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);  

	motor_init();	
	simulate_uart_init(1200);
	//sm_uart_test();
	proto_test();
	
    adc_init(); 
   
    usart1_init();
    digital_init();

	

	//motor_self_check();

#ifndef _ONLY_TEST_	
	for(i = MOTOR_CHN1; i< MOTOR_CHN_NUM;i++)
#else
	for(i = TEST_CHN; i< TEST_CHN+1;i++)
#endif	
	{
		motor_set_speed((MOTOR_CHN_E)i,SET_SPEED);
	}
	
	digital_set_num(1);
	delay_ms(500);  //
	
	while (1)
	{
        
 #if 1       
		for(i = TEST_CHN; i< TEST_CHN+1;i++)
		{
			/*if(i == MOTOR_CHN2)
			{
				continue;
			}*/
			adc_average = motor_get_adc_average((MOTOR_CHN_E)i);
			cur_ccr = motor_get_cur_ccr((MOTOR_CHN_E)i);
			cur_speed = motor_get_speed((MOTOR_CHN_E)i);
			
			//proc_motor(adc_average);
		#if 1	
			/*if(i == MOTOR_CHN1)*/
			{
				DEBUG_PRINTF("MOTOR_CHN%d cur_speed:%d adc_average:%d \n",\
					i+1,(u16)cur_speed,adc_average );
			}
		#endif	
			/*if(!motor_is_adc_in_expected((MOTOR_CHN_E)i,adc_average,(u16)cur_speed))
			{
				DEBUG_PRINTF("MOTOR_CHN%d is in ERROR adc = %d\n",i+1,adc_average);
				if(cur_speed + 1 <= SPEED_NUM)
				{
					motor_set_speed((MOTOR_CHN_E)i,cur_speed + 1);					
				}				
			}
			else
			{
				motor_set_speed((MOTOR_CHN_E)i,1);				
			}*/
			
			//delay_ms(100);
		}
#endif

/*---------------------------
检测堵转调速代码
----------------------------*/
#if 0

		//这部分代码是60ms 执行一次
		if(++cnt == 60/20)
		{
			cnt = 0;

			if(time_out>0)
			{
				time_out -- ;

				if(M_STUCK != get_m_state())
				{
					time_out = 0;
					motor_set_speed(TEST_CHN,SET_SPEED);
					set_m_state(M_WAITE);
				}				
				delay_ms(SAMPLE_TIME);				
				continue;
			}		

			if(motor_is_stucked())
			{
				if(cur_speed + 1 <= SPEED_NUM)
				{
					u8 next_speed = 0;
					/*if(cur_speed < 5)
					{
						next_speed = 5;
					}
					else*/
					{
						next_speed = cur_speed + 1;
					}
					motor_set_speed((MOTOR_CHN_E)TEST_CHN,next_speed);	
					time_out = 240/60;  //240ms
				}				
			}			
			else if(/*M_STEADY == get_m_state() &&*/ cur_speed != SET_SPEED)
			{
				motor_set_speed(TEST_CHN,SET_SPEED);
				set_m_state(M_WAITE);
			}				
			
		}
#endif


/*---------------------------
用于测试
----------------------------*/
#ifdef _TEST_NORMAL_ADD_SPEED_
#if 1
	    if(++cnt == 500/SAMPLE_TIME)     // 2 秒一次
		{
		    cnt = 0;

			if( cur_speed + 1 <= SPEED_NUM )
			{
			    for(i = TEST_CHN; i< TEST_CHN+1;i++)
				{	
				    motor_set_speed((MOTOR_CHN_E)i,cur_speed + ADD_STEP);
					
					DEBUG_PRINTF("-switch to speed %d-\n",cur_speed + 1);
				}	
			}
			else
			{
			    for(i = TEST_CHN; i< TEST_CHN+1;i++)
				{
				    motor_set_speed((MOTOR_CHN_E)i,1);
					
				}	
			}	
		}
#endif


#if 0
		
#ifndef _ONLY_TEST_	
		for(i = MOTOR_CHN1; i< MOTOR_CHN_NUM;i++)
#else
		for(i = TEST_CHN; i< TEST_CHN+1;i++)
#endif	
		{
			adc_average = motor_get_adc_average((MOTOR_CHN_E)i);
			cur_speed = motor_get_speed((MOTOR_CHN_E)i);
			
			DEBUG_PRINTF("----------------%d通道AD值为:%d-----------------\n",i,adc_average );

			if(cur_speed < 15)
			{
				if(adc_average > (10+(4*cur_speed)))
				{
					cur_speed ++;
					//cur_speed = cur_speed +2;
				}  
				else if (adc_average<(10+(3*cur_speed)))
				{				
					cur_speed --;					
				}
			}
			else if ((cur_speed >= 15)&&( cur_speed < 45))
			{
				if(adc_average >(10+(5*cur_speed)))
				{	
					cur_speed ++;
					//cur_speed = cur_speed +2;
				}  
				else if (adc_average < (10 + (4*cur_speed)))
				{
					cur_speed --;
					
				}	
			}
			else if (cur_speed >= 45)
			{
				if(adc_average > (10+(6*cur_speed)))
				{
					cur_speed ++;
					//cur_speed[i]=cur_speed[i]+2;
				}  
				else if (adc_average < (10+(5*cur_speed)))
				{
					cur_speed --;					
				}	
			}	
			
			if(cur_speed < 1)
			{
				cur_speed =1;
			}
			DEBUG_PRINTF("-----------------------%dch switch to speed %d--------------\n",i,(u16)cur_speed );
			motor_set_speed((MOTOR_CHN_E)i,cur_speed);
		//	motor_set_speed(2,cur_speed[i]);
		} 
#endif
#endif
		
			
		delay_ms(SAMPLE_TIME);	        
	}
}

int sort_buff(u16 * pbuff,u16 len)
{
	int i = 0;
	int j = 0;
	int min = 0;
	u16 temp = 0;
	
	if(pbuff == NULL || len <= 0)
	{
		return -1;
	}

	for(j=0;j<len-1;j++)
	{
		min = j;
		
		for(i=j+1;i<len;i++)
		{
			if(pbuff[i]<pbuff[min])
			{
				min = i;
			}
		}

		if(j!=min)
		{
			temp = pbuff[j];
			pbuff[j] = pbuff[min];
			pbuff[min] =  temp;			
		}		
	}	
}


