#include "TaskWatchdog.h"
#include "CAN.h"
#include "f29bms_dbc.h"
#include "HAL_Can.h"
#include <stdio.h>
#include "HAL_Uart.h"


// #define LAST_BREATH_CAN_ID 0x0BB

// Nominal operation that should be maintained in each bucket by its respective task
#define TASK_BUCKET_MAX 5

// How much a bucket gets incremented by a task pet
#define TASK_BUCKET_PET 2

// How much a bucket gets decremented by a task tick
#define TASK_BUCKET_TICK 1

#define PERIODIC_1HZ_MAX_PERIOD 1000
#define PERIODIC_10HZ_MAX_PERIOD 100
#define PERIODIC_1KHZ_MAX_PERIOD 1
#define CAN_MAX_PERIOD 1

typedef struct
{
	int counter;
	int period;
	int bucket;
} task_data_S;

static task_data_S tasks[NUM_TASKS];
static task_id_E expired_task;
static bool expired;

static void set_expired(task_id_E task);
static task_id_E get_expired_task(void);

void TaskWatchdog_init(void)
{
    tasks[task_id_PERIODIC_1Hz].counter = 0;
	tasks[task_id_PERIODIC_1Hz].period = PERIODIC_1HZ_MAX_PERIOD;
	tasks[task_id_PERIODIC_1Hz].bucket = TASK_BUCKET_MAX;

	tasks[task_id_PERIODIC_10Hz].counter = 0;
	tasks[task_id_PERIODIC_10Hz].period = PERIODIC_10HZ_MAX_PERIOD;
	tasks[task_id_PERIODIC_10Hz].bucket = TASK_BUCKET_MAX;
 
    tasks[task_id_PERIODIC_1kHz].counter = 0;
	tasks[task_id_PERIODIC_1kHz].period = PERIODIC_1KHZ_MAX_PERIOD;
	tasks[task_id_PERIODIC_1kHz].bucket = TASK_BUCKET_MAX;

    expired = false;
}

void TaskWatchdog_pet(task_id_E task)
{
    tasks[task].bucket += TASK_BUCKET_PET;

    // cap bucket at max
    if (tasks[task].bucket > TASK_BUCKET_MAX)
    {
        tasks[task].bucket = TASK_BUCKET_MAX;
    }
}

/*
 * Decrement the task bucket for the given task. Sets task expired if bucket reaches 0
 * Should decrement by less than TaskWatchdog_pet increments.
 * task [in] - the given task
 */
void TaskWatchdog_tick(task_id_E task)
{
    tasks[task].counter += 1;

    if(tasks[task].counter > tasks[task].period){
        tasks[task].bucket -= TASK_BUCKET_TICK;
		tasks[task].counter = 0;
    }
    if(tasks[task].bucket <= 0)
    {
        set_expired(task);
    }
}


void send_expired_payload()
{
    uint64_t msg_data; 
    if (-1 != f29bms_dbc_watchdog_last_breath_pack((uint8_t*) &msg_data, &can_bus.bms_watchdog_last_breath, 8))
    {
        HAL_Can_send_message(F29BMS_DBC_WATCHDOG_LAST_BREATH_FRAME_ID, 8, msg_data);
    }
}

void set_expired(task_id_E task)
{
	expired_task = task;
	expired = true;
   
    //Right now, if there is an infinite loop in the 1Hz task, the watchdog will reset the microcontroller, however
    //the can message will have the 1KHz task bit set, as if the 1KHz task stopped running, as it will because the 
    //1Hz task has a higher priority, so if it is stuck, no other tasks (other than the watchdog task) will run,
    //causing the 1KHz task to timeout (which happens faster than the 1Hz task). 
    if(task == task_id_PERIODIC_1Hz){
        can_bus.bms_watchdog_last_breath.watchdog_last_breath_1_hz_task_expired = true;
    }
    else if(task == task_id_PERIODIC_10Hz){
        can_bus.bms_watchdog_last_breath.watchdog_last_breath_10_hz_task_expired = true;
    }
    else if(task == task_id_PERIODIC_1kHz){
        can_bus.bms_watchdog_last_breath.watchdog_last_breath_1_khz_task_expired = true;
    }
	send_expired_payload();
}

bool TaskWatchdog_expired(void)
{
	return expired;
}

task_id_E get_expired_task(void)
{
	return expired_task;
}
