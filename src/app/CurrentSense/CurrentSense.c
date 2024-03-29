#include "CAN.h"
#include "CurrentSense.h"
#include "f29BmsConfig.h"
#include "HAL_CurrentSensor.h"
#include "FaultManager.h"
#include <stdio.h>
#include <math.h>

// kept track of in order to not filter on garbage memory
static bool first_reading_received;

// kept track of to prevent application code from using unfiltered current
static bool filter_valid;

// low pass filter history (only one value for now)
static float last_current_reading;

static bool did_last_read_error;

void filter_new_current_reading(float new_reading_A)
{
    if (first_reading_received)
    {
        // dummy simple low pass filter
        // y[n] = (x[n] + x[n-1]) * CURRENT_SENSE_LPF_COEFF
        last_current_reading = (last_current_reading + new_reading_A) * CURRENT_SENSE_LPF_COEFF;

        // data in last_current_reading is now filtered, safe to use
        filter_valid = true;
    }
    else
    {
        last_current_reading = new_reading_A;
        first_reading_received = true;
    }
}

bool current_valid(float new_reading_A)
{
    if (fabs(new_reading_A <= CURRENT_IRRATIONAL_A))
    {
        return true;
    }

    return false;
}

void CurrentSense_init(void)
{
    first_reading_received = false;
    filter_valid = false;
    did_last_read_error = false;
}

void CurrentSense_1kHz(void)
{
    // Get new current reading from ADC
    float raw_current_A;
    Error_t err = HAL_CurrentSensor_read_current(&raw_current_A);

    // Verify no errors accessing ADC
    if (err.active)
    {
        did_last_read_error = true;
        FaultManager_set_fault_active(FaultCode_CURRENT_SENSOR_COMM, err.data);
    }
    else
    {
        did_last_read_error = false;
        FaultManager_clear_fault(FaultCode_CURRENT_SENSOR_COMM);
        
        // this check is done here since we don't want to screw up the filter with huge currents
        // it wont recover from. There's no fault for irrational current yet.. maybe there should be?
        if (current_valid(raw_current_A))
        {
            if (fabs(raw_current_A) < CURRENT_REJECTION_THRESHOLD_A)
            {
                raw_current_A = 0;
            }

            filter_new_current_reading(raw_current_A);
        }

        can_bus.bms_current.bms_inst_current_filt = f29bms_dbc_bms_current_bms_inst_current_filt_encode(last_current_reading);
    }
}

bool CurrentSense_get_current(float* current_A)
{
    // save this off
    // keeps actions from here till return value consistent in case task is preempted...
    bool inst_current_valid = filter_valid && !did_last_read_error;

    if (inst_current_valid)
    {
        *current_A = last_current_reading;
    }
    
    return inst_current_valid;
}