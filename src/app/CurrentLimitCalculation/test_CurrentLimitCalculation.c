#include "unity.h"
#include "f29BmsConfig.h"

// module under test
#include "CurrentLimitCalculation.h"


/** 
 * Test to make sure that if the voltage drops with the same instantaneous current, the 
 * current limit will decrease
 */
void test_CurrentLimitCalculation_voltage_drop(void)
{
    float current_limit_1;
    float current_limit_2;

    float current = 150;
    BatteryModel_t bm;
    for(int i = 0; i < NUM_SERIES_CELLS; i++)
    {
        bm.cells[i].voltage = CHARGED_CELL_V;
    }
    bm.cells[NUM_SERIES_CELLS-1].voltage = MIN_ALLOWED_CELL_V+0.9;
    bm.smallest_V = MIN_ALLOWED_CELL_V+0.9;

    CurrentLimitCalculation_getCalculated(&current_limit_1, &bm, &current);

    current = 149;
    bm.smallest_V = MIN_ALLOWED_CELL_V+0.6;
    bm.cells[NUM_SERIES_CELLS-1].voltage = MIN_ALLOWED_CELL_V+0.6;

    CurrentLimitCalculation_getCalculated(&current_limit_2, &bm, &current);

    char err_msg[150];
    sprintf(err_msg, "Current limit calculation current limit didn't drop with a drop in voltage.(%fA vs %fA)", current_limit_1, current_limit_2);
    TEST_ASSERT_MESSAGE(current_limit_1 > current_limit_2, err_msg);
}

/**
 * Test to make sure that if the instantaneous current drops and the voltage remains the same, the current
 * limit will decrease
 */
void test_CurrentLimitCalculation_current_drop(void)
{
    float current_limit_1;
    float current_limit_2;

    float current = 100;
    BatteryModel_t bm;
    for(int i = 0; i < NUM_SERIES_CELLS; i++)
    {
        bm.cells[i].voltage = CHARGED_CELL_V;
    }
    bm.cells[NUM_SERIES_CELLS-1].voltage = MIN_ALLOWED_CELL_V+0.4;
    bm.smallest_V = MIN_ALLOWED_CELL_V+0.4;

    CurrentLimitCalculation_getCalculated(&current_limit_1, &bm, &current);

    current = 75;

    CurrentLimitCalculation_getCalculated(&current_limit_2, &bm, &current);

    char err_msg[150];
    sprintf(err_msg, "Current limit calculation current limit didn't drop with a drop in current.(%fA vs %fA)", current_limit_1, current_limit_2);
    TEST_ASSERT_MESSAGE(current_limit_1 > current_limit_2, err_msg);
}

/**
 * Test to make sure that if the instantaneous current and the voltage remain the same, the current
 * limit will stay the same
 */
void test_CurrentLimitCalculation_no_change(void)
{
    float current_limit_1;
    float current_limit_2;

    float current = 50;
    BatteryModel_t bm;
    for(int i = 0; i < NUM_SERIES_CELLS; i++)
    {
        bm.cells[i].voltage = CHARGED_CELL_V;
    }
    bm.cells[NUM_SERIES_CELLS-1].voltage = MIN_ALLOWED_CELL_V+0.2;
    bm.smallest_V = MIN_ALLOWED_CELL_V+0.2;

    CurrentLimitCalculation_getCalculated(&current_limit_1, &bm, &current);

    CurrentLimitCalculation_getCalculated(&current_limit_2, &bm, &current);

    char err_msg[150];
    sprintf(err_msg, "Current limit calculation current limit changed when current and voltage stayed the same.(%fA vs %fA)", current_limit_1, current_limit_2);
    TEST_ASSERT_MESSAGE(current_limit_1 == current_limit_2, err_msg);
}

/**
 * Test to make sure that if the instantaneoud current is 0, the current limit is set to 
 * (current voltage-min_voltage)/internal resistance
 * Need internal resistance to calculate expected result
 */
// void test_CurrentLimitCalculation_no_current(void)
// {
//     float current_limit;

//     float current = 0;
//     BatteryModel_t bm;
//     for(int i = 0; i < NUM_SERIES_CELLS; i++)
//     {
//         bm.cells[i].voltage = CHARGED_CELL_V;
//     }
//     bm.cells[NUM_SERIES_CELLS-1].voltage = CHARGED_CELL_V-0.5;
//     bm.smallest_V = CHARGED_CELL_V-0.5;

//     CurrentLimitCalculation_getCalculated(&current_limit, &bm, &current);

//     float expected_current_limit = (CHARGED_CELL_V-0.5-MIN_ALLOWED_CELL_V)/internal_resistances[NUM_SERIES_CELLS-1];

//     TEST_ASSERT_MESSAGE(current_limit == expected_current_limit, "Current limit calculation current limit incorrect when current = 0.");

// }
