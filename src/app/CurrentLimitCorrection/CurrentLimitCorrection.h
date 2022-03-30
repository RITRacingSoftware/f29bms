#ifndef CURRENT_LIMIT_CORRECTION_H
#define CURRENT_LIMIT_CORRECTION_H

#include <stdint.h>
#include "BatteryModel.h"

/**
 * Provides active current limit correction to limit the current to keep the voltage from
 * dropping below the minimum voltage. Lowers the current limit any time the voltage drops
 * below the threshold voltage
 */
 

void CurrentLimitCorrection_getCorrection(float *currentLimit,float *lowestVoltage, float *current);

#endif // CURRENT_LIMIT_CORRECTION_H