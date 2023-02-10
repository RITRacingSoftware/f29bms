#include "stm32f0xx.h"
#include "stm32f0xx_adc.h"
#include "HAL_CurrentSensor.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"
#include "f29BmsConfig.h"

void HAL_CurrentSensor_init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);


    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = CURRENT_SENSOR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(CURRENT_SENSOR_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_pin = LOW_CURRENT_SENSOR_PIN;


    ADC_DeInit(ADC1);
    ADC_InitTypeDef adcinit;
    ADC_StructInit(&adcinit);
    // scan direction and external trig conv shouldn't matter bc we arent scanning and we arent using external triggers
    ADC_Init(ADC1, &adcinit);

    ADC_GetCalibrationFactor(ADC1);
  
    /* Enable the ADC peripheral */
    ADC_Cmd(ADC1, ENABLE);     
}

//low sensor
//To do: contiue working with sensor and make sure that it runs to the correct file n stuff, 
Error_t HAL_Low_CurrentSensor_read_current(float* current)
{
    ADC1->CHSELR |= LOW_CURRENT_SENSOR_ADC_CHANNEL;
    ADC_StartOfConversion(ADC1);
    while(!(ADC1->ISR & ADC_ISR_EOC));
    uint16_t adc_read = ADC_GetConversionValue(ADC1);
    float unshifted_volts = ((float) adc_read / (float) ADC_MAX_VALUE) * (float) Vref;
    float shifted_volts = unshifted_volts - ZERO_AMP_REF_V;
    *current = shifted_volts / V_PER_A;    
}

//high sensor works as intended 
Error_t HAL_CurrentSensor_read_current(float* current)
{
    ADC1->CHSELR |= CURRENT_SENSOR_ADC_CHANNEL;
    ADC_StartOfConversion(ADC1);
    while(!(ADC1->ISR & ADC_ISR_EOC));
    uint16_t adc_read = ADC_GetConversionValue(ADC1);
    float unshifted_volts = ((float) adc_read / (float) ADC_MAX_VALUE) * (float) Vref;
    float shifted_volts = unshifted_volts - ZERO_AMP_REF_V;
    *current = shifted_volts / V_PER_A;
    if(*current <= 75)
    {
        HAL_Low_CurrentSensor_read_current(*current);
    }

    // errors disabled for now
    Error_t err;
    err.active = false;

    return err;
}