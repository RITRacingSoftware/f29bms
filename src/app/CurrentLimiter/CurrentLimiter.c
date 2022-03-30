#include "CurrentLimiter.h"
#include "f29BmsConfig.h"
#include "CurrentLimitCalculation.h"
#include "CurrentLimitCorrection.h"
#include "CAN.h"
#include "CurrentSense.h"

double lastCurrentLimit = MAX_ALLOWED_CURRENT_LIMIT;

void CurrentLimiter_10Hz(BatteryModel_t* bm)
{
    float smallestVoltage = bm->smallest_V; //limit current based on lowest cell
    float current;
    //Only calculate limit if the current reading is valid
    if(CurrentSense_get_current(&current))
    {
        double currentLimit;
        float currentLimitCorrection;
        float currentLimitCalculated;
        //Get both current limits
        CurrentLimitCorrection_getCorrection(&currentLimitCorrection, &smallestVoltage, &current);
        CurrentLimitCalculation_getCalculated(&currentLimitCalculated, bm, &current);
        //Set the lower limit as the limit, or the previous limit if its lower
        if((currentLimitCalculated < currentLimitCorrection) && (currentLimitCalculated < lastCurrentLimit))
        {
            currentLimit = (double) currentLimitCalculated;
        }
        else if ((currentLimitCorrection < lastCurrentLimit) && (currentLimitCorrection < lastCurrentLimit))
        {
            currentLimit = (double) currentLimitCorrection;
        }
        else
        {
            currentLimit = lastCurrentLimit;
        }
        //Make sure current limit isn't above maximum allowed limit
        if(currentLimit > MAX_ALLOWED_CURRENT_LIMIT)
        {
            currentLimit = MAX_ALLOWED_CURRENT_LIMIT;
        }
        //Make sure current limit isn't below minimum allowed limit
        else if(currentLimit < MIN_ALLOWED_CURRENT_LIMIT)
        {
            currentLimit = MIN_ALLOWED_CURRENT_LIMIT;
        }
        lastCurrentLimit = currentLimit;
        //Set the new limit
        can_bus.bms_current_limit.bms_max_discharge_current = f29bms_dbc_bms_current_limit_bms_max_discharge_current_encode(currentLimit);   
        // can_bus.bms_current_limit.bms_max_discharge_current = f29bms_dbc_bms_current_limit_bms_max_discharge_current_encode(80); 
    }
}
