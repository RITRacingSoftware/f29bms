#include "HAL_EEPROM.h"
#include "stm32f0xx_flash.h"

//Temporary fix to get it to build
#include "FreeRTOS.h"
#include "semphr.h"
SemaphoreHandle_t can_message_recieved_semaphore;
SemaphoreHandle_t can_message_transmit_semaphore;

#define CONFIG_FLASH_PAGE 0x8040000
FLASH_Status write_boot_byte(uint8_t* addr, uint8_t byte_to_write)
{
  FLASH_Status status = FLASH_COMPLETE; 

  /* Check the parameters */

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_ER_PRG_TIMEOUT);
  
  if(status == FLASH_COMPLETE)
  {  
    /* Enable the Option Bytes Programming operation */
    FLASH->CR |= FLASH_CR_OPTPG;

    //OB->USER = OB_BOOT0SW | 0x7F;
    *addr = byte_to_write;
  
    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_ER_PRG_TIMEOUT);

    if(status != FLASH_TIMEOUT)
    {
      /* If the program operation is completed, disable the OPTPG Bit */
      FLASH->CR &= ~FLASH_CR_OPTPG;
    }
  }
  /* Return the Option Byte program Status */
  return status;
}
#include "HAL_Can.h"
#include "stm32f0xx_rcc.h"
#include "stm32f0xx.h"
int main()
{
  FLASH->ACR &= ~(0x3);
  FLASH->ACR |= 1;
//  EXTI->IMR = 0;
// EXTI->EMR = 0;
// EXTI->SWIER = 0;
   // enable HSI48 clock
    RCC_HSI48Cmd(ENABLE);
    RCC->CIR &= ~RCC_CIR_HSI48RDYIE;
    // wait for HSI48 clock to be ready
    while(!RCC_GetFlagStatus(RCC_FLAG_HSI48RDY));
    // tell system clock (which CAN uses) to use HSI48 clock
    // uint32_t tmpreg = 0;

    /* Check the parameters */
    // assert_param(IS_RCC_SYSCLK_SOURCE(RCC_SYSCLKSource));

    // tmpreg = RCC->CFGR;

    /* Clear SW[1:0] bits */
    RCC->CFGR &= ~RCC_CFGR_SW;

    /* Set SW[1:0] bits according to RCC_SYSCLKSource value */
    RCC->CFGR |= 0x3;
  
  /* Store the new value */

    // wait for system clock to switch over to HSI48
    while(RCC_GetSYSCLKSource() != 0xc);
    for(;;);
    // bms_eeprom_t map;
    // map.saved_soc_en = 0;
    // map.saved_soc = 0;
    // HAL_EEPROM_write(&map);
    // HAL_EEPROM_read(&map);
    // FLASH_Unlock();
    // FLASH_OB_Unlock();
    // FLASH_Status erase_status = FLASH_OB_Erase();
    // FLASH_Unlock();
    // FLASH_OB_Unlock();
    //write_boot_byte((uint8_t*)0x1ffff800, 0xAA);
    //write_boot_byte((uint8_t*)0x1ffff801, 0x55);
    // write_boot_byte((uint8_t*)0x1ffff802, 0xFF);
    // write_boot_byte((uint8_t*)0x1ffff803, 0x00);

    // write_boot_byte((uint8_t*)0x1ffff804, 0xff);
    // write_boot_byte((uint8_t*)0x1ffff805, 0x00);
    // write_boot_byte((uint8_t*)0x1ffff806, 0xff);
    // write_boot_byte((uint8_t*)0x1ffff807, 0x00);

    // write_boot_byte((uint8_t*)0x1ffff808, 0xff);
    // write_boot_byte((uint8_t*)0x1ffff809, 0x00);
    // write_boot_byte((uint8_t*)0x1ffff80A, 0xff);
    // write_boot_byte((uint8_t*)0x1ffff80B, 0x00);

    // write_boot_byte((uint8_t*)0x1ffff80C, 0xff);
    // write_boot_byte((uint8_t*)0x1ffff80D, 0x00);
    // write_boot_byte((uint8_t*)0x1ffff80E, 0xff);
    // write_boot_byte((uint8_t*)0x1ffff80F, 0x00);
    
}