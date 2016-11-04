#ifndef _SM_UART_H_
#define _SM_UART_H_

#define GPIO_TX         GPIOC
#define GPIO_TX_PIN     GPIO_PIN_6
#define GPIO_RX         GPIOC
#define GPIO_RX_PIN     GPIO_PIN_7

#define EXTI_PORT_GPIORX     EXTI_PORT_GPIOC

int simulate_uart_init(int bps);
int simulate_uart_start_rx(void);
int simulate_uart_start_tx(void);
void simulate_uart_rxtx_proc(void);
int simulate_uart_send(u8* pdata ,u8 len);
void sm_printf(uint8_t *Data,...);
void sm_uart_test(void);
void sm_uart_en_rx_irq(int enable);
void sm_reset_timer_period(u8 period);
void sm_timer_enable(int enable);


extern u8 sm_period;



#endif
