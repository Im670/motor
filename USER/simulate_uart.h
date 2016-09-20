#ifndef _SM_UART_H_
#define _SM_UART_H_

int simulate_uart_config(int bps);
int simulate_uart_start_rx(void);
int simulate_uart_start_tx(u8 byte);
void simulate_uart_rxtx_proc(void);
void sm_uart_test(void);

#endif
