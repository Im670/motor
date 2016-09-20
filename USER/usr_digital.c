
#include "stm8s.h"



typedef enum
{
    BIT_0 = 0x01,
    BIT_1 = 0x02,
    BIT_2 = 0x04,
    BIT_3 = 0x08,
    BIT_4 = 0x10,
    BIT_5 = 0x20,
    BIT_6 = 0x40,
    BIT_8 = 0x80
}BIT_E;

/*-----------------
a PF4
b PA2
c PA1
d PD7
e PD2
f PD0
g PC5

ge
shi 
-------------------*/

#define DIGITAL_INIT_PIN_A()  GPIO_Init(GPIOF,GPIO_PIN_4,GPIO_MODE_OUT_PP_HIGH_SLOW)
#define DIGITAL_INIT_PIN_B()  GPIO_Init(GPIOA,GPIO_PIN_2,GPIO_MODE_OUT_PP_HIGH_SLOW)
#define DIGITAL_INIT_PIN_C()  GPIO_Init(GPIOA,GPIO_PIN_1,GPIO_MODE_OUT_PP_HIGH_SLOW)
#define DIGITAL_INIT_PIN_D()  GPIO_Init(GPIOD,GPIO_PIN_7,GPIO_MODE_OUT_PP_HIGH_SLOW)
#define DIGITAL_INIT_PIN_E()  GPIO_Init(GPIOD,GPIO_PIN_2,GPIO_MODE_OUT_PP_HIGH_SLOW)
#define DIGITAL_INIT_PIN_F()  GPIO_Init(GPIOD,GPIO_PIN_0,GPIO_MODE_OUT_PP_HIGH_SLOW)
#define DIGITAL_INIT_PIN_G()  GPIO_Init(GPIOC,GPIO_PIN_5,GPIO_MODE_OUT_PP_HIGH_SLOW)
#define DIGITAL_INIT_PIN_D0() GPIO_Init(GPIOB,GPIO_PIN_6,GPIO_MODE_OUT_PP_LOW_SLOW) 
#define DIGITAL_INIT_PIN_D1() GPIO_Init(GPIOB,GPIO_PIN_7,GPIO_MODE_OUT_PP_LOW_SLOW)



#define DIGITAL_SET_PIN_A(x) (((x)!=0) ? GPIO_WriteHigh(GPIOF,GPIO_PIN_4) : GPIO_WriteLow(GPIOF,GPIO_PIN_4))
#define DIGITAL_SET_PIN_B(x) (((x)!=0) ? GPIO_WriteHigh(GPIOA,GPIO_PIN_2) : GPIO_WriteLow(GPIOA,GPIO_PIN_2))
#define DIGITAL_SET_PIN_C(x) (((x)!=0) ? GPIO_WriteHigh(GPIOA,GPIO_PIN_1) : GPIO_WriteLow(GPIOA,GPIO_PIN_1))
#define DIGITAL_SET_PIN_D(x) (((x)!=0) ? GPIO_WriteHigh(GPIOD,GPIO_PIN_7) : GPIO_WriteLow(GPIOD,GPIO_PIN_7))
#define DIGITAL_SET_PIN_E(x) (((x)!=0) ? GPIO_WriteHigh(GPIOD,GPIO_PIN_2) : GPIO_WriteLow(GPIOD,GPIO_PIN_2))
#define DIGITAL_SET_PIN_F(x) (((x)!=0) ? GPIO_WriteHigh(GPIOD,GPIO_PIN_0) : GPIO_WriteLow(GPIOD,GPIO_PIN_0))
#define DIGITAL_SET_PIN_G(x) (((x)!=0) ? GPIO_WriteHigh(GPIOC,GPIO_PIN_5) : GPIO_WriteLow(GPIOC,GPIO_PIN_5))

#define DIGITAL_SET_PIN_D0(x) (((x)!=0) ? GPIO_WriteHigh(GPIOB,GPIO_PIN_6) : GPIO_WriteLow(GPIOB,GPIO_PIN_6))   
#define DIGITAL_SET_PIN_D1(x) (((x)!=0) ? GPIO_WriteHigh(GPIOB,GPIO_PIN_7) : GPIO_WriteLow(GPIOB,GPIO_PIN_7)) 



typedef enum
{
    DGT_POS_D0,
    DGT_POS_D1    
}DGT_POS_E;

//const u8 D_CODE[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71};  //共阴数码表  
//const u8 D_CODE2[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92, 0x82,0xF8,0x80,0x90,0x88,0x83,0xC6,0xA1,0x86,0x8E};	//共阳数码表
/*   
     b 
  |------
  |     |
 c|  d  |a
  |-----|
  |     |
 f|     |e
  ------|
     g  
*/
const u8 D_CODE2[]={0x08,0x6E,0x14,0x24,0x62,0x21,0x01,0x6C,0x00,0x20};	//共阳数码表


static u16 m_display_num = 0;


void digital_init(void)
{
    DIGITAL_INIT_PIN_A();
    DIGITAL_INIT_PIN_B();
    DIGITAL_INIT_PIN_C();
    DIGITAL_INIT_PIN_D();
    DIGITAL_INIT_PIN_E();
    DIGITAL_INIT_PIN_F();
    DIGITAL_INIT_PIN_G();
    DIGITAL_INIT_PIN_D0();
    DIGITAL_INIT_PIN_D1();    
}

static void digital_set_gpio(u8 code)
{
    DIGITAL_SET_PIN_A(code & BIT_0);
    DIGITAL_SET_PIN_B(code & BIT_1);
    DIGITAL_SET_PIN_C(code & BIT_2);
    DIGITAL_SET_PIN_D(code & BIT_3);
    DIGITAL_SET_PIN_E(code & BIT_4);
    DIGITAL_SET_PIN_F(code & BIT_5);
    DIGITAL_SET_PIN_G(code & BIT_6);
}


static int digital_show_one_num(u8 num)
{
    u8 code = 0;
    if(!(0 <= num && num <= 9))
    {
        return -1;
    }

    code = D_CODE2[num];

    digital_set_gpio(code);
    
}


/*---------------------------
digital_display_proc 
10 ms 调用一次 用于刷新数码管
-----------------------------*/
void digital_display_proc(void)
{
	static u8 pos = 0;
	u16 display_num = m_display_num;

	u8 ge = display_num%10;
	u8 shi = display_num/10;
    
    DIGITAL_SET_PIN_D0(0);
    DIGITAL_SET_PIN_D1(0);
    
	switch(pos)
	{
		case DGT_POS_D0:
		{
            DIGITAL_SET_PIN_D0(1);          
		    digital_show_one_num(ge);
		    pos = DGT_POS_D1;
		}
        break;
        case DGT_POS_D1:
        {            
            DIGITAL_SET_PIN_D1(1);
            digital_show_one_num(shi);
            pos = DGT_POS_D0;             
        }
        break;            
	}
    
}


int digital_set_num(u16 num)
{
    if(num > 99)
    {
        return -1;
    }

    m_display_num = num;
}





