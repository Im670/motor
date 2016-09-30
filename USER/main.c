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


#define TEST_CHN (MOTOR_CHN1)


//#define _TEST_NORMAL_ADD_SPEED_     //测试2秒一次自动增速

#define ADD_STEP  (1)  //占空比微调步长

void moto_check(void);


main()
{		
	u32 cur_time = 0;
	u32 motor_check_time = 0;

	int len = 0;
	motor_ctrl_cmd_t motor_ctrl;
	memset(&motor_ctrl,0,sizeof(motor_ctrl));
	
	
	 /* Clock divider to HSI/1 */
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);  

	motor_init();	
	proto_init();	
	//sm_uart_test();
	//proto_test();	
    adc_init(); 
   
    //usart1_init();
    digital_init();	
	
	digital_set_num(0);
	delay_ms(500);  //
	
	while (1)
	{
		if(is_proto_sending())
		{
			if(is_proto_send_finished())
			{
				proto_start_recv();
			}
		}
		else
		{
			len = proto_read_data((u8*)&motor_ctrl,sizeof(motor_ctrl));
			if( len > 0 )
			{			
				proto_proc_data(&motor_ctrl);			
			
				// led_en = 
				// led_flash = 
				
				proto_send_data(6,6);
			}			
		}    


		cur_time = get_time_tick();
		
		if(cur_time >= motor_check_time + 100 )  // 100 ms 检测一次电机状态
		{
			motor_check_time = cur_time;
			moto_check();
		}
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


void moto_check(void)
{
	int i = 0;
	
	u16 adc_average = 0;
	u8 cur_speed = 0;
#ifndef _ONLY_TEST_	
	for(i = MOTOR_CHN1; i< MOTOR_CHN_NUM;i++)
#else
	for(i = TEST_CHN; i< TEST_CHN+1;i++)
#endif	
	{
		adc_average = motor_get_adc_average((MOTOR_CHN_E)i);
		cur_speed = motor_get_speed((MOTOR_CHN_E)i);
		
		//DEBUG_PRINTF("----------------%d通道AD值为:%d-----------------\n",i,adc_average );

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
			cur_speed = 1;
		}
		//DEBUG_PRINTF("-----------------------%dch switch to speed %d--------------\n",i,(u16)cur_speed );
		motor_set_speed((MOTOR_CHN_E)i,cur_speed);
	
	} 

}

