#include "stm8s.h"
#include "usr_adc.h"

static void adc_gpio_init(void)
{
    GPIO_Init(GPIOB,GPIO_PIN_0,GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(GPIOB,GPIO_PIN_1,GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(GPIOB,GPIO_PIN_2,GPIO_MODE_IN_FL_NO_IT);
    GPIO_Init(GPIOB,GPIO_PIN_3,GPIO_MODE_IN_FL_NO_IT);
    //GPIO_Init(GPIOD,GPIO_PIN_6,GPIO_MODE_IN_FL_NO_IT);
    //GPIO_Init(GPIOD,GPIO_PIN_5,GPIO_MODE_IN_FL_NO_IT);
}

void adc_init(void)
{
    u16 adc_value = 0;
    adc_gpio_init();
    
    ADC1_DeInit();    
    ADC1_PrescalerConfig(ADC1_PRESSEL_FCPU_D2); 
    ADC1_Cmd(ENABLE);  //ADC1_CR1_ADON first time wake up . second time start convert
}

u16 adc_get_value(ADC1_Channel_TypeDef ADC1_Channel)
{
    u16 adc_value = 0;

    ADC1_ConversionConfig(ADC1_CONVERSIONMODE_SINGLE,ADC1_Channel,ADC1_ALIGN_RIGHT);
    ADC1_ClearFlag(ADC1_FLAG_EOC);
    ADC1_StartConversion();
    while(RESET == ADC1_GetFlagStatus(ADC1_FLAG_EOC));
    
    adc_value = ADC1_GetConversionValue();    
    
    return adc_value;
}

