#include "stm8s.h"

/*simulate_uart.c*/
typedef enum
{
	SM_UART_IDE_MODE,  //空闲
	SM_UART_RX_MODE,
	SM_UART_TX_MODE	
}SM_UART_RXTX_MODE;

SM_UART_RXTX_MODE gRxTxMode = SM_UART_IDE_MODE;
u8 gTxEnd = 0;
u8 gRxEnd = 0;
u8 gTxTemp = 0;
u8 gRxTemp = 0;


int simulate_uart_config(int bps);
int simulate_uart_start_rx(void);
int simulate_uart_start_tx(u8 byte);
void simulate_uart_rxtx_proc(void);

#define SM_UART_SET_BIT(x)  (((x)!=0) ? GPIO_WriteHigh(GPIOD,GPIO_PIN_5) : GPIO_WriteLow(GPIOD,GPIO_PIN_5))
#define SM_UART_STOP()   
#define SM_UART_READ_BIT()   GPIO_ReadInputPin(GPIOD,GPIO_PIN_6)



int simulate_uart_config(int bps)
{
	//timebase = 0.016ms
	//1000/bps/0.016ms  = 1000000/(bps*16)
	u16 period = 1000000L/(bps*16);

	GPIO_Init(GPIOD,GPIO_PIN_6,GPIO_MODE_IN_PU_NO_IT );
	GPIO_Init(GPIOD,GPIO_PIN_5,GPIO_MODE_OUT_PP_HIGH_FAST );
	/* Time base configuration */
	TIM5_TimeBaseInit(TIM5_PRESCALER_128, period);
	/* Clear TIM5 update flag */
	TIM5_ClearFlag(TIM5_FLAG_UPDATE);
	/* Enable update interrupt */
	TIM5_ITConfig(TIM5_IT_UPDATE, ENABLE);

	/* enable interrupts */
	enableInterrupts();

	/* Enable TIM5 */
	TIM5_Cmd(ENABLE);
}



int simulate_uart_start_rx(void)
{
	gRxEnd = 0;
	gRxTxMode = SM_UART_RX_MODE;
}

int simulate_uart_start_tx(u8 byte)
{
	gTxTemp = byte;	
	gTxEnd = 0;
	gRxTxMode = SM_UART_TX_MODE;
}

void simulate_uart_rxtx_proc(void)
{
	static u8 cnt = 0;

	switch(gRxTxMode)
	{
		case SM_UART_IDE_MODE:
			{
				//
			}
			break;				
		case SM_UART_RX_MODE:
			{
				if(cnt == 0)
				{
					if(SM_UART_READ_BIT()==0) //起始位为 0 开始接收
					{
						cnt ++;
						gRxTemp = 0;
					}
					else
					{
						//wait
					}
				}
				else if(cnt <= 8)
				{
					if(!SM_UART_READ_BIT())
					{
						gRxTemp |= 0x80;
					}
					else
					{
						//
					}
					
					cnt ++;
					gRxTemp >>= 1;
				}
				else
				{
					cnt = 0;
					if(SM_UART_READ_BIT())
					{
						gRxEnd = 1;
						gRxTxMode = SM_UART_IDE_MODE;
					}
				}
			}
			break;
		case SM_UART_TX_MODE:
			{
				cnt ++;
				if(cnt <= 8)
				{					
					SM_UART_SET_BIT(gTxTemp&0x01);
					gTxTemp >>= 1;
				}
				else if(cnt == 9)
				{
					SM_UART_SET_BIT(1); //结束位
					gTxEnd = 1;
					cnt = 0;
					gRxTxMode = SM_UART_IDE_MODE;
				}
			}
			break;
		default:
			break;
	}
}

void sm_uart_test(void)
{
	while(1)
	{
		simulate_uart_start_rx();
		while(gRxEnd == 0);
		simulate_uart_start_tx(gRxTemp);
		while(gTxEnd == 0);
	}
}


