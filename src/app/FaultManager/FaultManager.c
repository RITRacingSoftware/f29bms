#include <stdint.h>

#include "common_macros.h"
#include "CAN.h"
#include "FaultManager.h"
#include "f29BmsConfig.h"

/**
 * Each bit represents fault status.
 * 1 for active
 * 0 for inactive.
 * 
 * Fault bits are indexed using the FaultCode_e enum values.
 */
static uint64_t fault_vector;

void FaultManager_init(void)
{
    // zero out every fault/latch bit
    fault_vector = 0;

    // initialize the fault vector CAN message
    f29bms_dbc_bms_fault_vector_unpack(&can_bus.bms_fault_vector, (uint8_t*) &fault_vector, 8);
}

// TODO- use mutex around faultvector
void FaultManager_set_fault_active(FaultCode_e code, void* data)
{
    // dont do anything if fault is already sent. Don't need an alert spam.
    if (((fault_vector & BIT(code)) == 0) && ((BIT(code) & DISABLE_FAULT_MASK) == 0))
    {
        // set the fault
        fault_vector |= BIT(code);

        // set the CAN alert data field accordingly
        switch(code)
        {
            case FaultCode_OVER_CURRENT:
                can_bus.bms_fault_alert.bms_fault_alert_current = f29bms_dbc_bms_fault_alert_bms_fault_alert_current_encode(*((float*)data));
                break;
            
            case FaultCode_SLAVE_COMM_CELLS:
                can_bus.bms_fault_alert.bms_fault_alert_cell_comm_slave_board_num = f29bms_dbc_bms_fault_alert_bms_fault_alert_cell_comm_slave_board_num_encode(*((uint8_t*)data));
                break;
            
            case FaultCode_SLAVE_COMM_TEMPS:
                can_bus.bms_fault_alert.bms_fault_alert_temp_comm_slave_board_num = f29bms_dbc_bms_fault_alert_bms_fault_alert_temp_comm_slave_board_num_encode(*((uint8_t*)data));
                break;

            case FaultCode_SLAVE_COMM_DRAIN_REQUEST:
                can_bus.bms_fault_alert.bms_fault_alert_drain_comm_slave_board_num = f29bms_dbc_bms_fault_alert_bms_fault_alert_drain_comm_slave_board_num_encode(*((uint8_t*)data));
                break;

            case FaultCode_CURRENT_SENSOR_COMM:
                can_bus.bms_fault_alert.bms_fault_alert_adc_error_code = f29bms_dbc_bms_fault_alert_bms_fault_alert_adc_error_code_encode(*((uint8_t*)data));
                break;
            
            case FaultCode_CELL_VOLTAGE_IRRATIONAL:
                can_bus.bms_fault_alert.bms_fault_alert_irrational_voltage = f29bms_dbc_bms_fault_alert_bms_fault_alert_irrational_voltage_encode(*((float*)data));
                break;
            
            case FaultCode_CELL_VOLTAGE_DIFF:
                can_bus.bms_fault_alert.bms_fault_alert_voltage_diff = f29bms_dbc_bms_fault_alert_bms_fault_alert_voltage_diff_encode(*((float*)data));
                break;
            
            case FaultCode_OUT_OF_JUICE:
                can_bus.bms_fault_alert.bms_fault_alert_lowest_cell_voltage = f29bms_dbc_bms_fault_alert_bms_fault_alert_lowest_cell_voltage_encode(*((float*)data));
                break;
            
            case FaultCode_DRAIN_FAILURE:
                can_bus.bms_fault_alert.bms_fault_alert_failed_drain_cell = f29bms_dbc_bms_fault_alert_bms_fault_alert_failed_drain_cell_encode(*((int*)data));
                break;
            
            case FaultCode_TEMPERATURE_IRRATIONAL: 
                can_bus.bms_fault_alert.bms_fault_alert_irrational_temperature = f29bms_dbc_bms_fault_alert_bms_fault_alert_irrational_temperature_encode(*((float*)data));
                break;
            
            case FaultCode_OVER_TEMPERATURE:
                can_bus.bms_fault_alert.bms_fault_alert_over_temperature = f29bms_dbc_bms_fault_alert_bms_fault_alert_over_temperature_encode(*((float*)data));
                break;

               /**
                * SHRIYA TODO - add switch case for encoding manual fault
                */
            case ManualFault: 
            
                break 

            default:
                // send garbage data
                break;
        }

        // set the mux of the alert message to the fault code
        can_bus.bms_fault_alert.bms_fault_alert_code = f29bms_dbc_bms_fault_alert_bms_fault_alert_code_encode((uint8_t) code);

        CAN_send_message(F29BMS_DBC_BMS_FAULT_ALERT_FRAME_ID);

        // update the fault vector CAN message data
        f29bms_dbc_bms_fault_vector_unpack(&can_bus.bms_fault_vector, (uint8_t*)&fault_vector, 8);
    }
}

// TODO- use mutex around faultvector
void FaultManager_clear_fault(FaultCode_e code)
{
    uint64_t temp_fault_vector = fault_vector;

    // clear the fault bit
    temp_fault_vector &= ~BIT(code);

    // atomically? set the fault vector
    // this is only atomic if the stm32 supports 64 bit instructions...
    fault_vector = temp_fault_vector;

    // update the fault vector CAN message data
    f29bms_dbc_bms_fault_vector_unpack(&can_bus.bms_fault_vector, (uint8_t*)&fault_vector, 8);}

bool FaultManager_is_fault_active(FaultCode_e code)
{
    return (fault_vector & BIT(code)) != 0;
}

bool FaultManager_is_fault_enabled(FaultCode_e code)
{
    return (DISABLE_FAULT_MASK & (1 << code) == 0);
}

bool FaultManager_is_any_fault_active(void)
{
    return (fault_vector != 0);
}