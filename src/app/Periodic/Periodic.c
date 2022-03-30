#include "Periodic.h"

#include "f29BmsConfig.h"

#include "BatteryModel.h"
#include "CAN.h"
#include "CellBalancer.h"
#include "ChargeMonitor.h"
#include "CurrentMonitor.h"
#include "CurrentSense.h"
#include "DriveMonitor.h"
#include "PackMonitor.h"
#include "SlaveInterface.h"
#include "SOCestimator.h"
#include "StatusLed.h"
#include "TempConverter.h"
#include "TempModel.h"
#include "HAL_Watchdog.h"
#include "CurrentLimiter.h"

#include <stdio.h>
#include "HAL_Uart.h"

// the global battery model!
static BatteryModel_t battery_model;
// the global temperature model!
static TempModel_t temp_model;

void Periodic_init(void)
{
    for (int i = 0; i < NUM_SERIES_CELLS; i++)
    {
        // start with a cell voltage that shouldn't cause any errors
        battery_model.cells[i].voltage = MAX_ALLOWED_CELL_V;
        // nothing should be draining on startup
        battery_model.cells[i].is_draining = false;
    }
    battery_model.average_V = MAX_ALLOWED_CELL_V;
    battery_model.largest_V = MAX_ALLOWED_CELL_V;
    battery_model.smallest_V = MAX_ALLOWED_CELL_V;
    
    for (int i = 0; i < NUM_THERMISTOR; i++)
    {
        // start with temperatures that shouldn't cause any errors
        temp_model.temps_C[i] = OVER_TEMP_DEG_C - 10.0;
    }
}

void Periodic_1Hz(void)
{
    // Request or reject charging
    ChargeMonitor_1Hz(&battery_model);

    CAN_1Hz(&battery_model, &temp_model);
}

void Periodic_10Hz(void)
{
    // update information from the outside world
    SlaveInterface_read_cell_info(&battery_model);
    SlaveInterface_read_temperature_info(&temp_model);

    // // convert from thermistor voltage probe readings to temperatures
    TempConverter_convert(&temp_model);
   
    // // check the new slave board readings for errors
    PackMonitor_validate_battery_model_10Hz(&battery_model);
    PackMonitor_validate_temp_model_10Hz(&temp_model);


    // stage/unstage cell balancing based on cell voltage differences
    CellBalancer_stage_cell_draining(&battery_model);

    // for (int i = 0; i < NUM_SERIES_CELLS; i++)
    // if (i == 2 || i == 4) battery_model.cells[i].is_draining = 1; else battery_model.cells[i].is_draining = 0;
    // transmit new drain requests to slave board chips
    SlaveInterface_request_cell_draining(&battery_model);

    // update the bounds on State of Charge based on average pack voltage
    SOCestimator_voltage_threshold_update_10Hz(&battery_model, &temp_model);

    // update the discharge current limit
    CurrentLimiter_10Hz(&battery_model);

    // statuse LED blink algorithm iteration
    StatusLed_10Hz();

    CAN_10Hz(&battery_model, &temp_model);
}

void Periodic_1kHz(void)
{
    // read and filter a new current value from the sensor
    CurrentSense_1kHz();

    float filtered_current;
    if (CurrentSense_get_current(&filtered_current))
    {
        // check filtered current for problems, set faults if they exist
        CurrentMonitor_1kHz(filtered_current);
        SOCestimator_coulomb_count_update_1kHz(filtered_current);
    }
    else
    {
        // either the filtering in CurrentSense hasn't converged yet
        // or there was an error reading the current and a fault has been set
    }

    // shut down the car if any faults have been present for too long
    DriveMonitor_1kHz();

    CAN_1kHz();
}