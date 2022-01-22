#include <stdint.h>
#include <limits.h>

#include "counters.h"

#include "f29BmsConfig.h"
#include "DriveMonitor.h"
#include "FaultManager.h"

#include "HAL_Gpio.h"

/**
 * How many milliseconds each fault type is allowed to be active before triggering a shutdown.
 * 
 * This could be set statically, but its nice to define things at runtime in case FaultCode_e changes.
 */
static int fault_tolerances[FaultCode_NUM];

/**
 * How long each fault has been active in milliseconds.
 */
static int active_fault_timers[FaultCode_NUM];

/**
 * Keeps track of the last decision of DriveMonitor_1kHz of whether to shutdown or not.
 */
static bool is_driving_allowed;

static void indicate_fault(void)
{
    HAL_Gpio_write(GpioPin_SHUTDOWN_LINE, false);
}

static void indicate_nominal(void)
{
    HAL_Gpio_write(GpioPin_SHUTDOWN_LINE, true);
}

void DriveMonitor_init(void)
{
    // Default tolerances to safe value (0 for no tolerance).
    // Start timers at 0
    for (int i = 0; i < FaultCode_NUM; i++)
    {
        fault_tolerances[i] = 0;
        active_fault_timers[i] = 0;
    }

    // Fault tolerance defition is here, in milliseconds.
    // Any faults not defined here get 0, which means car will not drive until the
    // devloper who added the fault code but didn't add a tolerance gets their act together. 
    // Use 1 for faults that trigger shutdown if they're active at all.
    fault_tolerances[FaultCode_SLAVE_COMM_CELLS]            = SLAVE_COMM_CELLS_TOLERANCE_MS;
    fault_tolerances[FaultCode_SLAVE_COMM_TEMPS]            = SLAVE_COMM_TEMPS_TOLERANCE_MS;
    fault_tolerances[FaultCode_SLAVE_COMM_DRAIN_REQUEST]    = SLAVE_COMM_DRAIN_REQUEST_TOLERANCE_MS;
    fault_tolerances[FaultCode_CURRENT_SENSOR_COMM]         = CURRENT_SENSOR_COMM_TOLERANCE_MS;
    fault_tolerances[FaultCode_OVER_CURRENT]                = OVER_CURRENT_TOLERANCE_MS;
    fault_tolerances[FaultCode_CELL_VOLTAGE_IRRATIONAL]     = CELL_VOLTAGE_IRRATIONAL_TOLERANCE_MS;
    fault_tolerances[FaultCode_CELL_VOLTAGE_DIFF]           = CELL_VOLTAGE_DIFF_TOLERANCE_MS;
    fault_tolerances[FaultCode_OUT_OF_JUICE]                = OUT_OF_JUICE_TOLERANCE_MS;
    fault_tolerances[FaultCode_OVER_TEMPERATURE]            = OVER_TEMPERATURE_TOLERANCE_MS;
    fault_tolerances[FaultCode_TEMPERATURE_IRRATIONAL]      = TEMPERATURE_IRRATIONAL_TOLERANCE_MS;
    fault_tolerances[FaultCode_DRAIN_FAILURE]               = DRAIN_FAILURE_TOLERANCE_MS;

    /**
     * SHRIYA TODO- add entry to array and tolerance for manual fault
     */

    // ideally this would start low, but the FSAE rules say faults must latch
    // at the hardware level so we have to start with the true or the car will never drive
    is_driving_allowed = true;
    // make the shutdown line agree with this
    indicate_nominal();
}

/**
 * Update active fault timers, shut down car if any expire.
 */
void DriveMonitor_1kHz(void)
{
    // driving is now allowed, if no unsafe conditions detected below
    bool driving_allowed = true;

    // check to see if any faults have been active for too long
    for (int code = 0; code < FaultCode_NUM; code++)
    {   
        // if the fault is active or there is no tolerance for it
        if (FaultManager_is_fault_active((FaultCode_e) code) || (fault_tolerances[code] == 0))
        {
            if (fault_tolerances[code] != NO_SHUTDOWN)
            {
                // fault is active, increment counter + check for expiration
                if (incr_to_limit(&active_fault_timers[code], fault_tolerances[code], 1))
                {
                    driving_allowed = false;
                }
            }
        }
        else
        {
            // fault is not active, reset counter
            active_fault_timers[code] = 0;
        }
    }

    if (driving_allowed)
    {
        // no timers are expired
        // therefore an unsafe driver condition is not present
        indicate_nominal();
    }
    else
    {
        // unsafe driver condition is present
        indicate_fault();
    }

    is_driving_allowed = driving_allowed;
}
/**
 * Return false if the shutdown line is active, true otherwise.
 */
bool DriveMonitor_is_driving_allowed(void)
{
    return is_driving_allowed;
}