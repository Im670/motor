
#include "stm8s.h"
#include "string.h"
#include "usr_adc.h"
#include "usr_motor.h"
#include "delay.h"
#include "usr_usart.h"
#include "simulate_uart.h"
#include "usr_digital.h"
#include "public.h"



#define GPIO_MT_POWER       GPIOE
#define GPIO_MT_POWER_PIN   GPIO_PIN_5

#define KEEP_ZERO_NUM (10)     //连续差值为0次数
#define KEEP_INC_NUM  (20)     //连续差值>=0次数
#define KEEP_DEC_NUM  (10)     //连续差值<=0次数

#define KEEP_CLEAR_NUM (5)

#define ADC_SAMPLE_NUM  (50)   //Adc 采样个数

#define ERR_RANG (10)

extern int sort_buff(u16 * pbuff,u16 len);

int motor_init(void);
int motor_set_speed(MOTOR_CHN_E chn ,u8 speed);
u8 motor_get_speed(MOTOR_CHN_E chn );
int motor_control_proc(void);
void motor_ccr_proc(void);
ADC1_Channel_TypeDef motor_chn_to_adc_Chn(MOTOR_CHN_E chn);

static motor_config_t m_motor_config ;

static u16 get_ccr_by_speed( u8  speed)
{
	static int add = (MAX_CCR-MIN_CCR)/SPEED_NUM;

	if(speed > SPEED_NUM)
	{
		speed = SPEED_NUM;
	}
	
	if(0 == speed)
	{
		return 0;
	}
	else
	{
		return (MIN_CCR + (speed*add));
	}
}

void pwm_init(void)
{
	/*Y1 Y2 Y3 Y4*/ 
	TIM1_DeInit();
	TIM1_TimeBaseInit(15, TIM1_COUNTERMODE_UP, MAX_CCR, 0);

	TIM1_OC1Init(TIM1_OCMODE_PWM1, TIM1_OUTPUTSTATE_ENABLE, TIM1_OUTPUTNSTATE_DISABLE,
               0, TIM1_OCPOLARITY_LOW, TIM1_OCNPOLARITY_HIGH, TIM1_OCIDLESTATE_SET,
               TIM1_OCNIDLESTATE_RESET); 
	
	TIM1_OC2Init(TIM1_OCMODE_PWM1, TIM1_OUTPUTSTATE_ENABLE, TIM1_OUTPUTNSTATE_DISABLE,
	           0, TIM1_OCPOLARITY_LOW, TIM1_OCNPOLARITY_HIGH, TIM1_OCIDLESTATE_SET, 
	           TIM1_OCNIDLESTATE_RESET);

	TIM1_OC3Init(TIM1_OCMODE_PWM1, TIM1_OUTPUTSTATE_ENABLE, TIM1_OUTPUTNSTATE_DISABLE,
	           0, TIM1_OCPOLARITY_LOW, TIM1_OCNPOLARITY_HIGH, TIM1_OCIDLESTATE_SET,
	           TIM1_OCNIDLESTATE_RESET);

	TIM1_OC4Init(TIM1_OCMODE_PWM1, TIM1_OUTPUTSTATE_ENABLE, 0, TIM1_OCPOLARITY_LOW,
	           TIM1_OCIDLESTATE_SET);

	TIM1_ITConfig(TIM1_IT_UPDATE,ENABLE);
	/* TIM1 counter enable */
	TIM1_Cmd(ENABLE);	
    /* TIM1 Main Output Enable */
    TIM1_CtrlPWMOutputs(ENABLE);

	/*Y5 Y6*/
	TIM5_DeInit();
	TIM5_TimeBaseInit(TIM5_PRESCALER_16, MAX_CCR);
	//TIM5_ITConfig(TIM5_IT_UPDATE,ENABLE);
	
	TIM5_OC1Init(TIM5_OCMODE_PWM1, TIM5_OUTPUTSTATE_ENABLE, 0, TIM5_OCPOLARITY_LOW); 
	TIM5_OC2Init(TIM5_OCMODE_PWM1, TIM5_OUTPUTSTATE_ENABLE, 0, TIM5_OCPOLARITY_LOW); 

	TIM5_ITConfig(TIM5_IT_UPDATE,ENABLE);	
	TIM5_Cmd(ENABLE); 
   
	/**/
	
}


int motor_init(void)
{
	(void)pwm_init();
	memset(&m_motor_config,0,sizeof(m_motor_config));	
	//memset(&m_ctrl,0,sizeof(m_ctrl));
	//m_ctrl.inc_dir = 1;

	m_motor_config.init_finished = 1; 
	return 0;
}


int motor_set_cur_ccr(MOTOR_CHN_E chn ,u16 ccr)
{
	if(ccr > MAX_CCR || ccr < 0)
	{
		return -1;
	}	
	
	m_motor_config.motor[chn].cur_ccr = ccr;
	return 0;
}


u16 motor_get_cur_ccr(MOTOR_CHN_E chn)
{
	return m_motor_config.motor[chn].cur_ccr;
}


/*-------------------------
motor_set_speed(chn,speed)
---------------------------*/
int motor_set_speed(MOTOR_CHN_E chn ,u8 speed)
{
	u16 ccr = 0;
	if(speed > SPEED_NUM || speed < 0)
	{
		return -1;
	}	
	
	m_motor_config.motor[chn].cur_speed = speed;
	ccr = get_ccr_by_speed(speed);
	motor_set_cur_ccr(chn,ccr);

	return 0;
}

u8 motor_get_speed(MOTOR_CHN_E chn)
{
	return m_motor_config.motor[chn].cur_speed;
}

int motor_set_dst_speed(MOTOR_CHN_E chn ,u8 speed)
{
	u16 ccr = 0;
	if(speed > SPEED_NUM || speed < 0)
	{
		return -1;
	}	
	
	m_motor_config.motor[chn].dst_speed = speed;	
	

	return 0;
}

u8 motor_get_dst_speed(MOTOR_CHN_E chn)
{
	return m_motor_config.motor[chn].dst_speed;
}


/*
放到TIM update 中断函数里刷新CCR
*/
void motor_ccr_proc(void)
{
	int chn = 0;

	if(!m_motor_config.init_finished)
	{
		return;
	}

#ifdef _6_CHN_		
	for(chn = MOTOR_CHN3 ; chn < MOTOR_CHN_NUM; chn++)
#else
	for(chn = MOTOR_CHN4 ; chn < MOTOR_CHN_NUM; chn++)
#endif
	{
		u8 speed = m_motor_config.motor[chn].cur_speed;
		u16 ccr  = get_ccr_by_speed(speed);
		
		switch(chn)
		{

			case MOTOR_CHN6:// 6
				{
					TIM1_SetCompare1(ccr);
					//TIM1_SetCompare1(m_motor_config.motor[chn].cur_ccr);
				}
				break;
			case MOTOR_CHN5:// 5
				{
					TIM1_SetCompare2(ccr);
					//TIM1_SetCompare2(m_motor_config.motor[chn].cur_ccr);
				}
				break;
			case MOTOR_CHN4://4    /*4*/
				{
					TIM1_SetCompare3(ccr);
					//TIM1_SetCompare3(m_motor_config.motor[chn].cur_ccr);
				}
				break;
#ifdef _6_CHN_
			case MOTOR_CHN3:/*3*/
				{
					TIM1_SetCompare4(ccr);
					//TIM1_SetCompare4(m_motor_config.motor[chn].cur_ccr);
				}
				break;
#endif

		}
	}
}

void motor_ccr_proc_chn1_2(void)
{
	int chn = 0;

	if(!m_motor_config.init_finished)
	{
		return;
	}
#ifdef _6_CHN_
	for(chn = 0 ; chn < MOTOR_CHN3;chn++)
	{
		u8 speed = m_motor_config.motor[chn].cur_speed;
		u16 ccr  = get_ccr_by_speed(speed);
		
		switch(chn)
		{			
			case MOTOR_CHN2:/*2*/
				{
					TIM5_SetCompare1(ccr);
					//TIM5_SetCompare1(m_motor_config.motor[chn].cur_ccr);
				}
				break;
			case MOTOR_CHN1:/*1*/
				{
					TIM5_SetCompare2(ccr);
					//TIM5_SetCompare2(m_motor_config.motor[chn].cur_ccr);
				}
				break;
		}
	}
#endif
}


int motor_control_proc()
{
	return 0;
}


extern int sort_buff(u16 * pbuff,u16 len);


ADC1_Channel_TypeDef motor_chn_to_adc_Chn(MOTOR_CHN_E chn)
{
	switch(chn)
	{

		case MOTOR_CHN6:/*6*/
			return ADC1_CHANNEL_0;
		case MOTOR_CHN5:/*5*/
			return ADC1_CHANNEL_1;
		case MOTOR_CHN4:/*4*/
			return ADC1_CHANNEL_2;
#ifdef _6_CHN_
		case MOTOR_CHN3:/*3*/
			return ADC1_CHANNEL_3;
		case MOTOR_CHN2:/*2*/
			return ADC1_CHANNEL_6;
		case MOTOR_CHN1:/*1*/
			return ADC1_CHANNEL_5;
#endif

		default:
			return ADC1_CHANNEL_0;
	}
}


u16  motor_get_adc_average(MOTOR_CHN_E chn)
{
	int i = 0;	
	u16 adc_average = 0;
	
	u16 adc_buff[ADC_SAMPLE_NUM] = {0};

	memset(adc_buff,0,sizeof(adc_buff));
	
	for(i = 0 ; i < ADC_SAMPLE_NUM ; i++)
	{
		adc_buff[i] = adc_get_value(motor_chn_to_adc_Chn(chn));			
	}

	sort_buff(adc_buff,ADC_SAMPLE_NUM);
		
	adc_average = adc_buff[ADC_SAMPLE_NUM/2];
	
	return adc_average;
}



void motor_power_init(void)
{
	GPIO_Init(GPIO_MT_POWER, GPIO_MT_POWER_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_WriteHigh(GPIO_MT_POWER, GPIO_MT_POWER_PIN);	
}

void motor_power_enable(u8 enable)
{
	if(enable)
	{
		GPIO_WriteHigh(GPIO_MT_POWER, GPIO_MT_POWER_PIN);		
	}
	else
	{
		GPIO_WriteLow(GPIO_MT_POWER, GPIO_MT_POWER_PIN);	
	}
}


