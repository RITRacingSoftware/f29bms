#include <stdbool.h>

#include "SOCestimator.h"

#include "f29BmsConfig.h"
#include "common_macros.h"
#include "counters.h"
#include "CAN.h"
#include "HAL_EEPROM.h"

float SOC_raw;
float SOC_limit;

/**
 * Use the average series cell voltage and ambient temperature to determine a maximum SOC.
 * Voltage thresholds determined (shittily) by eyeballing voltage/discharge % graph on the cell datasheet.
 * avg_V [in] - average series cell voltage
 * ambient_temp_C [in] - ambient temperature
 */
static int SOC_limit_from_voltage(float avg_V, float ambient_temp_C)
{
    // determine which temperature range the ambient temp falls into
    // saturate to safe values
    int temp_index = SAT((ambient_temp_C + .5) / 10, 0, NUM_TEMP_RANGES - 1);
    
    if (avg_V < SOC_25_LIMIT_FOR_TEMP_INDEX(temp_index))
    {
        return 25;
    }
    else if (avg_V < SOC_50_LIMIT_FOR_TEMP_INDEX(temp_index))
    {
        return 50;
    }
    else if (avg_V < SOC_75_LIMIT_FOR_TEMP_INDEX(temp_index))
    {
        return 75;
    }
    else
    {
        return 100;
    }
}

bms_eeprom_t flash_map;

/**
 * Write the current SOC to eeprom for use on future startups.
 * soc_to_save [in] 0 to 100
 */
static void write_saved_soc(float soc_to_save)
{
    flash_map.saved_soc_en = 1;
    flash_map.saved_soc = soc_to_save;

    // indicate the saved value is valid for next runtime
    HAL_EEPROM_write(&flash_map);
}

/**
 * Read the last recorded SOC.
 * return 0 to 100 if SOC was recorded, -1 otherwise.
 */
static float read_saved_soc(void)
{
    HAL_EEPROM_read(&flash_map);

    float soc = -1;
    
    if (flash_map.saved_soc_en == 1)
    {
        soc = flash_map.saved_soc;
    }

    return soc;
}

/**
 * Calculate the change in SOC (delta SOC) resulting from a current held over an elapsed time.
 * current_A [in] - current held
 * time_elapsed_ms [in] - time elapsed
 */
static inline float calculate_dSOC(float current_A, unsigned int time_elapsed_ms)
{
    // -1 because positive current is exiting the battery pack
    // 100 converts to %
    return -1.0 * 100.0 * (current_A * ((float) time_elapsed_ms) * MS_TO_HR) / BATTERY_CAPACITY_Ah;
}

// SOC waits for an average voltage reading to determine starting SOC.
// This flag stays false until this first average voltage reading is recieved.
// Current readings up to this point are aggregated and applied to the starting SOC model
// as soon as the first voltage is received.
bool starting_soc_calculated;
// keep track of current and time expired prior to the initial SOC estimate
float pre_init_current_A;

void SOCestimator_init(void)
{
    SOC_raw = 100;
    SOC_limit = 100;
    pre_init_current_A = 0.0;

    float saved_soc = read_saved_soc();
    if (saved_soc != -1)
    {
        SOC_raw = saved_soc;
        starting_soc_calculated = true;
    }
    else
    {
        starting_soc_calculated = false;
    }
    
    can_bus.bms_status.bms_status_soc = SOC_limit;
    can_bus.bms_status.bms_status_soc_raw = SOC_raw;
}

/**
 * Use the instantaneous current to update the coulomb counting SOC model.
 * current [in] - the instantaneous current (negative for discharge, positive for charging)
 */
void SOCestimator_coulomb_count_update_1kHz(float current_A)
{
    if (starting_soc_calculated)
    {
        SOC_raw = SOC_raw + calculate_dSOC(current_A, 1);
        SOC_raw = SAT(SOC_raw, 0, 100);
    }
    else
    {
        pre_init_current_A += current_A;
    }

    can_bus.bms_status.bms_status_soc_raw = SOC_raw;
}

/**
 * Calculate the average voltage of the battery cells. Limit the SOC measurement to
 * the highest SOC possible at this voltage.
 * battery_model [in] - new cell voltages to use in calculating the SOC
 * ambient_temp [in] - ambient temperature
 */
void SOCestimator_voltage_threshold_update_10Hz(BatteryModel_t* battery_model, TempModel_t* tm)
{
    float tot_temp = 0;
    for (int i = 0; i < NUM_THERMISTOR; i++)
    {
        tot_temp += tm->temps_C[i];
    }

    int new_soc_limit = SOC_limit_from_voltage(battery_model->average_V, tot_temp / (float) NUM_THERMISTOR);

    // limit may flicker here
    SOC_limit = new_soc_limit;

    if (!starting_soc_calculated)
    {
        // This is the first voltage calculated.
        // Change the SOC as if all the pre-init current readings happened over 1ms.
        // This only works because the current measurements are all over 1ms.
        SOC_raw = SOC_raw + calculate_dSOC(pre_init_current_A, 1);
        SOC_raw = SAT(SOC_raw, 0, 100);
        starting_soc_calculated = true;
    }
    can_bus.bms_status.bms_status_soc = (uint8_t) (SOCestimator_get_soc_corrected() + .5);

    static int save_counter_ms = 0;
    if (incr_to_limit(&save_counter_ms, SOC_SAVE_PERIOD_MS, 100))
    {
        SOCestimator_save_soc();
        save_counter_ms = 0; //Reset Counter
    }
}

/**
 * Get the SOC as calculated by coulomb counting without voltage threshold limit.
 * return int [0:100]
 */
float SOCestimator_get_soc_raw(void)
{
    return SOC_raw;
}

/**
 * Get the SOC as calculated by coulomb counting with the voltage threshold limit applied.
 * return int [0:100]
 */
float SOCestimator_get_soc_corrected(void)
{
    return MIN(SOC_limit, SOC_raw);
}

void SOCestimator_save_soc(void)
{
   write_saved_soc(SOCestimator_get_soc_corrected());
}

void SOCestimator_reset_soc(void)
{
    SOC_raw = 100;
    SOC_limit = 100;
    SOCestimator_save_soc();
}