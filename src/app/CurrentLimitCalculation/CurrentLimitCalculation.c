#include "CurrentLimitCalculation.h"
#include "f29BmsConfig.h"
#include <stdio.h>

const float internal_resistances[NUM_SERIES_CELLS] = {.001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001,
                        .001, .001, .001, .001, .001, .001, .001, .001, .001, .001};
float lastLowestVoltage = MAX_CELL_V;
float lastCurrent = MAX_ALLOWED_CURRENT_LIMIT;
float lastCurrentLimitCalc = MAX_ALLOWED_CURRENT_LIMIT;

void CurrentLimitCalculation_getCalculated(float *currentLimit, BatteryModel_t* bm, float *current)
{
    float lowestVoltage = bm->smallest_V;
    // if((*current != lastCurrent) && (lowestVoltage != lastLowestVoltage))
    // {
        //Find the cell index of lowest cell
        uint8_t lowestCell = 0; //cell index of lowest cell
        for(uint8_t i = 0; i < NUM_SERIES_CELLS;i++)
        {
            if(lowestVoltage == bm->cells[i].voltage)
            {
                lowestCell = i;
                break;
            }
        }
        //Calculate current limit
        *currentLimit = ((lowestVoltage - CURRENT_LIMIT_THRESHOLD_V) / internal_resistances[lowestCell]) + *current;
        lastCurrentLimitCalc = *currentLimit;
    // }
    // else
    // {
    //     *currentLimit = lastCurrentLimitCalc;
    // }
    lastLowestVoltage = lowestVoltage;
    lastCurrent = *current;
}