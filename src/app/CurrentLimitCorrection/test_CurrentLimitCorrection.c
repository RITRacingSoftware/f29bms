#include "unity.h"
#include "f29BmsConfig.h"
#include "CAN.h"
#include "MockCAN.h"

// module under test
#include "CurrentLimitCorrection.h"

CAN_BUS can_bus;
/**
 * Test to make sure the current isn't limited above the threshold voltage
 */
void test_CurrentLimitCorrection_not_corrected(void)
{
    float current = 100;
    float current_limit;
    BatteryModel_t bm;
    bm.smallest_V = MAX_ALLOWED_CELL_V;
    
    // set most cells the same, only one with a massive difference
    for (float v = MAX_ALLOWED_CELL_V; v > CURRENT_LIMIT_THRESHOLD_V; v = v - 0.1)
    {
        bm.smallest_V = v;
        CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    }
    char err_msg[150];
    sprintf(err_msg, "Current limit correction limited current before threshold voltage reached.(%fA)", current_limit);
    TEST_ASSERT_MESSAGE(current_limit == CURRENT_IRRATIONAL_A, err_msg);
}

/**
 * Test to make sure the current is limited at the threshold voltage
 */
void test_CurrentLimitCorrection_triggered(void)
{
    float current = 100;
    float current_limit;
    BatteryModel_t bm;
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V-0.1;
    CAN_send_message_Expect(F29BMS_DBC_ACTIVE_CORRECTION_CURRENT_LIMIT_ALERT_FRAME_ID);
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    char err_msg[150];
    sprintf(err_msg, "Current limit correction didn't correct current limit when under threshold voltage.(%fA)", current_limit);
    TEST_ASSERT_MESSAGE(current_limit == (100-ACTIVE_CURRENT_CORRECTION_DECREASE_VALUE), err_msg);
}

/**
 * Test to make sure the current isn't limited multiple times without the voltage returning above the threshold voltage
 * but is after the current goes above and then back below the threshold voltage
 */
void test_CurrentLimitCorrection_triggered_once(void)
{
    float current = 100;
    float current_limit = 125;
    BatteryModel_t bm;
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V+0.1;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V-0.1;
    CAN_send_message_Expect(F29BMS_DBC_ACTIVE_CORRECTION_CURRENT_LIMIT_ALERT_FRAME_ID);
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    current = 80;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    current = 70;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V+0.1;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    char err_msg[150];
    sprintf(err_msg, "Current limit correction decreased limit multiple times without the voltage returning above threshold.(%fA)", current_limit);
    TEST_ASSERT_MESSAGE(current_limit == (100-ACTIVE_CURRENT_CORRECTION_DECREASE_VALUE), err_msg);
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V-0.1;
    current = 60;
    CAN_send_message_Expect(F29BMS_DBC_ACTIVE_CORRECTION_CURRENT_LIMIT_ALERT_FRAME_ID);
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    sprintf(err_msg, "Current limit correction didn't decreased after the voltage returned above threshold and then fell below.(%fA)", current_limit);
    TEST_ASSERT_MESSAGE(current_limit == (60-ACTIVE_CURRENT_CORRECTION_DECREASE_VALUE), err_msg);
}

/**
 * Test to make sure the current isn't limited multiple times without the voltage returning above the threshold voltage
 * but is after the current goes above and then back below the threshold voltage
 */
void test_CurrentLimitCorrection_hysteresis(void)
{
    float current = 100;
    float current_limit = 125;
    BatteryModel_t bm;
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V+0.1;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V-0.1;
    CAN_send_message_Expect(F29BMS_DBC_ACTIVE_CORRECTION_CURRENT_LIMIT_ALERT_FRAME_ID);
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    current = 80;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    current = 70;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V+0.1;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    char err_msg[150];
    sprintf(err_msg, "Current limit correction decreased limit multiple times without the voltage returning above threshold.(%fA)", current_limit);
    TEST_ASSERT_MESSAGE(current_limit == (100-ACTIVE_CURRENT_CORRECTION_DECREASE_VALUE), err_msg);
    bm.smallest_V = CURRENT_LIMIT_THRESHOLD_V-0.1;
    CAN_send_message_Expect(F29BMS_DBC_ACTIVE_CORRECTION_CURRENT_LIMIT_ALERT_FRAME_ID);
    current = 60;
    CurrentLimitCorrection_getCorrection(&current_limit, &bm.smallest_V, &current);
    sprintf(err_msg, "Current limit correction didn't decreased after the voltage returned above threshold and then fell below.(%fA)", current_limit);
    TEST_ASSERT_MESSAGE(current_limit == (60-ACTIVE_CURRENT_CORRECTION_DECREASE_VALUE), err_msg);
}
