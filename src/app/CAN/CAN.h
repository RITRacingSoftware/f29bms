#ifndef CAN_H
#define CAN_H
#include <stdbool.h>

#include "f29BmsConfig.h"
#include "f29bms_dbc.h"
#include "BatteryModel.h"
#include "TempModel.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct
{
    int id;
    int dlc;
    uint64_t data;
} can_message;

extern SemaphoreHandle_t can_message_transmit_semaphore;

/**
 * Holds the current state of the CAN bus.
 * Every signal in the DBC has a field in this.
 * Passed by reference into each CAN signal encoding function.
 * 
 * A packed CAN message can be extracted from this structure by using the pack functions.
 * 
 * So to send a CAN message, update the signals using the encode functions and 
 * then use CAN_send_message with data populated by a pack function.
*/
// extern can_obj_f29bms_dbc_h_t CAN_BUS;

typedef struct
{
    struct f29bms_dbc_bms_status_t bms_status;
    struct f29bms_dbc_bms_fault_vector_t bms_fault_vector;
    struct f29bms_dbc_bms_fault_alert_t bms_fault_alert;
    struct f29bms_dbc_bms_voltages_t bms_voltages;
    struct f29bms_dbc_bms_thermistor_voltages_t bms_thermistor_voltages;
    struct f29bms_dbc_bms_temperatures_t bms_temperatures;
    struct f29bms_dbc_bms_drain_status_a_t bms_drain_status_a;
    struct f29bms_dbc_bms_drain_status_b_t bms_drain_status_b;
    struct f29bms_dbc_bms_current_t bms_current;
    struct f29bms_dbc_bms_charge_request_t bms_charge_request;
    struct f29bms_dbc_bms_hard_fault_indicator_t bms_hard_fault_indicator;
    struct f29bms_dbc_bms_ref_t bms_ref;
    struct f29bms_dbc_watchdog_last_breath_t bms_watchdog_last_breath;
    struct f29bms_dbc_bms_current_limit_t bms_current_limit;
    struct f29bms_dbc_active_correction_current_limit_alert_t bms_current_limit_correction_count;
} CAN_BUS;

extern CAN_BUS can_bus;

/**
 * Initialize error status to none.
 */
void CAN_init(void);

/**
 * Attempt to send a CAN message with a given id.
 * Data is automatically retrieved from CAN_BUS.
 * id [in] - 11 bit CAN id- use the generated definitions in can_ids.h
 */
void CAN_send_message(unsigned long int id);

/**
 * Return true if last CAN message could not send, false otherwise.
 */
bool CAN_get_error(void);

/**
 * Sets the CAN_error variable
 */
void CAN_set_error(void);

/**
 * Clears the CAN_error variable
 */
void CAN_reset_error(void);

/**
 * Periodic CAN functions. Send periodic CAN messages.
 */
void CAN_1kHz(void);
void CAN_10Hz(BatteryModel_t* bm, TempModel_t* tm);
void CAN_1Hz(BatteryModel_t* bm, TempModel_t* tm);

/**
 * Processes recevied can messages
 */
void CAN_process_recieved_messages(void);


/**
 * Fills empty transmit mailboxes with CAN messages from the queue
 */
void CAN_send_queued_messages(void);

/**
 * Returns whether the transmit queue is empty. Must only be called from an ISR
 */
bool CAN_is_transmit_queue_empty_fromISR(void);

/**
 * Adds a received CAN message to the receive queue
 */
void CAN_add_message_rx_queue(uint32_t id, uint8_t dlc, uint8_t *data);

#endif // CAN_H
