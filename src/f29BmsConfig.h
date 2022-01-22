#ifndef F29BMSCONFIG_H
#define F29BMSCONFIG_H

#include "FaultManager.h"

/**
 * Cells
 */

#define NUM_BOARDS 6
#define NUM_CHIPS (NUM_BOARDS*2)
#define NUM_SERIES_CELLS (NUM_BOARDS * 15)
#define NUM_PARALLEL_CELLS 3

// NOTE: Be sure to define things here with decimals if you intend to use them as floats

#define CELL_CAPACITY_Ah 6.950
#define BATTERY_CAPACITY_Ah (((float) NUM_PARALLEL_CELLS) * CELL_CAPACITY_Ah)


#define MAX_CELL_V 4.5 // any value above this is considered irrational

#define MAX_ALLOWED_CELL_V 4.3 // any cell above this is considered overcharged and balancing should begin
#define CHARGED_CELL_V 4.25 // once all cells are above this, charging ends

#define MIN_ALLOWED_CELL_V 3.1 // out of juice fault thrown if cells go below this
#define MIN_CELL_V 2.0 // cells below this are considered irrational

#define BALANCING_HISTERESIS_V .03 // how low a cell must be balanced below CHARGED_CELL_V for balancing to shut off
#define BALANCING_MEASURE_INTERVAL_S 30 // how long to wait in between measuring cell voltages when balancing

// Simply transmitted to the charger.
// These are NOT the overcurrent limits. See later in this document for those.
#define MAX_CHARGING_CURRENT_A 15
#define MAX_CHARGING_V ((float)MAX_ALLOWED_CELL_V * (float)NUM_SERIES_CELLS)
#define CHARGE_CURRENT_SETTLE_TIME_S 30

// The amount two cell voltages must differ by to be considered different
#define VOLTAGE_TOLERANCE 0.001

// maxmimum expected difference in cell voltages
#define MAX_CELL_DIFF_V 0.5
#define DIFF_CORRECTION_THRESHOLD_V 0.3

// how long a voltage fault condition must be present to trigger a fault
#define VOLTAGE_FAULT_HYSTERESIS_MS 500

/**
 * Current Monitoring
 */

#define CURRENT_IRRATIONAL_A 500
#define OVERCURRENT_A 230
#define OVERCURRENT_CHG_A -41
#define OVERCURRENT_HYST_MS 5
#define CURRENT_TOLERANCE 0.01
#define CURRENT_REJECTION_THRESHOLD_A 2.0
/**
 * Current Sensor
 */
#define CURRENT_SENSOR_ADC_CHANNEL ADC_CHSELR_CHSEL1
#define ADC_MAX_VALUE 4095.0
#define Vref 3.3
#define ZERO_AMP_ADC_CAL 2090.0
#define ZERO_AMP_REF_V ((ZERO_AMP_ADC_CAL/ADC_MAX_VALUE) * Vref)
#define V_PER_A 0.0036168

#define CURRENT_SENSOR_PORT GPIOA
#define CURRENT_SENSOR_PIN GPIO_Pin_1

/**
 * Faults
 */
// any bits set in this won't get set in the fault vector
//#define DISABLE_FAULT_MASK ((1 << FaultCode_SLAVE_COMM_TEMPS) | (1 << FaultCode_CELL_VOLTAGE_IRRATIONAL) | (1 << FaultCode_TEMPERATURE_IRRATIONAL) | (1 << FaultCode_OVER_TEMPERATURE))
#define DISABLE_FAULT_MASK ((1 << FaultCode_CELL_VOLTAGE_DIFF) | (1 << FaultCode_DRAIN_FAILURE))// | (1 << FaultCode_CELL_VOLTAGE_IRRATIONAL))

// Fault tolerances are the times a fault is allowed to be continuously active before triggering a shutdown event
// value a fault tolerance should have if the fault does not cause the car to shutdown
#define NO_SHUTDOWN -1

#define SLAVE_COMM_CELLS_TOLERANCE_MS 10000
#define SLAVE_COMM_TEMPS_TOLERANCE_MS 10000
#define SLAVE_COMM_DRAIN_REQUEST_TOLERANCE_MS 10000 // not implemented
#define CURRENT_SENSOR_COMM_TOLERANCE_MS 10 // not implemented?
#define OVER_CURRENT_TOLERANCE_MS 1
#define CELL_VOLTAGE_IRRATIONAL_TOLERANCE_MS 10000
#define CELL_VOLTAGE_DIFF_TOLERANCE_MS NO_SHUTDOWN
#define OUT_OF_JUICE_TOLERANCE_MS 1
#define OVER_TEMPERATURE_TOLERANCE_MS 1
#define TEMPERATURE_IRRATIONAL_TOLERANCE_MS 10000
#define DRAIN_FAILURE_TOLERANCE_MS NO_SHUTDOWN // not implemented

/**
 * SHRIYA TODO- add a #define here for manual fault tolerance. Should be 1 so it shuts down the car immediately
 */
 #define MANUAL_FAULT_TOl 1 

/**
 * MCU
 */
#define MCU_VCC 3.3 // not used

/**
 * SOC
 */
#define NUM_TEMP_RANGES 5

// columns: maximum SOC
// rows: average voltage of series cell
// example: row 0 col 0 means a voltage of 4.0V indicates the battery is at no more than 75% SOC when its <= 5 deg C
// each index is 0-10 deg C
static float temp_lut_V[NUM_TEMP_RANGES][3] = 
{//  75%  50%  25%
    {4.0, 3.8, 3.7}, //       t <= 5C
    {3.8, 3.7, 3.6}, //  5C < t <= 15C
    {3.8, 3.6, 3.5}, // 15C < t <= 25C
    {3.8, 3.5, 3.4}, // 25C < t <= 35C
    {3.5, 3.4, 3.3}  //       t > 35C
};

#define SOC_75_LIMIT_FOR_TEMP_INDEX(temp_index) temp_lut_V[temp_index][0]
#define SOC_50_LIMIT_FOR_TEMP_INDEX(temp_index) temp_lut_V[temp_index][1]
#define SOC_25_LIMIT_FOR_TEMP_INDEX(temp_index) temp_lut_V[temp_index][2]

// how often the SOC is written to flash
#define SOC_SAVE_PERIOD_MS 10000

/**
 * Temperature Monitoring
 */

// defined for 2 test segments rn
#define NUM_THERMISTOR (NUM_BOARDS*3)
#define LTC6804_ADC_MAX_VALUE ((float)(1 << 16))
#define LTC6804_ADC_RANGE_V (5.7)
#define THERM_INPUT_V (3.0)

#define MAX_TEMP_DEG_C 100.0
#define OVER_TEMP_DEG_C 60.0
#define MIN_TEMP_DEG_C -20.0

// how long a temperature fault condition must be present to trigger a fault
#define TEMPERATURE_FAULT_HYSTERESIS_MS 1000

#endif // F29BMSCONFIG_H