#include "unity.h"
#include "f29BmsConfig.h"

// module under test
#include "CurrentLimiter.h"

#include "MockCurrentLimitCorrection.h"
#include "MockCurrentLimitCalculation.h"
#include "MockCAN.h"
#include "MockCurrentSense.h"

// need to define this here since CMock isn't smart enough to define it in MockCAN.h
CAN_BUS can_bus;

/**
 * Test to make sure that the lower limit between the correction and calculated limit is used as the limit and
 * doesn't go below the minimum allowed current limit
 */
void test_CurrentLimiter_selectMinCurrent(void)
{
    BatteryModel_t bm;
    bm.smallest_V = CHARGED_CELL_V; //Shouldn't really matter, only used to pass to calculation and correction functions 

    can_bus.bms_current_limit.bms_max_discharge_current = f29bms_dbc_bms_current_limit_bms_max_discharge_current_encode(MIN_ALLOWED_CURRENT_LIMIT+10);
    
    
    
    //Make sure limit doesn't go above the max allowed current limit
    float current_calculated = MAX_ALLOWED_CURRENT_LIMIT + 5;
    float current_correction = MAX_ALLOWED_CURRENT_LIMIT + 5;
    CurrentLimitCalculation_getCalculated_ExpectAnyArgs();
    CurrentLimitCorrection_getCorrection_ExpectAnyArgs();
    CurrentLimitCalculation_getCalculated_ReturnThruPtr_currentLimit(&current_calculated);
    CurrentLimitCorrection_getCorrection_ReturnThruPtr_currentLimit(&current_correction);
    CurrentSense_get_current_ExpectAnyArgsAndReturn(true);
    CurrentLimiter_10Hz(&bm);
    char err_msg[150];
    sprintf(err_msg, "Current limit set above maximum allowed current limit.(%fA)", f29bms_dbc_bms_current_limit_bms_max_discharge_current_decode(can_bus.bms_current_limit.bms_max_discharge_current));
    TEST_ASSERT_MESSAGE(f29bms_dbc_bms_current_limit_bms_max_discharge_current_decode(can_bus.bms_current_limit.bms_max_discharge_current) == MAX_ALLOWED_CURRENT_LIMIT, err_msg);
    
    
    current_correction = MIN_ALLOWED_CURRENT_LIMIT + 100;
    current_calculated = MIN_ALLOWED_CURRENT_LIMIT + 50; 
    // Set return for calculated current limit below return for correction limit
    CurrentLimitCalculation_getCalculated_ExpectAnyArgs();
    CurrentLimitCorrection_getCorrection_ExpectAnyArgs();
    CurrentLimitCalculation_getCalculated_ReturnThruPtr_currentLimit(&current_calculated);
    CurrentLimitCorrection_getCorrection_ReturnThruPtr_currentLimit(&current_correction);
    CurrentSense_get_current_ExpectAnyArgsAndReturn(true);

    //Make sure current limit set to the calculated current
    CurrentLimiter_10Hz(&bm);
    TEST_ASSERT_MESSAGE(f29bms_dbc_bms_current_limit_bms_max_discharge_current_decode(can_bus.bms_current_limit.bms_max_discharge_current) == MIN_ALLOWED_CURRENT_LIMIT + 50, "Current limit not set to lower limit value (calculated).");

    //Change correction to lowet
    current_correction = MIN_ALLOWED_CURRENT_LIMIT + 25;
    CurrentLimitCalculation_getCalculated_ExpectAnyArgs();
    CurrentLimitCorrection_getCorrection_ExpectAnyArgs();
    CurrentLimitCalculation_getCalculated_ReturnThruPtr_currentLimit(&current_calculated);
    CurrentLimitCorrection_getCorrection_ReturnThruPtr_currentLimit(&current_correction);
    CurrentSense_get_current_ExpectAnyArgsAndReturn(true);

    //Make sure current limit set to the corrected current
    CurrentLimiter_10Hz(&bm);
    TEST_ASSERT_MESSAGE(f29bms_dbc_bms_current_limit_bms_max_discharge_current_decode(can_bus.bms_current_limit.bms_max_discharge_current) == MIN_ALLOWED_CURRENT_LIMIT + 25, "Current limit not set to lower limit value (correction).");
    
    //Make sure limit doesn't go below the minimum allowed current limit
    current_calculated = MIN_ALLOWED_CURRENT_LIMIT - 5;
    CurrentLimitCalculation_getCalculated_ExpectAnyArgs();
    CurrentLimitCorrection_getCorrection_ExpectAnyArgs();
    CurrentLimitCalculation_getCalculated_ReturnThruPtr_currentLimit(&current_calculated);
    CurrentLimitCorrection_getCorrection_ReturnThruPtr_currentLimit(&current_correction);
    CurrentSense_get_current_ExpectAnyArgsAndReturn(true);

    CurrentLimiter_10Hz(&bm);
    TEST_ASSERT_MESSAGE(f29bms_dbc_bms_current_limit_bms_max_discharge_current_decode(can_bus.bms_current_limit.bms_max_discharge_current) == MIN_ALLOWED_CURRENT_LIMIT, "Current limit set below minimum allowed current limit.");
}
