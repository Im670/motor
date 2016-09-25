#include "stm8s.h"
#include <stdarg.h>
#include <string.h>
#include "usr_usart.h"


#ifdef PRINTF_ON   //重定向printf函数 但是会消耗代码空间 FLASH小的mcu不建议开启
/* Private define ------------------------------------------------------------*/
#ifdef _RAISONANCE_
#define PUTCHAR_PROTOTYPE int putchar (char c)
#define GETCHAR_PROTOTYPE int getchar (void)
#elif defined (_COSMIC_)
#define PUTCHAR_PROTOTYPE char putchar (char c)
#define GETCHAR_PROTOTYPE char getchar (void)
#else /* _IAR_ */
#define PUTCHAR_PROTOTYPE int putchar (int c)
#define GETCHAR_PROTOTYPE int getchar (void)
#endif /* _RAISONANCE_ */

/* Private function ----------------------------------------------------------*/
/**
  * @brief Retargets the C library printf function to the UART.
  * @param c Character to send
  * @retval char Character sent
  */
PUTCHAR_PROTOTYPE
{
    /* Write a character to the UART1 */
    UART1_SendData8(c);
    /* Loop until the end of transmission */
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);

    return (c);
}

/**
  * @brief Retargets the C library scanf function to the USART.
  * @param None
  * @retval char Character to Read
  */
GETCHAR_PROTOTYPE
{
#ifdef _COSMIC_
    char c = 0;
#else
    int c = 0;
#endif
    /* Loop until the Read data register flag is SET */
    while (UART1_GetFlagStatus(UART1_FLAG_RXNE) == RESET);
    c = UART1_ReceiveData8();
    return (c);
}
#endif    

void usart1_init(void)
{
	
	UART1_DeInit();
	/* UART1 configuration ------------------------------------------------------*/
	/* UART1 configured as follow:
	   - BaudRate = 115200 baud  
	   - Word Length = 8 Bits
	   - One Stop Bit
	   - No parity
	   - Receive and transmit enabled
	   - UART1 Clock disabled
	*/
	UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
	         UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TX_ENABLE);
	UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
}

char * itoa(u16 Value,u8 *pString);
char * HexToStr(char *s,int x);
void usart1_printf(uint8_t *Data,...);

static void usart1_send_byte(u8 byte)
{
    UART1_SendData8(byte);	
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);
}

void usart1_printf(uint8_t *Data,...)
{ 
#if USART1_PRINTF_ON
	const char *s;
    int d;
    char buf[16];
	
    va_list ap;
    va_start(ap, Data);
	
	while(*Data!=0)
    {                                         //
		//while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);
		if(*Data==0x5c)                       //'\'
        {                                    
			switch (*++Data)
            {
				case 'r':							          //
					usart1_send_byte(0x0d);
					Data++;
					break;
				case 'n':							          //
					usart1_send_byte(0x0d);
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
						usart1_send_byte(*s++);                        
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
						usart1_send_byte(*s++);                        
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
						usart1_send_byte(*s++);      
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
            usart1_send_byte(*Data);
            Data++;
		}        	
	}
#endif
}

void show_buffer(int len, u8* buffer)
{	
	int i=0;
	for(i=0;i<len;i++)
	{
		if(0 == (i+1)%16 || i == len-1)
			usart1_printf("%x\n",(u16)buffer[i]);
		else if(0 == (i+1)%8)
			usart1_printf("%x    ",(u16)buffer[i]);
		else
			usart1_printf("%x ",(u16)buffer[i]);
	}

}

char * itoa(u16 Value,u8 *pString)
{
    u8  Buff[10];
    u16 Cnt=0;

	if(pString == NULL)
	{
		return NULL;
	}
	
    if(Value == 0) 
	{
		Buff[Cnt++] =0x30;
    }
    while(Value)
    {
        Buff[Cnt++] = Value % 10 + '0';
        Value /= 10;
    }
    Buff[Cnt] ='\0';
    while(Cnt--)
    {
        *pString++  =Buff[Cnt];
    }
    *pString  ='\0';  

	return pString;
}

char * HexToStr(char *s,int x)
{
	int i=0,n;
    while(x>0)
	{
		n=x%16;
		if(n<10) s[i]=n+'0';
		else s[i]=n-10+'A';
		x=x/16;
		i++;
	}
	if(i==0)
	{
		s[0]= '0';
		s[1] = '\0';
		return s;
	}
	s[i]='\0';
 
	n=i-1;
	i=0;
	while(i<n)
	{ 
		char t=s[i];
		s[i]=s[n];
		s[n]=t;
		i++;
		n--;
	}
	return s;
}
