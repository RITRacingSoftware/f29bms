#ifndef CURRENT_LIMIT_CALCULATION_H
#define CURRENT_LIMIT_CALCULATION_H

#include <stdint.h>
#include "BatteryModel.h"


/**
 * Caclulates the max current limit that keeps the lowest cell voltage below the minimum
 * voltage based on internal resistance.
 */
 
void CurrentLimitCalculation_getCalculated(float *currentLimit, BatteryModel_t* bm, float *current);

#endif // CURRENT_LIMIT_CALCULATION_H