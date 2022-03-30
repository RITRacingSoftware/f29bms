#include "HAL_Gpio.h"
#include "common_macros.h"

#include <stdint.h>
#include "stm32f0xx_gpio.h"
#include "stm32f0xx.h"
#include <string.h>
#include <stdlib.h>

#define STATUS_LED_PIN_POS (2)
#define CHARGE_ENABLE_PIN_POS (8)  
#define CHARGE_AVAILABLE_PIN_POS (6) 
#define SHUTDOWN_LINE_PIN_POS (7)

/** @defgroup GPIO_pins_define 
  * @{
  */
#define GPIO_Pin_0 ((uint16_t)0x0001)   /*!< Pin 0 selected    */
#define GPIO_Pin_1 ((uint16_t)0x0002)   /*!< Pin 1 selected    */
#define GPIO_Pin_2 ((uint16_t)0x0004)   /*!< Pin 2 selected    */
#define GPIO_Pin_3 ((uint16_t)0x0008)   /*!< Pin 3 selected    */
#define GPIO_Pin_4 ((uint16_t)0x0010)   /*!< Pin 4 selected    */
#define GPIO_Pin_5 ((uint16_t)0x0020)   /*!< Pin 5 selected    */
#define GPIO_Pin_6 ((uint16_t)0x0040)   /*!< Pin 6 selected    */
#define GPIO_Pin_7 ((uint16_t)0x0080)   /*!< Pin 7 selected    */
#define GPIO_Pin_8 ((uint16_t)0x0100)   /*!< Pin 8 selected    */
#define GPIO_Pin_9 ((uint16_t)0x0200)   /*!< Pin 9 selected    */
#define GPIO_Pin_10 ((uint16_t)0x0400)  /*!< Pin 10 selected   */
#define GPIO_Pin_11 ((uint16_t)0x0800)  /*!< Pin 11 selected   */
#define GPIO_Pin_12 ((uint16_t)0x1000)  /*!< Pin 12 selected   */
#define GPIO_Pin_13 ((uint16_t)0x2000)  /*!< Pin 13 selected   */
#define GPIO_Pin_14 ((uint16_t)0x4000)  /*!< Pin 14 selected   */
#define GPIO_Pin_15 ((uint16_t)0x8000)  /*!< Pin 15 selected   */
#define GPIO_Pin_All ((uint16_t)0xFFFF) /*!< All pins selected */

// const GPIO_TypeDef STATUS_LED_GPIOx = GPIOA;
// const GPIO_TypeDef CHARGE_ENABLE_GPIOx = GPIOA;
// const GPIO_TypeDef CHARGE_AVAILABLE_GPIOx = GPIOA;

struct PIN_Typedef
{
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin_Pos;
};

static void get_GPIO_TypeDef(GpioPin_e thisPin, struct PIN_Typedef *thisGPIO)
{
    switch (thisPin)
    {
        case GpioPin_STATUS_LED:
            thisGPIO->GPIOx = GPIOA;
            thisGPIO->GPIO_Pin_Pos = STATUS_LED_PIN_POS;
            break;
        case GpioPin_CHARGE_ENABLE:
            thisGPIO->GPIOx = GPIOA;
            thisGPIO->GPIO_Pin_Pos = CHARGE_ENABLE_PIN_POS;
            break;
        case GpioPin_CHARGER_AVAILABLE:
            thisGPIO->GPIOx = GPIOA;
            thisGPIO->GPIO_Pin_Pos = CHARGE_AVAILABLE_PIN_POS;
            break;
        case GpioPin_SHUTDOWN_LINE:
            thisGPIO->GPIOx = GPIOA;
            thisGPIO->GPIO_Pin_Pos = SHUTDOWN_LINE_PIN_POS;
            break;
    }
}

static void populate_Init_Struct(GPIO_InitTypeDef *thisInit, uint32_t pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed, GPIOOType_TypeDef Otype, GPIOPuPd_TypeDef pupd)
{
    thisInit->GPIO_Pin = pin;
    thisInit->GPIO_Mode = mode;
    thisInit->GPIO_Speed = speed;
    thisInit->GPIO_OType = Otype;
    thisInit->GPIO_PuPd = pupd;
}

void HAL_Gpio_init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    GPIO_InitTypeDef gpioInit;
    //Init STATUS_LED pin
    populate_Init_Struct(&gpioInit, BIT(STATUS_LED_PIN_POS), GPIO_Mode_OUT, GPIO_Speed_Level_1, GPIO_OType_PP, GPIO_PuPd_NOPULL); //Not sure about speed, otype and pupd
    GPIO_Init(GPIOA, &gpioInit);
    //Init CHARGE_ENABLE pin
    populate_Init_Struct(&gpioInit, BIT(CHARGE_ENABLE_PIN_POS), GPIO_Mode_OUT, GPIO_Speed_Level_1, GPIO_OType_PP, GPIO_PuPd_NOPULL); //Not sure about speed, otype, and pupd
    GPIO_Init(GPIOA, &gpioInit);
    //Init CHARGE_AVAILABLE
    populate_Init_Struct(&gpioInit, BIT(CHARGE_AVAILABLE_PIN_POS), GPIO_Mode_IN, GPIO_Speed_Level_1, GPIO_OType_PP, GPIO_PuPd_NOPULL);
    GPIO_Init(GPIOA, &gpioInit);

    populate_Init_Struct(&gpioInit, BIT(SHUTDOWN_LINE_PIN_POS), GPIO_Mode_OUT, GPIO_Speed_Level_1, GPIO_OType_PP, GPIO_PuPd_NOPULL);
    GPIO_Init(GPIOA, &gpioInit);

}

bool HAL_Gpio_read(GpioPin_e pin)
{
    struct PIN_Typedef thisGPIO;
    get_GPIO_TypeDef(pin, &thisGPIO);
    return GPIO_ReadInputDataBit(thisGPIO.GPIOx, BIT(thisGPIO.GPIO_Pin_Pos));
}

bool HAL_Gpio_read_output(GpioPin_e pin)
{
    struct PIN_Typedef thisGPIO;
    get_GPIO_TypeDef(pin, &thisGPIO);
    return GPIO_ReadOutputDataBit(thisGPIO.GPIOx, BIT(thisGPIO.GPIO_Pin_Pos));
}

void HAL_Gpio_write(GpioPin_e pin, bool state)
{
    struct PIN_Typedef thisGPIO;
    get_GPIO_TypeDef(pin, &thisGPIO);
    if (state)
    {
        GPIO_SetBits(thisGPIO.GPIOx, BIT(thisGPIO.GPIO_Pin_Pos));
    }
    else
    {
        GPIO_ResetBits(thisGPIO.GPIOx, BIT(thisGPIO.GPIO_Pin_Pos));
    }
}
