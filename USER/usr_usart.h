#ifndef __USART_H
#define __USART_H

void usart1_init(void);
void usart1_printf(uint8_t *Data,...);

void show_buffer(int len, u8* buffer);

//#define PRINTF_ON

#define USART1_PRINTF_ON 0

#ifdef PRINTF_ON
#include <stdio.h>
#endif


#endif
