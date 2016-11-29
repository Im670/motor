#include "stm8s.h"
#include "usr_motor.h"
#include <stdio.h>
#include <string.h>
#include "simulate_uart.h"
#include "ringbuf.h"
#include "usr_digital.h"
#include "cmd_proto.h"



typedef enum
{
	PROTO_RECVING,     //接收
	PROTO_SENDING      //正在发送 	
}PROTO_STATE_E;


static PROTO_STATE_E proto_state = PROTO_RECVING;

extern ringbuf_t sm_rx_ringbuf ;
extern ringbuf_t sm_tx_ringbuf ;
u8 flag_smg;

void proto_init(void);
int proto_read_data(u8 *pdata ,u8 len);
int proto_send_data(u8 len_en,u8 led_flash);
int proto_proc_data(motor_ctrl_cmd_t *pm_ctrl);
void proto_start_recv(void);
int is_proto_sending(void);
int is_proto_send_finished(void);
void proto_test(void);

u8  calcrc_1byte(u8  abyte) 
{ 
	u8  i,crc_1byte;  
	crc_1byte=0;                //设定crc_1byte初值为0
	for(i = 0; i < 8; i++) 
	{ 
		if(((crc_1byte^abyte)&0x01)) 
		{ 
			crc_1byte^=0x18;  
			crc_1byte>>=1; 
			crc_1byte|=0x80; 
		}       
		else  
		{
			crc_1byte>>=1;
		}
		abyte>>=1;       
	}
	return crc_1byte;
}
u8  calcrc_bytes(u8  *p,u8  len)
{
	u8  crc=0;
	while(len--) //len为总共要校验的字节数
	{
		crc=calcrc_1byte(crc^*p++);
	}
	return crc;  //若最终返回的crc为0，则数据传输正确
}

/* 
调用方法：
 unsigned char pBuffer[5] = {0x55, 0x00, 0x00, 0xFF, 0x01};
 int aaa = 0;
 aaa = calcrc_bytes(pBuffer, 5);//方法2调用

*/


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

	//chksum = calcrc_bytes(pdata,len);

	return chksum;
}





void proto_init(void)
{
	simulate_uart_init(2400);
	proto_start_recv();
}

int is_proto_sending(void)
{
	return (PROTO_SENDING == proto_state);
}

int proto_send_data(u8 len_en,u8 led_flash)
{
	led_ctrl_cmd_t led_ctrl;

	memset(&led_ctrl,0,sizeof(led_ctrl));

	led_ctrl.head   = TX_HEAD;
	led_ctrl.data[0] = len_en;
	led_ctrl.data[1] = led_flash;

	led_ctrl.chksum = get_chksum((u8*)&led_ctrl,sizeof(led_ctrl.data)+1);
	
	proto_state = PROTO_SENDING;
	simulate_uart_start_tx();
	simulate_uart_send((u8*)&led_ctrl,sizeof(led_ctrl));
	
	return 0;
	
}

int proto_read_data(u8 *pdata, u8 len) 
{
	int rdlen = 0;	
	u8 chksum = 0;
	int chksum2 = 0;
	u8 debug = 0;

	if(rb_length(&sm_rx_ringbuf) < len)
	{
		return 0;
	}
	
	rdlen = rb_read(&sm_rx_ringbuf,(u8*)&pdata[0],1);// .
	if(rdlen <= 0)
	{
		debug = 1;
		return -1;
	}	

	if(pdata[0] != RX_HEAD)
	{
		rb_updaterd(&sm_rx_ringbuf,1);
		return -1;
	}

	rb_updaterd(&sm_rx_ringbuf,1);

	rdlen = rb_read(&sm_rx_ringbuf,(u8*)&pdata[1],sizeof(motor_ctrl_cmd_t)-1);
	if(rdlen <=0)
	{
		debug = 2;
		return -1;
	}

	rb_updaterd(&sm_rx_ringbuf,sizeof(motor_ctrl_cmd_t)-1);

	chksum = get_chksum((u8*)pdata,sizeof(motor_ctrl_cmd_t) - 1); 
	
	if(pdata[sizeof(motor_ctrl_cmd_t) - 1] != chksum )	
	{
		debug = 3;
 
 return -1;
	}	
	
	return len;
}


int proto_proc_data(motor_ctrl_cmd_t *pm_ctrl)
{
	u8 i = 0;
	u8 speed = 0;
	u8 show_speed;
	
//	static u8 last_spped[MOTOR_CHN_NUM];
	

	if(NULL == pm_ctrl)
	{
		return 0;
	}
	if(pm_ctrl->para==0)
	{
		flag_smg=1;

		motor_power_enable(0);
	}
	else
	{
		flag_smg=0;
		motor_power_enable(1);
	}
	
	for( i = 0 ; i < MOTOR_CHN_NUM; i++)
	{
		speed = pm_ctrl->data[i] * pm_ctrl->para / 100;
#if 0
		/*数据消抖*/
		if(speed != last_spped[i])
		{			
			last_spped[i] = speed;
			continue;
		}		
		last_spped[i] = speed;
#endif
		if( speed == motor_get_dst_speed((MOTOR_CHN_E)i))
		{
			continue;
		}
		
		motor_set_speed((MOTOR_CHN_E)i,speed);
		motor_set_dst_speed((MOTOR_CHN_E)i,speed);
	}

	show_speed =  pm_ctrl->para;
	
	digital_set_num(show_speed);

	return 0;
}


void proto_start_recv(void)
{
	proto_state = PROTO_RECVING;
	simulate_uart_start_rx();
}

int is_proto_send_finished(void)
{
	return (0 == rb_length(&sm_tx_ringbuf));
}

void proto_test(void)
{
	int len = 0;
	motor_ctrl_cmd_t motor_ctrl;
	memset(&motor_ctrl,0,sizeof(motor_ctrl));
	
	while(1)
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
				
				proto_send_data(6,6);
			}			
		}
	}
}




