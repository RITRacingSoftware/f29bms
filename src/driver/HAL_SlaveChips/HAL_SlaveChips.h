#ifndef HAL_SLAVECHIPS_H
#define HAL_SLAVECHIPS_H

#include <stdbool.h>
#include "BatteryModel.h"
#include "FaultManager.h"

void HAL_SlaveChips_init(void);

//voltages: array to be populated with cell voltages
//is_draining: array to be populated with whether each cell is draining
//num: number of cells to get info from
Error_t HAL_SlaveChips_get_all_cell_data(float* voltages, bool* is_draining, unsigned int num);

//num: number of thermistors
Error_t HAL_SlaveChips_get_all_tm_readings(float* temperatures, float* vref2s, unsigned int num);
//Error_t HAL_SlaveChips_get_all_tm_readings(float* temperatures, unsigned int num);

//cells: drain state to set for each cell
//num: number of cells to set drain state for
Error_t HAL_SlaveChips_request_cell_drain_state(bool* cells, unsigned int num);

#endif // HAL_SLAVECHIPS_H