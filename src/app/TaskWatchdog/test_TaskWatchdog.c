// includes for mocking
#include "unity.h"

// module under test
#include "TaskWatchdog.h"

#include "MockCAN.h"
#include "MockHAL_Can.h"

CAN_BUS can_bus;
//static task_id_E tasks[NUM_TASKS] = {task_id_PERIODIC_1Hz, task_id_PERIODIC_10Hz, task_id_PERIODIC_1kHz, task_id_CAN};

void test_TaskWatchdog_expired_if_not_pet(void)
{
    TaskWatchdog_init();

    // check that with no petting the task is marked expired
    // must update this test if we decide to use > 100 size buckets...
    bool tasks_expired[NUM_TASKS];
    for (int i = 0; i < NUM_TASKS; i++)
    {
        tasks_expired[i] = false;
    }
    Error_t ret;
    ret.active=false;
    HAL_Can_send_message_ExpectAnyArgsAndReturn(ret);

    for (int i = 0; i < 6000; i++)
    {
        if(!TaskWatchdog_expired())
        {
            TaskWatchdog_tick(task_id_PERIODIC_10Hz);
        }
    }

    TEST_ASSERT(TaskWatchdog_expired() == true);
}

void test_TaskWatchdog_not_expired_if_pet(void)
{
    TaskWatchdog_init();

    for (int i = 0; i < 10000; i++)
    {
        for (int task = 0; task < NUM_TASKS; task++)
        {
            TaskWatchdog_pet((task_id_E) task);
            TaskWatchdog_tick((task_id_E) task);
        }
    }

    TEST_ASSERT(TaskWatchdog_expired() == false);
}