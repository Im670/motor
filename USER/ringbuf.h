#ifndef _RINGBUF_H
#define _RINGBUF_H


#include "stm8s.h"


typedef struct
{
	int size;	
	int read;
	int write;
	int bfull;
	u8 *pdata;
}ringbuf_t;

int rb_create(ringbuf_t *pringbuf, u8 *pbuffer, int size);
int rb_write(ringbuf_t *pringbuf, unsigned char* pdata, int len);
void rb_updatewr(ringbuf_t *pringbuf, int len);
int rb_read(ringbuf_t *pringbuf, unsigned char *pbuf, int len);
void rb_updaterd(ringbuf_t *pringbuf, int len);
void rb_clear(ringbuf_t *pringbuf);
int rb_length(ringbuf_t *pringbuf);

#endif

