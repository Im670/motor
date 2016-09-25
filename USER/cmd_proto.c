#include "stm8s.h"
#include "usr_motor.h"
#include <stdio.h>
#include <string.h>
#include "simulate_uart.h"
#include "ringbuf.h"
#include "usr_digital.h"
#include "cmd_proto.h"


extern ringbuf_t sm_rx_ringbuf ;
extern ringbuf_t sm_tx_ringbuf ;


void proto_init();
int proto_read_data(u8 *pdata ,u8 len);
int proto_send_data(u8 len_en,u8 led_flash);
int proto_proc_data(motor_ctrl_cmd_t *pm_ctrl);

u8 get_chksum(u8 *pdata,u8 len )
{
	u8 chksum = 0;
	u8 i = 0;
	if(NULL == pdata || 0 == len )
	{
		return 0;
	}

	for(i = 0;i < len; i++)
	{
		chksum += pdata[i];
	}

	return chksum;
}


int proto_send_data(u8 len_en,u8 led_flash)
{
	led_ctrl_cmd_t led_ctrl;

	memset(&led_ctrl,0,sizeof(led_ctrl));

	led_ctrl.head = TX_HEAD;
	led_ctrl.data[0] = len_en;
	led_ctrl.data[1] = led_flash;

	led_ctrl.chksum = get_chksum((u8*)&led_ctrl,sizeof(led_ctrl.data)+1);
	simulate_uart_send((u8*)&led_ctrl,sizeof(led_ctrl));
	return 0;
	
}


int proto_read_data(u8 *pdata, u8 len)
{
	motor_ctrl_cmd_t motor_ctrl;

	memset(&motor_ctrl,0,sizeof(motor_ctrl));
	
	if(rb_read(&sm_rx_ringbuf,(u8*)&motor_ctrl.head,1)<=0)
	{
		return -1;
	}	

	if(motor_ctrl.head != RX_HEAD)
	{
		return -1;
	}

	rb_updaterd(&sm_rx_ringbuf,1);

	if(rb_read(&sm_rx_ringbuf,(u8*)&motor_ctrl.data,sizeof(motor_ctrl_cmd_t)-1)<=0)
	{
		return -1;
	}

	rb_updaterd(&sm_rx_ringbuf,sizeof(motor_ctrl_cmd_t)-1);

	if(motor_ctrl.chksum != get_chksum((u8*)&motor_ctrl,sizeof(motor_ctrl.data) + 1))
	{
		return -1;
	}	

	memcpy(pdata,(u8*)&motor_ctrl,len);
	
	return len;
}


int proto_proc_data(motor_ctrl_cmd_t *pm_ctrl)
{
	u8 i = 0;
	u8 speed = 0;
	u8 show_speed;

	if(NULL == pm_ctrl)
	{
		return 0;
	}
	
	for( i = 0 ; i < MOTOR_CHN_NUM; i++)
	{
		speed = pm_ctrl->data[i] * pm_ctrl->para / 100;		
		motor_set_speed((MOTOR_CHN_E)i,speed);
	}

	show_speed = pm_ctrl->data[MOTOR_CHN1] * pm_ctrl->para / 100;
	
	digital_set_num(show_speed);

	return 0;
}


void proto_test(void)
{
	int len = 0;
	motor_ctrl_cmd_t motor_ctrl;
	memset(&motor_ctrl,0,sizeof(motor_ctrl));
	
	while(1)
	{
		simulate_uart_start_rx();

		len = proto_read_data((u8*)&motor_ctrl,sizeof(motor_ctrl));
		if( len > 0 )
		{			
			//sm_printf("succeed.\n");
			simulate_uart_send((u8*)&motor_ctrl,sizeof(motor_ctrl));
			//simulate_uart_start_tx();
			
		}
	}
}




