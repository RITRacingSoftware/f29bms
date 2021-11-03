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
#include "semphr.h"

// the global battery model!
static BatteryModel_t battery_model;
// the global temperature model!
static TempModel_t temp_model;

// mutex locks for thread safetey, one for battery and temp
// Locks indefinitely until the process is done
// Lock before function and unlock after function
// With &battery_model and &temp_model to maintain task safety
static SemaphoreHandle_t mutex_bat; 
static SemaphoreHandle_t mutex_temp;


void Periodic_init(void)
{
    // Create mutexes on init
    mutex_bat = xSemaphoreCreateMutex();
    mutex_temp = xSemaphoreCreateMutex();

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
    // Take the mutex (lock)
    xSemaphoreTake(mutex_bat, portMAX_DELAY);
    // Request or reject charging
    ChargeMonitor_1Hz(&battery_model);
    // Unlock
    xSemaphoreGive(mutex_bat);
    
    CAN_1Hz();
}

void Periodic_10Hz(void)
{
    // update information from the outside world
    xSemaphoreTake(mutex_bat, portMAX_DELAY);
    SlaveInterface_read_cell_info(&battery_model);
    xSemaphoreGive(mutex_bat);

    xSemaphoreTake(mutex_temp, portMAX_DELAY);
    SlaveInterface_read_temperature_info(&temp_model);
    xSemaphoreGive(mutex_temp);

    // convert from thermistor voltage probe readings to temperatures
    xSemaphoreTake(mutex_temp, portMAX_DELAY);
    TempConverter_convert(&temp_model);
    xSemaphoreGive(mutex_temp);

    // check the new slave board readings for errors
    xSemaphoreTake(mutex_bat, portMAX_DELAY);
    PackMonitor_validate_battery_model_10Hz(&battery_model);
    xSemaphoreGive(mutex_bat);

    xSemaphoreTake(mutex_temp, portMAX_DELAY);
    PackMonitor_validate_temp_model_10Hz(&temp_model);
    xSemaphoreGive(mutex_temp);

    // stage/unstage cell balancing based on cell voltage differences
    xSemaphoreTake(mutex_bat, portMAX_DELAY);
    CellBalancer_stage_cell_draining(&battery_model);
    xSemaphoreGive(mutex_bat);

    // for (int i = 0; i < NUM_SERIES_CELLS; i++)
    // if (i == 2 || i == 4) battery_model.cells[i].is_draining = 1; else battery_model.cells[i].is_draining = 0;
    // transmit new drain requests to slave board chips
    xSemaphoreTake(mutex_bat, portMAX_DELAY);
    SlaveInterface_request_cell_draining(&battery_model);
    xSemaphoreGive(mutex_bat);

    // update the bounds on State of Charge based on average pack voltage
    xSemaphoreTake(mutex_bat, portMAX_DELAY);
    xSemaphoreTake(mutex_temp, portMAX_DELAY);
    SOCestimator_voltage_threshold_update_10Hz(&battery_model, &temp_model);
    xSemaphoreGive(mutex_bat);
    xSemaphoreGive(mutex_temp);

    // statuse LED blink algorithm iteration
    StatusLed_10Hz();

    xSemaphoreTake(mutex_bat, portMAX_DELAY);
    xSemaphoreTake(mutex_temp, portMAX_DELAY);
    CAN_10Hz(&battery_model, &temp_model);
    xSemaphoreGive(mutex_bat);
    xSemaphoreGive(mutex_temp);
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