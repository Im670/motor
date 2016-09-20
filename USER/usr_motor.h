#ifndef __MOTOR_H__
#define __MOTOR_H__

#define SPEED_NUM  (80)          //速度档位数 10
#define MIN_CCR    (250-1)       //PWM 最小CCR  对应25% 占空比
#define MAX_CCR    (1000-1)      //PWM 最大CCR  对应100% 占空比
#define SAMPLE_TIME   (50)       // ms ADC采样时间


typedef enum
{
	MOTOR_CHN1, // =0
	MOTOR_CHN2, // =1
	MOTOR_CHN3,
	MOTOR_CHN4,
	MOTOR_CHN5,
	MOTOR_CHN6,
	MOTOR_CHN_NUM
}MOTOR_CHN_E;


typedef struct 
{
	u8 cur_speed;
	u8 dst_speed;
	u16 cur_ccr;
}motor_t;

typedef enum
{
	M_WAITE,   //等待趋近稳定
	M_STEADY,  //稳定
	M_STUCK    //堵住
}M_STEAE;



typedef struct
{
	u8 init_finished; 
	motor_t motor[MOTOR_CHN_NUM];
}motor_config_t;


int  motor_init(void);
int motor_set_cur_ccr(MOTOR_CHN_E chn ,u16 ccr);
u16 motor_get_cur_ccr(MOTOR_CHN_E chn);
int  motor_set_speed(MOTOR_CHN_E chn ,u8 speed);
u8  motor_get_speed(MOTOR_CHN_E chn );
void motor_self_check(void);
int  motor_control_proc(void);
void motor_ccr_proc(void);
u16  motor_get_adc_average(MOTOR_CHN_E chn);
int proc_motor(u16 adc_value);
int get_m_state(void);
void set_m_state(M_STEAE state);

int motor_is_stucked(void);

#endif

