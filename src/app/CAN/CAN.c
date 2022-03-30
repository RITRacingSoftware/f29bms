#include <math.h>
#include <stdio.h>

#include "CAN.h"
#include "HAL_Can.h"
#include "common_macros.h"
#include "HAL_Uart.h"


#define TICKS_TO_WAIT_QUEUE_CAN_MESSAGE (0) //Will return immediately if queue is full, not sure if this should be different

// can_obj_f29bms_dbc_h_t CAN_BUS;
CAN_BUS can_bus;


static bool can_error;

static QueueHandle_t tx_can_message_queue;
static QueueHandle_t rx_can_message_queue;

void CAN_init(void)
{
    tx_can_message_queue = xQueueCreate(CAN_QUEUE_LEN, sizeof(can_message));
    rx_can_message_queue= xQueueCreate(CAN_QUEUE_LEN, sizeof(can_message));
    can_error = false;
    can_bus.bms_current_limit_correction_count.count_active_correction_count = 0;
}

static int pack_message(int id, uint8_t* msg_data)
{
    switch(id)
    {
        case F29BMS_DBC_BMS_STATUS_FRAME_ID:
             return f29bms_dbc_bms_status_pack(msg_data, &can_bus.bms_status, 8);

        case F29BMS_DBC_BMS_FAULT_VECTOR_FRAME_ID:
            return f29bms_dbc_bms_fault_vector_pack(msg_data, &can_bus.bms_fault_vector, 8);

        case F29BMS_DBC_BMS_FAULT_ALERT_FRAME_ID:
            return f29bms_dbc_bms_fault_alert_pack(msg_data, &can_bus.bms_fault_alert, 8);

        case F29BMS_DBC_BMS_VOLTAGES_FRAME_ID:
            return f29bms_dbc_bms_voltages_pack(msg_data, &can_bus.bms_voltages, 8);
        
        case F29BMS_DBC_BMS_THERMISTOR_VOLTAGES_FRAME_ID:
            return f29bms_dbc_bms_thermistor_voltages_pack(msg_data, &can_bus.bms_thermistor_voltages, 8);
        
        case F29BMS_DBC_BMS_TEMPERATURES_FRAME_ID:
            return f29bms_dbc_bms_temperatures_pack(msg_data, &can_bus.bms_temperatures, 8);

        case F29BMS_DBC_BMS_DRAIN_STATUS_A_FRAME_ID:
            return f29bms_dbc_bms_drain_status_a_pack(msg_data, &can_bus.bms_drain_status_a, 8);

        case F29BMS_DBC_BMS_DRAIN_STATUS_B_FRAME_ID:
            return f29bms_dbc_bms_drain_status_b_pack(msg_data, &can_bus.bms_drain_status_b, 8);

        case F29BMS_DBC_BMS_CURRENT_FRAME_ID:
            return f29bms_dbc_bms_current_pack(msg_data, &can_bus.bms_current, 8);
        
        case F29BMS_DBC_BMS_CHARGE_REQUEST_FRAME_ID:
            return f29bms_dbc_bms_charge_request_pack(msg_data, &can_bus.bms_charge_request, 8);

        case F29BMS_DBC_BMS_REF_FRAME_ID:
            return f29bms_dbc_bms_ref_pack(msg_data, &can_bus.bms_ref, 8);
        
        case F29BMS_DBC_WATCHDOG_LAST_BREATH_FRAME_ID:
            return f29bms_dbc_watchdog_last_breath_pack(msg_data, &can_bus.bms_watchdog_last_breath, 8);

        case F29BMS_DBC_BMS_CURRENT_LIMIT_FRAME_ID:
            return f29bms_dbc_bms_current_limit_pack(msg_data, &can_bus.bms_current_limit, 8);
            
        case F29BMS_DBC_ACTIVE_CORRECTION_CURRENT_LIMIT_ALERT_FRAME_ID:
            return f29bms_dbc_active_correction_current_limit_alert_pack(msg_data, &can_bus.bms_current_limit_correction_count, 8);

        default:
            printf("f29bms: unknown CAN id: %d\n", id);
            break;
    }

    return -1;
}

void CAN_send_message(unsigned long int id)
{
    uint64_t msg_data;

    // get the message data for the given id
    // if(id == F29BMS_DBC_BMS_DRAIN_STATUS_A_FRAME_ID)
    // {
        if (-1 != pack_message(id, (uint8_t*) &msg_data))
        {
            can_message thisMessage = {id, 8, msg_data};
            xQueueSend(tx_can_message_queue, &thisMessage, 0);
            xSemaphoreGive(can_message_transmit_semaphore);
        }
        else
        {
            // CAN id invalid, dont attempt to send the message
            can_error = true;
            printf("CAN ERROR: %x\n", id);
        }
    // }
}

bool CAN_get_error(void)
{
    return can_error;
}

void CAN_set_error(void)
{
    can_error = true;
}

void CAN_reset_error(void)
{
    can_error = false;
}

void CAN_1kHz(void)
{
    CAN_send_message(F29BMS_DBC_BMS_CURRENT_FRAME_ID);
}

void CAN_10Hz(BatteryModel_t* bm, TempModel_t* tm)
{
    uint64_t msg_data = 0; 

    // Multiplexed message cycling. Mux value cycles every 100ms
    static uint8_t voltages_mux = 0;
    msg_data |= voltages_mux;
    const int voltages_per_message = 6; 
    // must pack these kinds of messages manually or call 90 signal encoding functions individually
    for (uint64_t i = 0; i < voltages_per_message; i++)
    {
        const uint64_t voltages_start_bit = 8;
        const uint64_t voltage_len_bits = 9;
        const uint16_t voltage_mask = 0b111111111;
        const float voltage_granularity = 0.01;

        int cell_index = voltages_mux * voltages_per_message + i;
        float cell_voltage = bm->cells[cell_index].voltage;
        uint64_t frac_voltage = ROUND_INT((cell_voltage / voltage_granularity));
        frac_voltage &= voltage_mask;
        msg_data |= (frac_voltage << (voltages_start_bit + i*voltage_len_bits)); 
    }
    f29bms_dbc_bms_voltages_unpack(&can_bus.bms_voltages, (uint8_t*)(&msg_data), 8);
    CAN_send_message(F29BMS_DBC_BMS_VOLTAGES_FRAME_ID);
    voltages_mux = (voltages_mux + 1) % 15; // mux limits like this are hardcoded according to the DBC maximum mux values (here its m14)

    // msg_data = 0;
    // static uint8_t thermistor_mux = 0;
    // msg_data |= thermistor_mux;
    // const int therm_per_message = 6;
    // for (int i = 0; i < therm_per_message; i++)
    // {
    //     const int therm_start_bit = 2;
    //     const int therm_len_bits = 9;
    //     const int therm_mask = 0b111111111;
    //     const float therm_granularity = 0.01;

    //     int therm_index = thermistor_mux * therm_per_message + i;
    //     if (therm_index < NUM_THERMISTOR)
    //     {
    //         float therm_voltage = tm->tm_readings_V[therm_index];
    //         // printf("Therm voltage: %f\n", therm_voltage);
    //         uint64_t frac_voltage = ROUND_INT((therm_voltage / therm_granularity)) & therm_mask;
    //         // printf("Fractional voltage: %x\n", frac_voltage);
    //         uint64_t shifted_fractional = (frac_voltage << (uint64_t)(therm_start_bit + i*therm_len_bits));
    //         msg_data |= shifted_fractional;
    //         // printf("index %d value %lx\n", therm_index, shifted_fractional);

    //     }
    // }
    // f29bms_dbc_bms_thermistor_voltages_unpack(&can_bus.bms_thermistor_voltages, (uint8_t*)&msg_data, 8);
    // CAN_send_message(F29BMS_DBC_BMS_THERMISTOR_VOLTAGES_FRAME_ID);
    // thermistor_mux = (thermistor_mux + 1) % 4;

    // msg_data = 0;
    // static int temperature_mux = 0;
    // msg_data |= temperature_mux;
    // const int temp_per_message = 7;
    // for (int i = 0; i < temp_per_message; i++)
    // {
    //     const int temp_start_bit = 2;
    //     const int temp_len_bits = 8;
    //     const int temp_mask = 0b11111111;
    //     const float temp_granularity = 1;

    //     int temp_index = temperature_mux * temp_per_message + i;
    //     if (temp_index < NUM_THERMISTOR)
    //     {
    //         float temp = SAT(tm->temps_C[temp_index], -128, 127);
    //         uint64_t frac_temp;
    //         if (temp >= 0)
    //         {
    //             frac_temp = ROUND_INT((temp / temp_granularity)) & temp_mask;
    //         }
    //         else
    //         {
    //              frac_temp = ROUND_INT(((temp * -1) / temp_granularity)) & temp_mask;
    //              frac_temp *= -1;
    //              frac_temp &= temp_mask;
    //         }

    //         msg_data |= (frac_temp << (temp_start_bit + i*temp_len_bits)); 
    //     }
    // }

    // f29bms_dbc_bms_temperatures_unpack(&can_bus.bms_temperatures, (uint8_t*)&msg_data, 8);
    // CAN_send_message(F29BMS_DBC_BMS_TEMPERATURES_FRAME_ID);
    // temperature_mux = (temperature_mux + 1) % 3;

    // drain states
    // uint64_t a_drain_states = 0;
    // uint64_t b_drain_states = 0;
    // for (int i = 0; i < NUM_SERIES_CELLS; i++)
    // {
    //     uint64_t draining = (bm->cells[i].is_draining ? 1 : 0);
    //     if (i < 64)
    //     {
            
    //         a_drain_states |= (draining << i);
    //     }
    //     else
    //     {
    //         b_drain_states |= (draining << (i - 64));
    //     }
    // }

    // f29bms_dbc_bms_drain_status_a_unpack(&can_bus.bms_drain_status_a, (uint8_t*)&a_drain_states, 8);
    // f29bms_dbc_bms_drain_status_b_unpack(&can_bus.bms_drain_status_b, (uint8_t*)&b_drain_states, 8);

    // CAN_send_message(F29BMS_DBC_BMS_DRAIN_STATUS_A_FRAME_ID);
    // CAN_send_message(F29BMS_DBC_BMS_DRAIN_STATUS_B_FRAME_ID);

    // static int bms_ref_mux = 0;
    // can_bus.bms_ref.bms_ref_mux = bms_ref_mux;
    // can_bus.bms_ref.bms_ref_ref0 = f29bms_dbc_bms_ref_bms_ref_ref0_encode(tm->vref2s[0]);
    // can_bus.bms_ref.bms_ref_ref1 = f29bms_dbc_bms_ref_bms_ref_ref1_encode(tm->vref2s[1]);
    // can_bus.bms_ref.bms_ref_ref2 = f29bms_dbc_bms_ref_bms_ref_ref2_encode(tm->vref2s[2]);
    // can_bus.bms_ref.bms_ref_ref3 = f29bms_dbc_bms_ref_bms_ref_ref3_encode(tm->vref2s[3]);
    // can_bus.bms_ref.bms_ref_ref4 = f29bms_dbc_bms_ref_bms_ref_ref4_encode(tm->vref2s[4]);
    // can_bus.bms_ref.bms_ref_ref5 = f29bms_dbc_bms_ref_bms_ref_ref5_encode(tm->vref2s[5]);
    // can_bus.bms_ref.bms_ref_ref6 = f29bms_dbc_bms_ref_bms_ref_ref6_encode(tm->vref2s[6]);
    // can_bus.bms_ref.bms_ref_ref7 = f29bms_dbc_bms_ref_bms_ref_ref7_encode(tm->vref2s[7]);
    // can_bus.bms_ref.bms_ref_ref8 = f29bms_dbc_bms_ref_bms_ref_ref8_encode(tm->vref2s[8]);
    // can_bus.bms_ref.bms_ref_ref9 = f29bms_dbc_bms_ref_bms_ref_ref9_encode(tm->vref2s[9]);
    // can_bus.bms_ref.bms_ref_ref10 = f29bms_dbc_bms_ref_bms_ref_ref10_encode(tm->vref2s[10]);
    // can_bus.bms_ref.bms_ref_ref11 = f29bms_dbc_bms_ref_bms_ref_ref11_encode(tm->vref2s[11]);
    // CAN_send_message(F29BMS_DBC_BMS_REF_FRAME_ID);
    // bms_ref_mux ^= 1;
    CAN_send_message(F29BMS_DBC_BMS_CURRENT_LIMIT_FRAME_ID);
}



void CAN_1Hz(BatteryModel_t* bm, TempModel_t* tm)
{
    CAN_send_message(F29BMS_DBC_BMS_STATUS_FRAME_ID);
    CAN_send_message(F29BMS_DBC_BMS_FAULT_VECTOR_FRAME_ID);

    uint64_t msg_data = 0;
    static uint8_t thermistor_mux = 0;
    msg_data |= thermistor_mux;
    const int therm_per_message = 6;
    for (int i = 0; i < therm_per_message; i++)
    {
        const int therm_start_bit = 2;
        const int therm_len_bits = 9;
        const int therm_mask = 0b111111111;
        const float therm_granularity = 0.01;

        int therm_index = thermistor_mux * therm_per_message + i;
        if (therm_index < NUM_THERMISTOR)
        {
            float therm_voltage = tm->tm_readings_V[therm_index];
            // printf("Therm voltage: %f\n", therm_voltage);
            uint64_t frac_voltage = ROUND_INT((therm_voltage / therm_granularity)) & therm_mask;
            // printf("Fractional voltage: %x\n", frac_voltage);
            uint64_t shifted_fractional = (frac_voltage << (uint64_t)(therm_start_bit + i*therm_len_bits));
            msg_data |= shifted_fractional;
            // printf("index %d value %lx\n", therm_index, shifted_fractional);

        }
    }
    f29bms_dbc_bms_thermistor_voltages_unpack(&can_bus.bms_thermistor_voltages, (uint8_t*)&msg_data, 8);
    CAN_send_message(F29BMS_DBC_BMS_THERMISTOR_VOLTAGES_FRAME_ID);
    thermistor_mux = (thermistor_mux + 1) % 4;

    msg_data = 0;
    static int temperature_mux = 0;
    msg_data |= temperature_mux;
    const int temp_per_message = 7;
    for (int i = 0; i < temp_per_message; i++)
    {
        const int temp_start_bit = 2;
        const int temp_len_bits = 8;
        const int temp_mask = 0b11111111;
        const float temp_granularity = 1;

        int temp_index = temperature_mux * temp_per_message + i;
        if (temp_index < NUM_THERMISTOR)
        {
            float temp = SAT(tm->temps_C[temp_index], -128, 127);
            uint64_t frac_temp;
            if (temp >= 0)
            {
                frac_temp = ROUND_INT((temp / temp_granularity)) & temp_mask;
            }
            else
            {
                 frac_temp = ROUND_INT(((temp * -1) / temp_granularity)) & temp_mask;
                 frac_temp *= -1;
                 frac_temp &= temp_mask;
            }

            msg_data |= (frac_temp << (temp_start_bit + i*temp_len_bits)); 
        }
    }

    f29bms_dbc_bms_temperatures_unpack(&can_bus.bms_temperatures, (uint8_t*)&msg_data, 8);
    CAN_send_message(F29BMS_DBC_BMS_TEMPERATURES_FRAME_ID);
    temperature_mux = (temperature_mux + 1) % 3;

    //drain states
    uint64_t a_drain_states = 0;
    uint64_t b_drain_states = 0;
    for (int i = 0; i < NUM_SERIES_CELLS; i++)
    {
        uint64_t draining = (bm->cells[i].is_draining ? 1 : 0);
        if (i < 64)
        {
            
            a_drain_states |= (draining << i);
        }
        else
        {
            b_drain_states |= (draining << (i - 64));
        }
    }

    f29bms_dbc_bms_drain_status_a_unpack(&can_bus.bms_drain_status_a, (uint8_t*)&a_drain_states, 8);
    f29bms_dbc_bms_drain_status_b_unpack(&can_bus.bms_drain_status_b, (uint8_t*)&b_drain_states, 8);

    CAN_send_message(F29BMS_DBC_BMS_DRAIN_STATUS_A_FRAME_ID);
    CAN_send_message(F29BMS_DBC_BMS_DRAIN_STATUS_B_FRAME_ID);

    static int bms_ref_mux = 0;
    can_bus.bms_ref.bms_ref_mux = bms_ref_mux;
    can_bus.bms_ref.bms_ref_ref0 = f29bms_dbc_bms_ref_bms_ref_ref0_encode(tm->vref2s[0]);
    can_bus.bms_ref.bms_ref_ref1 = f29bms_dbc_bms_ref_bms_ref_ref1_encode(tm->vref2s[1]);
    can_bus.bms_ref.bms_ref_ref2 = f29bms_dbc_bms_ref_bms_ref_ref2_encode(tm->vref2s[2]);
    can_bus.bms_ref.bms_ref_ref3 = f29bms_dbc_bms_ref_bms_ref_ref3_encode(tm->vref2s[3]);
    can_bus.bms_ref.bms_ref_ref4 = f29bms_dbc_bms_ref_bms_ref_ref4_encode(tm->vref2s[4]);
    can_bus.bms_ref.bms_ref_ref5 = f29bms_dbc_bms_ref_bms_ref_ref5_encode(tm->vref2s[5]);
    can_bus.bms_ref.bms_ref_ref6 = f29bms_dbc_bms_ref_bms_ref_ref6_encode(tm->vref2s[6]);
    can_bus.bms_ref.bms_ref_ref7 = f29bms_dbc_bms_ref_bms_ref_ref7_encode(tm->vref2s[7]);
    can_bus.bms_ref.bms_ref_ref8 = f29bms_dbc_bms_ref_bms_ref_ref8_encode(tm->vref2s[8]);
    can_bus.bms_ref.bms_ref_ref9 = f29bms_dbc_bms_ref_bms_ref_ref9_encode(tm->vref2s[9]);
    can_bus.bms_ref.bms_ref_ref10 = f29bms_dbc_bms_ref_bms_ref_ref10_encode(tm->vref2s[10]);
    can_bus.bms_ref.bms_ref_ref11 = f29bms_dbc_bms_ref_bms_ref_ref11_encode(tm->vref2s[11]);
    CAN_send_message(F29BMS_DBC_BMS_REF_FRAME_ID);
    bms_ref_mux ^= 1;
}

void CAN_process_recieved_messages(void)
{
    can_message received_message;
    //Get all can messages received
    while (xQueueReceive(rx_can_message_queue, &received_message, TICKS_TO_WAIT_QUEUE_CAN_MESSAGE) == pdTRUE)
    {
        uint8_t data[8];
        for (int i = 0; i < 8; i++)
        {
            data[i] = (received_message.data >> (i*8)) & 0xff;
        }
        // uint8_t print_buffer[50];
        // uint8_t n = sprintf(&print_buffer[0], "ID: %d  Data: %d  %d  %d  %d  %d  %d  %d  %d\n\r", received_message.id, data[0],
        //      data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        // HAL_Uart_send(&print_buffer[0], n);
        //Unpack message recieved
        switch(received_message.id)
        {
            case F29BMS_DBC_BMS_STATUS_FRAME_ID:
                f29bms_dbc_bms_status_unpack(&can_bus.bms_status, (uint8_t*)&received_message.data, 8);
                break;

            case F29BMS_DBC_BMS_FAULT_VECTOR_FRAME_ID:
                f29bms_dbc_bms_fault_vector_unpack(&can_bus.bms_fault_vector, (uint8_t*)&received_message.data, 8);
                break;

            case F29BMS_DBC_BMS_FAULT_ALERT_FRAME_ID:
                f29bms_dbc_bms_fault_alert_unpack(&can_bus.bms_fault_alert, (uint8_t*)&received_message.data, 8);
                break;

            case F29BMS_DBC_BMS_VOLTAGES_FRAME_ID:
                f29bms_dbc_bms_voltages_unpack(&can_bus.bms_voltages, (uint8_t*)&received_message.data, 8);
                break;
            
            case F29BMS_DBC_BMS_THERMISTOR_VOLTAGES_FRAME_ID:
                f29bms_dbc_bms_thermistor_voltages_unpack(&can_bus.bms_thermistor_voltages, (uint8_t*)&received_message.data, 8);
                break;
            
            case F29BMS_DBC_BMS_TEMPERATURES_FRAME_ID:
                f29bms_dbc_bms_temperatures_unpack(&can_bus.bms_temperatures, (uint8_t*)&received_message.data, 8);
                break;

            case F29BMS_DBC_BMS_DRAIN_STATUS_A_FRAME_ID:
                f29bms_dbc_bms_drain_status_a_unpack(&can_bus.bms_drain_status_a, (uint8_t*)&received_message.data, 8);
                break;

            case F29BMS_DBC_BMS_DRAIN_STATUS_B_FRAME_ID:
                f29bms_dbc_bms_drain_status_b_unpack(&can_bus.bms_drain_status_b, (uint8_t*)&received_message.data, 8);
                break;

            case F29BMS_DBC_BMS_CURRENT_FRAME_ID:
                f29bms_dbc_bms_current_unpack(&can_bus.bms_current, (uint8_t*)&received_message.data, 8);
                break;
            
            case F29BMS_DBC_BMS_CHARGE_REQUEST_FRAME_ID:
                f29bms_dbc_bms_charge_request_unpack(&can_bus.bms_charge_request, (uint8_t*)&received_message.data, 8);
                break;

            case F29BMS_DBC_BMS_REF_FRAME_ID:
                f29bms_dbc_bms_ref_unpack(&can_bus.bms_ref, (uint8_t*)&received_message.data, 8);
                break;
            
            default:
                // printf("f29bms: unknown CAN id: %d\n", received_message.id);
                break;
        }

    }
}


void CAN_send_queued_messages(void)
{
    //Check how many mailboxes are free, and put a new message in each empty mailbox, if there are any messages
    uint8_t num_free_mailboxes = HAL_number_of_empty_mailboxes();
    can_message dequeued_message;
    while (num_free_mailboxes > 0) //Fill all empty mailboxes with messages
    {
        if (xQueueReceive(tx_can_message_queue, &dequeued_message, TICKS_TO_WAIT_QUEUE_CAN_MESSAGE) == pdTRUE) //Get next message to send if there is one
        {
            Error_t err = HAL_Can_send_message(dequeued_message.id, dequeued_message.dlc, dequeued_message.data);
            can_error = err.active;
        }
        else
        {
            break;
        }   
        num_free_mailboxes--;
    }
}

bool CAN_is_transmit_queue_empty_fromISR(void)
{
    return xQueueIsQueueEmptyFromISR(tx_can_message_queue) == pdTRUE;
}

void CAN_add_message_rx_queue(uint32_t id, uint8_t dlc, uint8_t *data)
{
    uint64_t msg_data = 0x0;
    memcpy(&msg_data, data, sizeof(msg_data));
    
    can_message rx_msg;
    rx_msg.data = msg_data;
    rx_msg.id = id;
    rx_msg.dlc = dlc;
    xQueueSend(rx_can_message_queue, &rx_msg, 10);
}
