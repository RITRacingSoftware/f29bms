#include "HAL_Watchdog.h"
#include "BmsSim.pb.h"
#include "BmsSimClient.h"

void HAL_Watchdog_init(void)
{
    //lols
}

void HAL_Watchdog_pet(void)
{
    BmsOut* out = BmsSimClient_get_outputs();
    out->watchdog_pet = true;
}