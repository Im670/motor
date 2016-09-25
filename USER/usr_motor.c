
#include "stm8s.h"
#include "string.h"
#include "usr_adc.h"
#include "usr_motor.h"
#include "delay.h"
#include "usr_usart.h"
#include "usr_digital.h"


#define KEEP_ZERO_NUM (10)     //连续差值为0次数
#define KEEP_INC_NUM  (20)     //连续差值>=0次数
#define KEEP_DEC_NUM  (10)     //连续差值<=0次数

#define KEEP_CLEAR_NUM (5)


typedef struct
{
	M_STEAE m_state;
	char inc_dir;  // 0 :负增长 1:正增长
	u16 last_adc;
	int inc_cnt;
	int dec_cnt;
	int zero_cnt;
	int flag;
	u16 waite_time;
}m_ctrl_t;

m_ctrl_t m_ctrl;



#define ADC_SAMPLE_NUM  (10)   //Adc 采样个数

#define ERR_RANG (10)

extern int sort_buff(u16 * pbuff,u16 len);

int motor_init(void);
int motor_set_speed(MOTOR_CHN_E chn ,u8 speed);
u8 motor_get_speed(MOTOR_CHN_E chn );
int motor_control_proc(void);
void motor_ccr_proc(void);
ADC1_Channel_TypeDef motor_chn_to_adc_Chn(MOTOR_CHN_E chn);

u16 CCR_table[SPEED_NUM+1]={0};   //CCR_table[0] =0 CCR_table[1] = min

static motor_config_t m_motor_config ;

static int make_ccr_table(int min, int max,int divnum)
{
	int add = 0;
	int i = 0;
		
	if(min >= max || divnum <= 0)
	{
		return -1;
	}

	add = (max - min)/divnum;
	
	CCR_table[0] = 0;
	
	for(i=0;i<divnum;i++)
	{
		CCR_table[i+1] = min + (i*add);
		usart1_printf("(%d,%d)\n",i+1,CCR_table[i+1]);
	}	
	return 0;
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
	memset(CCR_table,0,sizeof(CCR_table));
	memset(&m_ctrl,0,sizeof(m_ctrl));
	m_ctrl.inc_dir = 1;
	make_ccr_table(MIN_CCR,MAX_CCR,SPEED_NUM);
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
	if(speed > SPEED_NUM || speed < 0)
	{
		return -1;
	}	
	
	m_motor_config.motor[chn].cur_speed = speed;

	motor_set_cur_ccr(chn,CCR_table[speed]);

	return 0;
}

u8 motor_get_speed(MOTOR_CHN_E chn)
{
	return m_motor_config.motor[chn].cur_speed;
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
		
	for(chn = MOTOR_CHN1 ; chn < MOTOR_CHN5 ; chn++)
	{
		int speed = m_motor_config.motor[chn].cur_speed;
		
		switch(chn)
		{
			case MOTOR_CHN1:
				{
					TIM1_SetCompare1(CCR_table[speed]);
					//TIM1_SetCompare1(m_motor_config.motor[chn].cur_ccr);
				}
				break;
			case MOTOR_CHN2:
				{
					TIM1_SetCompare2(CCR_table[speed]);
					//TIM1_SetCompare2(m_motor_config.motor[chn].cur_ccr);
				}
				break;
			case MOTOR_CHN3:
				{
					TIM1_SetCompare3(CCR_table[speed]);
					//TIM1_SetCompare3(m_motor_config.motor[chn].cur_ccr);
				}
				break;
			case MOTOR_CHN4:
				{
					TIM1_SetCompare4(CCR_table[speed]);
					//TIM1_SetCompare4(m_motor_config.motor[chn].cur_ccr);
				}
				break;			
		}
	}
}

void motor_ccr_proc_chn5_6(void)
{
	int chn = 0;

	if(!m_motor_config.init_finished)
	{
		return;
	}
		
	for(chn = MOTOR_CHN5 ; chn < MOTOR_CHN_NUM;chn++)
	{
		int speed = m_motor_config.motor[chn].cur_speed;
		
		switch(chn)
		{			
			case MOTOR_CHN5:
				{
					TIM5_SetCompare1(CCR_table[speed]);
					//TIM5_SetCompare1(m_motor_config.motor[chn].cur_ccr);
				}
				break;
			case MOTOR_CHN6:
				{
					TIM5_SetCompare2(CCR_table[speed]);
					//TIM5_SetCompare2(m_motor_config.motor[chn].cur_ccr);
				}
				break;
		}
	}
}


int motor_control_proc()
{
	return 0;
}


extern int sort_buff(u16 * pbuff,u16 len);

void motor_self_check(void)
{
	u16 i = 0;
	u32 pre = get_time_tick();
	u16 adc = 0;
	for(i = 1;i <= SPEED_NUM;i++)
	{
		motor_set_speed(MOTOR_CHN1,i);
		delay_ms(100);
		adc = motor_get_adc_average(MOTOR_CHN1);
		usart1_printf("(%d,%d)\n",i,adc);		
	}
	usart1_printf("cost time %d ms\n",(u16)(get_time_tick()-pre));
}


ADC1_Channel_TypeDef motor_chn_to_adc_Chn(MOTOR_CHN_E chn)
{
	switch(chn)
	{
		case MOTOR_CHN1:
			return ADC1_CHANNEL_0;
		case MOTOR_CHN2:
			return ADC1_CHANNEL_1;
		case MOTOR_CHN3:
			return ADC1_CHANNEL_2;
		case MOTOR_CHN4:
			return ADC1_CHANNEL_3;
		case MOTOR_CHN5:
			return ADC1_CHANNEL_6;
		case MOTOR_CHN6:
			return ADC1_CHANNEL_5;
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

void change_m_state(M_STEAE state)
{
	m_ctrl.m_state = state;

	m_ctrl.flag = 0;
	m_ctrl.zero_cnt = 0;
	m_ctrl.inc_cnt = 0;
	m_ctrl.dec_cnt= 0;
	m_ctrl.waite_time = 0;
}



int proc_motor(u16 adc_value)
{
	int dec = 0;
	dec = (int)(adc_value - m_ctrl.last_adc);
	
	switch(m_ctrl.m_state)
	{
		case M_WAITE:
			{
				m_ctrl.waite_time ++;
				
				if(adc_value < 10) //小电流忽略不计
				{
					break;
				}
				
				if(m_ctrl.inc_dir) //正增长
				{
					if(dec == 0)
					{
						if(++m_ctrl.zero_cnt >= KEEP_ZERO_NUM)
						{
							usart1_printf("waite_time:%d\n",(SAMPLE_TIME*m_ctrl.waite_time));
							change_m_state(M_STEADY);	//切到稳定状态
							
							break;
						}
					}
					else
					{
						m_ctrl.zero_cnt = 0;
					}
				}
				else
				{
					if(dec > 0)
					{
						if(!m_ctrl.inc_cnt)
						{
							m_ctrl.inc_cnt = 1;
							break;
						}
					}

					if(m_ctrl.inc_cnt)
					{
						if(dec < 0)
						{
							m_ctrl.inc_cnt = 0;
							break;
						}

						if(++m_ctrl.inc_cnt >= KEEP_INC_NUM)
						{
							change_m_state(M_STUCK);
							break;
						}
					}
				
					if(dec == 0)
					{
						m_ctrl.zero_cnt++;
							
						if(m_ctrl.zero_cnt >= KEEP_ZERO_NUM)
						{							
							change_m_state(M_STEADY);							
							break;
						}

						if(m_ctrl.zero_cnt >= KEEP_CLEAR_NUM)
						{
							m_ctrl.inc_cnt = 0;   //计数清零
							m_ctrl.dec_cnt= 0;
						}						
					}
					else
					{
						m_ctrl.zero_cnt = 0;				
					}					
										
				}				
			}
			break;
		case M_STEADY:
			{
				if(dec > 0)
				{
					if(!m_ctrl.flag)
					{
						m_ctrl.flag = 1;
						break;
					}
				}	

				if(m_ctrl.flag)
				{
					if(dec < 0)
					{
						m_ctrl.inc_cnt = 0;
						m_ctrl.flag = 0;						
						break;
					}

					if(dec == 0)
					{
						if(++m_ctrl.zero_cnt >= KEEP_ZERO_NUM)
						{
							change_m_state(M_STEADY);
							break;
						}
						break;
					}
					else
					{
						m_ctrl.zero_cnt = 0;
					}

					if(++m_ctrl.inc_cnt >= KEEP_INC_NUM)
					{
						change_m_state(M_STUCK);						
						break;
					}
				}
			}
			break;
		case M_STUCK:
			{
				if(dec < 0)
				{
					if(!m_ctrl.flag )
					{
						m_ctrl.flag = 1;
						break;
					}
				}

				if(m_ctrl.flag)
				{
					if(dec > 0)
					{
						m_ctrl.dec_cnt = 0;
						m_ctrl.flag = 0;						
						break;
					}

					if(dec == 0)
					{
						if(++m_ctrl.zero_cnt >= KEEP_ZERO_NUM)
						{
							change_m_state(M_WAITE);
							break;
						}
					}
					m_ctrl.zero_cnt = 0;

					if(++m_ctrl.dec_cnt >= KEEP_DEC_NUM)
					{
						m_ctrl.inc_dir = 0;
						change_m_state(M_WAITE);
						break;
					}
				}
			}			
			break;
			
	}

	m_ctrl.last_adc = adc_value;
}

int get_m_state(void)
{
	return m_ctrl.m_state;
}

void set_m_state(M_STEAE state)
{
	m_ctrl.m_state = state;

	m_ctrl.flag = 0;
	m_ctrl.zero_cnt = 0;
	m_ctrl.inc_cnt = 0;
	m_ctrl.dec_cnt= 0;
	m_ctrl.waite_time = 0;
}

int motor_is_stucked(void)
{
	return (m_ctrl.m_state == M_STUCK);
}


