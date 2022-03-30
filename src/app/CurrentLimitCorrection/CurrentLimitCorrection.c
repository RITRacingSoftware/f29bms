#include "CurrentLimitCorrection.h"
#include "f29BmsConfig.h"
#include "CAN.h"
#include <stdio.h>

float lastCurrentLimitCorrection = CURRENT_IRRATIONAL_A;
bool lastVoltageLow = false; //True if the current limit was already decreased, used for hysteresis
uint16_t numTimesLimited = 0;

void CurrentLimitCorrection_getCorrection(float *currentLimit, float *lowestVoltage, float *current)
{
    //Check if voltage is below threshold and reduce current limit
    if((*lowestVoltage < CURRENT_LIMIT_THRESHOLD_V) && (*lowestVoltage > MIN_CELL_V))
    {
        //Don't decrease current limit again if it has already been decresed and the voltage hasn't increased above the threshold voltage
        if(!lastVoltageLow)
        {
            *currentLimit = *current - ACTIVE_CURRENT_CORRECTION_DECREASE_VALUE;
            lastCurrentLimitCorrection = *currentLimit;
            lastVoltageLow = true;
            numTimesLimited++;
            //Send can message alert
            can_bus.bms_current_limit_correction_count.count_active_correction_count++;
            CAN_send_message(F29BMS_DBC_ACTIVE_CORRECTION_CURRENT_LIMIT_ALERT_FRAME_ID);
        }
        else
        {
            *currentLimit = lastCurrentLimitCorrection;
        }    
    }
    else
    {
        *currentLimit = lastCurrentLimitCorrection;
        lastVoltageLow = false;
    }
}
