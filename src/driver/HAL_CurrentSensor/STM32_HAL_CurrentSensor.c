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
    GPIO_InitStructure.GPIO_Pin = CURRENT_SENSOR_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(CURRENT_SENSOR_GPIO_PORT, &GPIO_InitStructure);

    ADC_DeInit(ADC1);
    ADC_InitTypeDef adcinit;
    ADC_StructInit(&adcinit);
    // scan direction and external trig conv shouldn't matter bc we arent scanning and we arent using external triggers
    ADC_Init(ADC1, &adcinit);
    // Set sampling time
    ADC1->SMPR = 3;

    ADC_GetCalibrationFactor(ADC1);
  
    /* Enable the ADC peripheral */
    ADC_Cmd(ADC1, ENABLE);     
}

Error_t HAL_CurrentSensor_read_current(float* outCurrent)
{
    ADC1->CHSELR |= CURRENT_SENSOR_ADC_CHANNEL;
    ADC_StartOfConversion(ADC1);
    while(!(ADC1->ISR & ADC_ISR_EOC));
    uint16_t adc_read = ADC_GetConversionValue(ADC1);
    float voltage = ((float) adc_read / ADC_MAX_VALUE) * ADC_VREF * CURRENT_SENSOR_VOLTAGE_DIVIDER;
    float current = (voltage - CURRENT_SENSOR_OFFSET_V) / CURRENT_SENSOR_SENSITIVITY;
    *outCurrent = current;

    // errors disabled for now
    Error_t err;
    err.active = false;

    return err;
}