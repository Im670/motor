 /* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */
#include "stm8s.h"
#include "delay.h"
#include "usr_usart.h"
#include "usr_digital.h"
#include "usr_adc.h"
#include "Usr_digital.h"
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
//extern void digital_display_proc(void);
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

//#define _ONLY_TEST_


#define TEST_CHN (MOTOR_CHN1)

#define CUT_OFF_VALUE (600)


//#define _TEST_NORMAL_ADD_SPEED_     //测试2秒一次自动增速

#define ADD_STEP  (1)  //占空比微调步长
void moto_check(void);
int led_on=0,led_play=0;
motor_ctrl_cmd_t for_led_flash;
u8 stall_flag[MOTOR_CHN_NUM];
bool pause_flag = 0;
u16 last_adc_average[MOTOR_CHN_NUM]= {0};
u16 now_adc_average[MOTOR_CHN_NUM] = {0};
void stall_out (u8 v);
u16 DEC_FUN(u16 D1 ,u16 D2);
extern u8 no_receive_flag;
extern u8 flag_smg;
void weaken_fun(u16 *volue1,u16 *value2,u8 i);





main()
{		
	u32 cur_time = 0;
	u32 motor_check_time = 0;

	u8 i =0 ;
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
	GPIO_Init(GPIOE, GPIO_PIN_5, GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_WriteHigh(GPIOE, GPIO_PIN_5);		
	
	digital_set_num(0);
	//digital_en_flash(1);	

	delay_ms(100);  //

	
	
	while (1)
	{
		sim();
		cur_time = get_time_tick();
		rim();
		
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
				
				for_led_flash = motor_ctrl;
				
				proto_send_data(led_on,led_play);
			}			
		}    		
		
 		//if(cur_time >= motor_check_time + 100 )  // 100 ms 检测一次电机状态
 		if(cur_time >= motor_check_time + 2 )// 2ms检测一次；
		{
			motor_check_time = cur_time;
			moto_check();
		}

	//	digital_display_proc();
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
	u8 i = 0;
	u16 adc_average = 0;
	
	u8 cur_speed = 0;
	u8 nom_speed=0;
	
	for(i = 0; i< MOTOR_CHN_NUM;i++)
	{
		last_adc_average[i] = motor_get_adc_average((MOTOR_CHN_E)i);
		
		weaken_fun(&last_adc_average[i], &now_adc_average[i],i);
		
		adc_average = now_adc_average[i];
#if 0
		if(adc_average >= CUT_OFF_VALUE)
		{
			motor_power_enable(0);
			digital_set_num(0);
			digital_en_flash(1);
			while(1);			
		}
#endif		
		nom_speed = motor_get_dst_speed((MOTOR_CHN_E)i);
		cur_speed = motor_get_speed((MOTOR_CHN_E)i);
		
		if(cur_speed <= 45)
		{
			if (adc_average > (3+(4*cur_speed)))
			{
				cur_speed ++;
			}
			else if (adc_average<=(3+(3*cur_speed)))
			{
				cur_speed--;
			}
		}
		
		if((cur_speed>45)&&(cur_speed<65))
		{
			if (adc_average > (6+(4*cur_speed)))
			{
				cur_speed ++;
			}
			else if (adc_average<=(6+(3*cur_speed)))
			{
				cur_speed--;
			}
		} 
		
		if(cur_speed>=65)
		{
			if (adc_average > (6+(5*cur_speed)))
			{
				cur_speed ++;
			}
			else if (adc_average<=(6+(4*cur_speed)))
			{
				cur_speed--;
			}
		}

	
		if((for_led_flash.data[i])>0)
		{
			if(adc_average>300||stall_flag[i]==1)
			{
			 	led_on=led_on&(~(1<<i));
		 		led_play=led_play|(1<<i);
			}
			else
			{
			  	led_on=led_on|(1<<i);
			 	led_play=led_play&(~(1<<i));
			}
		}
		else
		{
			led_on=led_on&(~(1<<i));
			led_play=led_play&(~(1<<i));
		}
		
		if(flag_smg==1)
		{
			led_on=led_on&(~(1<<i));
			led_play=led_play&(~(1<<i));
		}
		
		if(adc_average>500&&cur_speed>=99)
		{
			stall_flag[i]=2;
		}
		else if ((adc_average<=500)&&pause_flag==0)
		{
			stall_flag[i]=0;
		}
		if (nom_speed<1) 
		{
			cur_speed=0;
		}
		else if ((cur_speed < nom_speed)||cur_speed>100)
		{
			cur_speed = nom_speed;
		}
		
		if(nom_speed>97)
		{
			cur_speed=99;
		}
		
		stall_out(i);
		if(stall_flag[i]==1)
		{
			motor_set_speed((MOTOR_CHN_E)i,0);
		}
		else if(stall_flag[i]==0)
		{
			motor_set_speed((MOTOR_CHN_E)i, cur_speed);
		}
		else if((stall_flag[i]==2))
		{
			motor_set_speed((MOTOR_CHN_E)i, 100);
		}
	} 

}
void stall_out (u8 v)// 
{
	static u16  time_out[MOTOR_CHN_NUM];
	if(stall_flag[v]==2)
	{ 
		time_out[v]++;
		if(time_out[v]>=150)
		{
			stall_flag[v]=1;
			pause_flag=1;
		}
	}
	else if(stall_flag[v]==1)
	{
		time_out[v]++;
		if(time_out[v]>=250)
		{ 
			stall_flag[v]=0;
			pause_flag=0;
		}
	}
	else if(stall_flag[v]==0)
	{
		time_out[v]=0;
	}
	
}
u16 DEC_FUN(u16 D1 ,u16 D2)
{
	u16 DEC;
	if(D1>=D2)
	{
		DEC = D1-D2;
	}
	else
	{
		DEC = D2-D1;
	}
	return DEC;
}
void weaken_fun(u16* volue1,u16* value2,u8 i)
{
	static u8 add[MOTOR_CHN_NUM];
	if(DEC_FUN(*volue1, *value2)>4)
	{
		add[i]++;
	}
	else
	{
		add[i]=0;
	}
	
	if(add[i]>20)
	{
		add[i]=0;
		*value2=*volue1; 
	}
}

