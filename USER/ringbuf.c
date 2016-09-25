#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ringbuf.h"

static int rb_leftspace(ringbuf_t *pringbuf);

int rb_create(ringbuf_t *pringbuf, u8 *pbuffer,int size)
{
	if(NULL == pringbuf || NULL == pbuffer)
	{
		return -1;
	}
	
	pringbuf->size=size;
	pringbuf->pdata=pbuffer;
	if(NULL == pringbuf->pdata)
	{
		return -1;
	}
	memset(pringbuf->pdata, 0, pringbuf->size);

	return 0;
}


int rb_write(ringbuf_t *pringbuf, unsigned char* pdata, int len)
{
	int buttlen = 0;
	if(len > pringbuf->size)
	{
		//data is too long, drop it
		return -1;
	}
	
	if(rb_leftspace(pringbuf) < len)
	{
		//RF buffer full,over write it	
	}
	
	buttlen = pringbuf->size - pringbuf->write;
	if( buttlen >= len)
	{
		memcpy(pringbuf->pdata+pringbuf->write,pdata,len);
	}
	else
	{
		memcpy(pringbuf->pdata+pringbuf->write,pdata,buttlen);
		memcpy(pringbuf->pdata,(unsigned char*)pdata+buttlen,len - buttlen);
	}

	return 0;
}

void rb_updatewr(ringbuf_t *pringbuf, int len)
{		
	pringbuf->write = (pringbuf->write + len)%pringbuf->size;
	if(len !=0 && pringbuf->read == pringbuf->write)
	{
		pringbuf->bfull = 1;
	}
}

int rb_read(ringbuf_t *pringbuf, unsigned char *pbuf, int len)
{
	int buttlen = pringbuf->size - pringbuf->read;
	if(NULL == pringbuf || NULL == pbuf)
	{
		return -1;
	}

	if(rb_length(pringbuf) < len)
	{
		return 0;
	}
	
	if( buttlen >= len)
	{
		memcpy(pbuf,pringbuf->pdata+pringbuf->read,len);
	}
	else
	{
		memcpy(pbuf,pringbuf->pdata+pringbuf->read,buttlen);
		memcpy(pbuf+buttlen,pringbuf->pdata,len-buttlen);
	}

	return len;
}

void rb_updaterd(ringbuf_t *pringbuf, int len)
{
	pringbuf->read = (pringbuf->read + len)%pringbuf->size;
	if(len!=0 && pringbuf->read == pringbuf->write)
	{
		pringbuf->bfull = 0;
	}
}

void rb_clear(ringbuf_t *pringbuf)
{	
	memset(pringbuf->pdata, 0, pringbuf->size);
	pringbuf->write = 0;
	pringbuf->read = 0;
	pringbuf->bfull = 0;
}

static int rb_leftspace(ringbuf_t *pringbuf)
{
	if(pringbuf->read == pringbuf->write)
	{
		return (pringbuf->bfull ? 0 : pringbuf->size);
	}
	
	if(pringbuf->read > pringbuf->write)
	{
		return pringbuf->read - pringbuf->write;
	}
	else
	{
		return pringbuf->read + pringbuf->size - pringbuf->write;
	}
}

int rb_length(ringbuf_t *pringbuf)
{
	int len = 0;
	
	if(pringbuf->write == pringbuf->read && pringbuf->bfull == 0)
	{
		return 0;
	}
	else if(pringbuf->write > pringbuf->read)
	{
		len = (int)(pringbuf->write) - (int)(pringbuf->read);
	}
	else
	{
		len = (int)(pringbuf->write) + (int)(pringbuf->size) - (int)(pringbuf->read);
	}
	
	return len;
}


