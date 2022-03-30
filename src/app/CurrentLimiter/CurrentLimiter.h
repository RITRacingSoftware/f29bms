#ifndef CURRENT_LIMITER_H
#define CURRENT_LIMITER_H

#include "BatteryModel.h"

/**
 * Sets current limit to keep cells above lower voltage limit without just stopping driving.
 * Uses internal resistance to calculate maximum current allowed while staying above the
 * minimum allowed voltage
 */
 

void CurrentLimiter_10Hz(BatteryModel_t* bm); 

#endif // CURRENT_LIMITER_H