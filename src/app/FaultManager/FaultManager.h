#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include <stdbool.h>

/**
 * Handles setting and clearing faults. 
 * Additionally transmits CAN alerts on setting faults and provides information for CAN fault vectors. 
 * Fault statuses are stored as bits (1 for active, 0 for inactive) in a 64 bit value called the 'fault vector'.
 */

#define ERROR_DATA_LEN 7

// Error type used in intermediate indexes, assessed by higher level code for fault generation.
typedef struct
{
    bool active;
    char data[ERROR_DATA_LEN];
} Error_t;

// Different fault types
// these must be in the same order on the DBC (without NO_FAULT and FaultCode_NUM)
// any addition to this must be reflected in the DBC
typedef enum
{
    // Problem communicating with slave boards for voltages/drain states
    FaultCode_SLAVE_COMM_CELLS,
    // Problem communicating with slave boards for thermistor readings
    FaultCode_SLAVE_COMM_TEMPS,
    // Problem communicating with slave boards when requesting new drain states
    FaultCode_SLAVE_COMM_DRAIN_REQUEST,
    // Current sensor HAL indicated error reading current
    FaultCode_CURRENT_SENSOR_COMM,
    // Current above overcurrent threshold for too long
    FaultCode_OVER_CURRENT,
    // A cell voltage is reading of reasonable physical limits
    FaultCode_CELL_VOLTAGE_IRRATIONAL,
    // The maximum cell voltage difference is higher than expected.
    FaultCode_CELL_VOLTAGE_DIFF,
    // It is unsafe to discharge the cells any further.
    FaultCode_OUT_OF_JUICE,
    // An irrational temperature has been reported.
    FaultCode_TEMPERATURE_IRRATIONAL,
    // A temperature over the allowable limit has been reported.
    FaultCode_OVER_TEMPERATURE,
    // Draining was requested for a cell and was not observed.
    FaultCode_DRAIN_FAILURE,

    /**
     * SHRIYA TODO- add a new fault here called FaultCode_MANUAL
     */

    //Manual Fault 
    FaultCode_MANUAL,

    // Number of enumerations
    FaultCode_NUM
} FaultCode_e;

/**
 * Start with a clean fault vector.
 */
void FaultManager_init(void);

/**
 * Record a fault as active, and transmit a CAN alert that contains the fault
 * code and associated data.
 * code [in] - type of fault to set to active
 * data [in] - points to relevant data for fault to be sent over CAN
 */
void FaultManager_set_fault_active(FaultCode_e code, void* data);

/**
 * Record a fault as not active in the fault vector.
 */
void FaultManager_clear_fault(FaultCode_e code);

/**
 * Check if a fault is active. Return true if active, false otherwise.
 * code [in] - fault code to check if active
 */
bool FaultManager_is_fault_active(FaultCode_e code);

/**
 * Return true if any faults are active, false otherwise.
 */
bool FaultManager_is_any_fault_active(void);

bool FaultManager_is_fault_enabled(FaultCode_e code);


#endif // FAULT_MANAGER_H