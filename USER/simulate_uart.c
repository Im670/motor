#include "stm8s.h"
#include <stdarg.h>
#include <string.h>
#include "simulate_uart.h"
#include "delay.h"
#include "ringbuf.h"

/*simulate_uart.c*/
typedef enum
{
	SM_UART_IDE_MODE,  //空闲
	SM_UART_RX_MODE,
	SM_UART_TX_MODE	
}SM_UART_RXTX_MODE;

typedef enum
{
	TX_LOAD_DATA,
	TX_SEND_BIT
}SM_TX_STEP;


typedef struct
{
	SM_UART_RXTX_MODE rxtx_mode ;
	SM_TX_STEP        tx_step ;	
	u8                startbit ;
	u8 				  tx_end ;
	u8                rx_end ;
	u8                rx_data ;
	u8                tx_data ;
}sm_uart_ctrl_t;


static sm_uart_ctrl_t sm_uart_ctrl ;

int simulate_uart_init(int bps);
int simulate_uart_start_rx(void);
int simulate_uart_start_tx(void);
void simulate_uart_rxtx_proc(void);
int simulate_uart_send(u8* pdata ,u8 len);
void simulate_uart_send_byte(u8 bsend);
u8   simulate_uart_read_byte(void);
void sm_printf(uint8_t *Data,...);


extern   u32 recv_time ;

/*-----------------
TX - PC7
RX - PC6
------------------*/



#define SM_SET_TX_OUT()     GPIO_Init(GPIO_TX,GPIO_TX_PIN,GPIO_MODE_OUT_PP_HIGH_FAST )
#define SM_SET_RX_IN()      GPIO_Init(GPIO_RX,GPIO_RX_PIN,GPIO_MODE_IN_PU_NO_IT )

#define SM_UART_SET_BIT(x)  (((x)!=0) ? GPIO_WriteHigh(GPIO_TX,GPIO_TX_PIN) : GPIO_WriteLow(GPIO_TX,GPIO_TX_PIN))
#define SM_UART_STOP()   
#define SM_UART_READ_BIT()   GPIO_ReadInputPin(GPIO_RX,GPIO_RX_PIN)

/*------------------
sample bit:采一位翻转一次
------------------*/
#define GPIO_SAMP_BIT       GPIOB
#define GPIO_SAMP_BIT_PIN   GPIO_PIN_4

#define SM_SET_SAMP_BIT_OUT() GPIO_Init(GPIO_SAMP_BIT,GPIO_SAMP_BIT_PIN,GPIO_MODE_OUT_PP_HIGH_FAST )
#define SM_SET_SAMP_NOR()     GPIO_WriteReverse(GPIO_SAMP_BIT,GPIO_SAMP_BIT_PIN)

#define _PRESCALER_ TIM6_PRESCALER_128

ringbuf_t sm_tx_ringbuf ;
ringbuf_t sm_rx_ringbuf ;

u8 sm_tx_buffer[120];
u8 sm_rx_buffer[240];

u8 sm_period = 0;

int simulate_uart_init(int bps)
{
	u16 Prescaler = 1 << _PRESCALER_;

	u8  timebase = (u8)(Prescaler/16) ; //us

	//t = 1000/1200 = 0.83 ms
	u16 period = 1000000L/(bps*timebase);	//

	sm_period = period; 
 
	memset(sm_tx_buffer,0,sizeof(sm_tx_buffer));
	memset(sm_rx_buffer,0,sizeof(sm_rx_buffer));
	memset(&sm_uart_ctrl,0,sizeof(sm_uart_ctrl));

	if(rb_create(&sm_tx_ringbuf, sm_tx_buffer, sizeof(sm_tx_buffer))<0)
	{
		return -1;
	}	 

	if(rb_create(&sm_rx_ringbuf, sm_rx_buffer, sizeof(sm_rx_buffer))<0)
	{
		return -1;
	}

	SM_SET_RX_IN(); 
	SM_SET_TX_OUT(); 
	SM_SET_SAMP_BIT_OUT();

	/* Time base configuration */
	TIM6_TimeBaseInit(_PRESCALER_, period - 1);   // 
	/* Clear TIM4 update flag */
	TIM6_ClearFlag(TIM6_FLAG_UPDATE);
	/* Enable update interrupt */
	TIM6_ITConfig(TIM6_IT_UPDATE, ENABLE);

	/* enable interrupts */
	enableInterrupts();

	/* Enable TIM4 */
	TIM6_Cmd(ENABLE);

	return 0;
}



int simulate_uart_start_rx(void)
{
	sim();
	sm_uart_ctrl.rx_end = 0;	
	sm_uart_ctrl.startbit = 0;
	sm_uart_ctrl.rxtx_mode = SM_UART_RX_MODE;

	TIM6_Cmd(DISABLE);
	sm_uart_en_rx_irq(1);	
	
	rim();
}

int simulate_uart_start_tx(void)
{	
	sim();
	sm_uart_ctrl.tx_end = 0;	
	sm_uart_ctrl.startbit = 0;
	sm_uart_ctrl.rxtx_mode = SM_UART_TX_MODE;
	TIM6_Cmd(ENABLE);
	rim();
}

void simulate_uart_rxtx_proc(void)
{
	static u8 cnt = 0;
	
	switch(sm_uart_ctrl.rxtx_mode)
	{
		case SM_UART_IDE_MODE:
			{
				//
			}
			break;				
		case SM_UART_RX_MODE:
			{
				if(sm_uart_ctrl.startbit == 0)
				{
					if(SM_UART_READ_BIT()==0) //起始位为 0 开始接收
					{
						sm_uart_ctrl.startbit = 1;
						sm_uart_ctrl.rx_data = 0;
						cnt = 0;

						sm_uart_en_rx_irq(0);
					}
					else
					{
						//wait
					}
					break;
				}

				if(cnt < 9)
				{
					cnt ++;
				}
				
				if(cnt <= 8)
				{
					SM_SET_SAMP_NOR();
					
					if(SM_UART_READ_BIT())
					{
						//no_receive_flag=1;
						//recv_time=get_time_tick ();
						sm_uart_ctrl.rx_data |= 0x80;
					}

					if(cnt < 8)
					{
						sm_uart_ctrl.rx_data >>= 1;
					}
				}
				else
				{					
					if(SM_UART_READ_BIT())
					{
						//no_receive_flag=1;
						sm_uart_ctrl.rx_end = 1;
						sm_uart_ctrl.startbit = 0;
						//recv_time=get_time_tick ();
						rb_write(&sm_rx_ringbuf,&sm_uart_ctrl.rx_data,1);
						rb_updatewr(&sm_rx_ringbuf,1);
						cnt = 0;
						sm_timer_enable(0);
						sm_uart_en_rx_irq(1);
						
					}
				}
			}
			break;
		case SM_UART_TX_MODE:
			{
				switch(sm_uart_ctrl.tx_step)
				{
					case TX_LOAD_DATA:
					{
						if(rb_read(&sm_tx_ringbuf,&sm_uart_ctrl.tx_data,1) > 0)
						{							
							sm_uart_ctrl.tx_step = TX_SEND_BIT;
							cnt = 0;
						}
					}
					break;
					case TX_SEND_BIT:
					{
						if(sm_uart_ctrl.startbit == 0)
						{
							sm_uart_ctrl.startbit = 1;
							SM_UART_SET_BIT(0); //起始位
							break;
						}
						cnt ++;
						if(cnt <= 8)
						{					
							SM_UART_SET_BIT(sm_uart_ctrl.tx_data & 0x01);
							sm_uart_ctrl.tx_data >>= 1;
						}
						else if(cnt == 9)
						{
							sm_uart_ctrl.startbit = 0;
							SM_UART_SET_BIT(1); //结束位
							sm_uart_ctrl.tx_end = 1;
							cnt = 0;
							rb_updaterd(&sm_tx_ringbuf, 1);
							sm_uart_ctrl.tx_step = TX_LOAD_DATA;
						}
					}
					break;
				}
			}
			break;
		default:
			break;
	}

	TIM6_TimeBaseInit(_PRESCALER_, sm_period -1);   // 
}


int simulate_uart_send(u8* pdata ,u8 len)
{
	if(NULL == pdata || 0 == len)
	{
		return -1;
	}
	sim();
	rb_write(&sm_tx_ringbuf,pdata,len);
	rb_updatewr(&sm_tx_ringbuf, len);
	rim();
	return len;
}

void simulate_uart_send_byte(u8 bsend)
{	
	simulate_uart_send(&bsend,1);
}

u8 simulate_uart_read_byte(void)
{	
	u8 data = 0;
	data = sm_uart_ctrl.rx_data;
	sm_uart_ctrl.rx_data = 0;
	return data;
}



void sm_uart_test(void)
{
	//simulate_uart_start_tx();
	//simulate_uart_start_rx();
	u8 data = 0;

	simulate_uart_start_tx();
	sm_printf("sm_printf\n");
	delay_ms(1000);  

	//return;
	
	//simulate_uart_start_rx();		
	
	while(1)
	{
		simulate_uart_start_rx();		
		while(sm_uart_ctrl.rx_end == 0);
		data= simulate_uart_read_byte();

		simulate_uart_start_tx();
		simulate_uart_send_byte(data);
		while(sm_uart_ctrl.tx_end == 0);

		//sm_printf("sm_printf\n");
		
		//delay_ms(1000);  
	}
}



extern char * itoa(u16 Value,u8 *pString);
extern char * HexToStr(char *s,int x);

//sm_printf("format",...);

void sm_printf(uint8_t *Data,...)
{ 
	const char *s;
    int d;
    char buf[16];
	
    va_list ap;
    va_start(ap, Data);

	sim();
	
	while(*Data!=0)
    { 
		if(*Data==0x5c)                       //'\'
        {                                    
			switch (*++Data)
            {
				case 'r':							          //
					simulate_uart_send_byte(0x0d);
					Data++;
					break;
				case 'n':							          //
					simulate_uart_send_byte(0x0d);
					Data++;
					break;
				
				default:
					Data++;
				    break;
			}
		}
		else if(*Data=='%')
        {                                     //
			switch (*++Data)
            {               
				case 's':										  
                	s = va_arg(ap, const char *);
                	while(*s)
                    {
						simulate_uart_send_byte(*s++);                        
                	}
					Data++;
                	break;
            	case 'd':										  
                	d = va_arg(ap, int);
					memset(buf,0,sizeof(buf));
                	itoa(d, buf);
					s = buf;
                	while(*s)
                    { 
						simulate_uart_send_byte(*s++);                        
                	}
					Data++;
                	break;
				case 'x':
					d = va_arg(ap, int);
					memset(buf,0,sizeof(buf));
					HexToStr(buf,d);
					s = buf;
					while(*s)
                    { 
						simulate_uart_send_byte(*s++);      
                	}
					Data++;
                	break;
				default:
					Data++;
				    break;
			}	
            
		}
		else
        {
            simulate_uart_send_byte(*Data);
            Data++;
		}        	
	}

	rim();

}

void sm_uart_en_rx_irq(int enable)
{	
	if(enable)
	{
		GPIO_Init(GPIO_RX,GPIO_RX_PIN,GPIO_MODE_IN_PU_IT ); 
		
		EXTI_SetExtIntSensitivity(EXTI_PORT_GPIORX,EXTI_SENSITIVITY_FALL_ONLY);
	}
	else
	{
		GPIO_Init(GPIO_RX,GPIO_RX_PIN,GPIO_MODE_IN_PU_NO_IT ); 
	}
}


void sm_reset_timer_period(u8 period)
{

	TIM6_Cmd(DISABLE);
	
	TIM6_SetCounter(0);

	TIM6_TimeBaseInit(_PRESCALER_, period);   // 

	TIM6_Cmd(ENABLE);
}

void sm_timer_enable(int enable)
{
	if(enable)
	{
		TIM6_Cmd(ENABLE);
	}
	else
	{
		TIM6_Cmd(DISABLE);
	}
}
