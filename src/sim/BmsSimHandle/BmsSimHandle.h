#ifndef BMS_SIM_HANDLE
#define BMS_SIM_HANDLE

#include <stdbool.h>

#include "BmsSimConfig.h"
#include "BmsSim.pb.h"

/**
 * C module for performing I/O on the f29bms simulation.
 * 
 * // TODO - IMPLEMENT CAN
 * 
 * The f29bms simulation is the same f29bms code, but compiled for linux for testing.
 * This module provides ways for communicating with the linux version of the f29bms driver layer,
 * and ticking the freertos clock, advancing time within the program.
 * 
 * The program is spawned as a separate process through fork and exec calls, and communicated to using
 * sockets for driver I/O, and a linux signal for the clock ticks.
 */

/**
 * Set up server socket connection and wait for sim client to connect.
 * Return -1 on any errors.
 */
int BmsSim_init(void);

/**
 * Fork and exec the f29bms program and save the PID for future use.
 * Return -1 if the fork failed. Return -2 if exec failed.
 */
int BmsSim_start(void);

/**
 * Create a BLF (Vector binary logging format) to record received CAN messages in.
 * filename [in] - null terminated name of the file (including .blf)
 */
int BmsSim_begin_logging(char* filename);

/**
 * Kill the f29bms process.
 */
void BmsSim_stop(void);

/**
 * Pass a signal to the f29bms process that will trigger a FreeRTOS tick. 
 */
void BmsSim_tick(void);

bool BmsSim_get_status_led(void);

bool BmsSim_get_shutdown_line(void);

bool BmsSim_get_watchdog_pet(void);

bool BmsSim_get_charge_enable(void);

void BmsSim_set_charger_available(bool charger_available);

void BmsSim_set_current(float current);

/**
 * Stage new thermistor data to send to the f29bms process on the next tick.
 * therm_index [in] - index of thermistor to change temperature of
 * voltage [in] - new voltage (volts) to set thermistor to
 */
void BmsSim_set_temp_info(int therm_index, float voltage);

/**
 * Stage new cell data to send to the f29bms process on the next tick.
 * cell_index [in] - index of cell to change data for
 * voltage [in] - voltage (volts) to set the cell to
 * is_draining [in] - drain status to set the cell to
 */
void BmsSim_set_cell_info(int cell_index, float voltage, bool is_draining);

/**
 * Get the latest drain state requested for a cell by the f29bms process.
 * cell_index [in] - index of the cell to check
 * return true if draining requested, false otherwise
 */
bool BmsSim_read_drain_state(int cell_index);

/**
 * Get the next CAN message in the queue for processing.
 * data [out] - can message data
 * return can id, or -1 if no more messages
 */
unsigned long int BmsSim_next_can_msg(int64_t* data);

#endif // BMS_SIM_HANDLE