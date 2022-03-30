#include "ChargeMonitor.h"
#include "CAN.h"
#include "common_macros.h"
#include "CurrentSense.h"
#include "f29BmsConfig.h"
#include "FaultManager.h"
#include "SOCestimator.h"

#include "HAL_Gpio.h"

// STATE MACHINE STUFF
typedef enum
{
    // The charger is disconnected from the vehicle
    ChargeState_DISCONNECTED,

    // Charging has completed or the BMS faulted and the cable must be cycled to restart charging
    ChargeState_CONNECTED_COMPLETE,
    
    // Cell balancing is occuring
    ChargeState_CONNECTED_BALANCING,

    // Charging is occuring
    ChargeState_CONNECTED_CHARGING,

    // Balancing Halted to Allow measurement
    ChargeState_CONNECTED_BALANCE_SENSING
} ChargeState_e;

typedef struct
{
    // is the charging system connected to the BMS?
    bool charger_connected;

    // are any cells still under CHARGED_CELL_V?
    bool cell_left_to_charge;

    // are any cells at MAX_ALLOWED_CELL_V?
    bool cell_at_max;

    // is the pack accepting enough current to warrant more charging?
    bool charge_current_nominal;

    // are there any faults active?
    bool bms_faulted;

    // are any cells eligible for balancing (> CHARGED_CELL_V)? 
    bool cell_over_charged;
} ChargeStateInputs_s;

typedef struct
{
    bool allow_balancing;
    bool request_charge;
    bool close_airs;
} ChargeStateOutputs_s;

static ChargeState_e state;
static int state_counter_seconds; // time spent in current state
static ChargeStateInputs_s sm_inputs;
static ChargeStateOutputs_s sm_outputs;

static void new_state(ChargeState_e next)
{
    state = next;
    state_counter_seconds = 0;

    if (next == ChargeState_CONNECTED_COMPLETE)
    {
        SOCestimator_reset_soc();
    }
}

static void sm_1Hz(void)
{
    // printf("ChargingState: ");
    switch(state)
    {
        case ChargeState_DISCONNECTED:
            // printf("DISCONNECTED");
            if (sm_inputs.charger_connected)
            {
                new_state(ChargeState_CONNECTED_BALANCING);
            }

            sm_outputs.request_charge = false;
            sm_outputs.allow_balancing = false;
            sm_outputs.close_airs = false;
            break;
        
        case ChargeState_CONNECTED_BALANCING:
        // printf("CONNECTED_BALANCING");
            if (!sm_inputs.charger_connected)
            {
                new_state(ChargeState_DISCONNECTED);
            }
            else if (sm_inputs.bms_faulted || !sm_inputs.cell_left_to_charge)
            {
                new_state(ChargeState_CONNECTED_COMPLETE);
            }
            else if (!sm_inputs.cell_over_charged)
            {
                new_state(ChargeState_CONNECTED_CHARGING);
            }
            else if (state_counter_seconds >= BALANCING_MEASURE_INTERVAL_S)
            {
                new_state(ChargeState_CONNECTED_BALANCE_SENSING);
            }

            sm_outputs.request_charge = false;
            sm_outputs.allow_balancing = true;
            sm_outputs.close_airs = true;
            break;
        
        case ChargeState_CONNECTED_BALANCE_SENSING:
        // printf("CONNECTED_BALALNCE_SENSING");
            if (!sm_inputs.charger_connected)
            {
                new_state(ChargeState_DISCONNECTED);
            }

            if (state_counter_seconds >= 1)
            {
                new_state(ChargeState_CONNECTED_BALANCING);
            }

            sm_outputs.allow_balancing = false;
            sm_outputs.request_charge = false;
            sm_outputs.close_airs = true;
            break;

        case ChargeState_CONNECTED_COMPLETE:
        // printf("CHARGING_CONNECTED_COMPLETE");
            if (!sm_inputs.charger_connected)
            {
                new_state(ChargeState_DISCONNECTED);
            }

            sm_outputs.request_charge = false;
            sm_outputs.allow_balancing = false;
            sm_outputs.close_airs = false;
            break;
        
        case ChargeState_CONNECTED_CHARGING:
        // printf("CONNECTED_CHARGING");
            if (!sm_inputs.charger_connected)
            {
                new_state(ChargeState_DISCONNECTED);
            }
            else if (!sm_inputs.cell_left_to_charge || sm_inputs.bms_faulted)
            {
                new_state(ChargeState_CONNECTED_COMPLETE);
            }
            else if (!sm_inputs.charge_current_nominal && (state_counter_seconds >= CHARGE_CURRENT_SETTLE_TIME_S))
            {
                new_state(ChargeState_CONNECTED_COMPLETE);
            }
            else if (sm_inputs.cell_at_max)
            {
                new_state(ChargeState_CONNECTED_BALANCING);
            }

            sm_outputs.request_charge = true;
            sm_outputs.allow_balancing = false;
            sm_outputs.close_airs = true;
            break;
    }
    // printf("\r\n");
    state_counter_seconds++;
}

static void start_charging(void)
{
    // set bit in can message
    // yes, according to Elcon its 1 to stop and 0 to start...
    can_bus.bms_charge_request.bms_charge_request_control = 0;
}

static void close_airs(void)
{
    // assert charge enable line
    HAL_Gpio_write(GpioPin_CHARGE_ENABLE, 1);
}

static void stop_charging(void)
{
    // clear bit in can message
    // yes, according to Elcon its 1 to stop and 0 to start...
    can_bus.bms_charge_request.bms_charge_request_control = 1;
}

static void open_airs(void)
{
    // de-assert charge enable line
    HAL_Gpio_write(GpioPin_CHARGE_ENABLE, 0);
}

void ChargeMonitor_init(void)
{
    // don't start up requesting charge
    stop_charging();

    state = ChargeState_DISCONNECTED;
    state_counter_seconds = 0;
    sm_inputs.charger_connected = false;
    sm_inputs.cell_at_max = false;
    sm_inputs.charge_current_nominal = true;
    sm_inputs.cell_left_to_charge = false;
    sm_inputs.bms_faulted = false;
    sm_inputs.cell_over_charged = false;

    sm_outputs.allow_balancing = false;
    sm_outputs.request_charge = false;

    can_bus.bms_charge_request.bms_charge_request_max_current = f29bms_dbc_bms_charge_request_bms_charge_request_max_current_encode(MAX_CHARGING_CURRENT_A);
    can_bus.bms_charge_request.bms_charge_request_max_voltage = f29bms_dbc_bms_charge_request_bms_charge_request_max_voltage_encode(MAX_CHARGING_V);
}

/**
 * Request charging if the charger is connected.
 * Check the average cell voltage in the battery pack.
 * Once it crosses the fully charged threshold, request to stop charging.
 */
void ChargeMonitor_1Hz(BatteryModel_t* bm)
{
    // first get updated state machine inputs
    sm_inputs.charger_connected = HAL_Gpio_read(GpioPin_CHARGER_AVAILABLE);
    sm_inputs.cell_left_to_charge = bm->smallest_V < CHARGED_CELL_V;
    sm_inputs.cell_at_max = FLOAT_GT_EQ(bm->largest_V, MAX_ALLOWED_CELL_V, VOLTAGE_TOLERANCE);
    sm_inputs.cell_over_charged = FLOAT_GT(bm->largest_V, CHARGED_CELL_V, VOLTAGE_TOLERANCE);
    sm_inputs.bms_faulted = FaultManager_is_any_fault_active();
    float current;
    if (CurrentSense_get_current(&current))
    {
        sm_inputs.charge_current_nominal = (current >= (MAX_CHARGING_CURRENT_A / 10.0));
    }
    else
    {
        // current filter not initialized yet, we just booted up
        sm_inputs.charge_current_nominal = false;
    }

    // next, run the state machine
    sm_1Hz();

    // finally, apply outputs
    // ** allow_balancing is applied using a getter
    if (sm_outputs.request_charge)
    {
        start_charging();
    }
    else
    {
        stop_charging();
    }

    if (sm_outputs.close_airs)
    {
        close_airs();
    }
    else
    {
        open_airs();
    }

    // Actually send the charger control CAN message, if we are connected to the charger
    if (state != ChargeState_DISCONNECTED)
    {
        CAN_send_message(F29BMS_DBC_BMS_CHARGE_REQUEST_FRAME_ID);
    }

    can_bus.bms_status.bms_status_charge_state = state;
}

/**
 * Return true if the charger is connected, false otherwise.
 */
bool ChargeMonitor_charger_available(void)
{
    return sm_inputs.charger_connected;
}

bool ChargeMonitor_is_charging(void)
{
    return sm_outputs.request_charge;
}

bool ChargeMonitor_is_balancing_allowed(void)
{
    return sm_outputs.allow_balancing;
}

