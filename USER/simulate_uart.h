#ifndef _SM_UART_H_
#define _SM_UART_H_

int simulate_uart_init(int bps);
int simulate_uart_start_rx(void);
int simulate_uart_start_tx(void);
void simulate_uart_rxtx_proc(void);
int simulate_uart_send(u8* pdata ,u8 len);
void sm_printf(uint8_t *Data,...);
void sm_uart_test(void);

#endif
